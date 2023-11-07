#include <signal.h>


#include <dpdk_utils.h>
#include <utils.h>

#include <doca_argp.h>
#include <doca_log.h>
#include <doca_flow.h>

DOCA_LOG_REGISTER(sample);

volatile bool force_quit = false;

static void
signal_handler(int signum)
{
	if (signum == SIGINT || signum == SIGTERM) {
		printf("\n\nSignal %d received, preparing to exit...\n", signum);
		force_quit = true;
	}
}

static void install_signal_handler(void)
{
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
}

////////////////////////////////////////////////////////////////////////////////
// DOCA Flow

static struct doca_flow_port *
port_init(uint16_t port_id)
{
	char port_id_str[128];
	snprintf(port_id_str, sizeof(port_id_str), "%d", port_id);

	struct doca_flow_port_cfg port_cfg = {
		.port_id = port_id,
		.type = DOCA_FLOW_PORT_DPDK_BY_ID,
		.devargs = port_id_str,
	};
	struct doca_flow_port * port;
    doca_error_t res = doca_flow_port_start(&port_cfg, &port);
	if (port == NULL) {
		DOCA_LOG_ERR("failed to initialize doca flow port: %s", doca_error_get_descr(res));
		return NULL;
	}
	return port;
}

int
flow_init(
    struct application_dpdk_config *dpdk_config,
    struct doca_flow_port *ports[])
{
	struct doca_flow_cfg arp_sc_flow_cfg = {
		.mode_args = "vnf,hws,isolated",
		.queues = dpdk_config->port_config.nb_queues,
        .queue_depth = 128,
		.resource.nb_counters = 1024,
	};
	doca_error_t res = doca_flow_init(&arp_sc_flow_cfg);
    if (res != DOCA_SUCCESS) {
		DOCA_LOG_ERR("failed to init doca: %s", doca_error_get_descr(res));
		return -1;
	}
	DOCA_LOG_DBG("DOCA flow init done");

	for (uint16_t port_id = 0; port_id < dpdk_config->port_config.nb_ports; port_id++) {
		ports[port_id] = port_init(port_id);
	}

	/* pair the two ports together for hairpin forwarding */
	for (uint16_t port_id = 0; port_id + 1 < dpdk_config->port_config.nb_ports; port_id += 2) {	
		if (doca_flow_port_pair(ports[port_id], ports[port_id + 1])) {
			DOCA_LOG_ERR("DOCA Flow port pairing failed");
			return -1;
		}
	}

	DOCA_LOG_DBG("DOCA flow init done");
	return 0;
}

void
flow_destroy(struct doca_flow_port *ports[], uint16_t num_ports)
{
	for (uint16_t port_id = 0; port_id < num_ports; port_id++)
	{
        doca_flow_port_pipes_flush(ports[port_id]);
		doca_flow_port_stop(ports[port_id]);
	}
    doca_flow_destroy();
}

void create_flows(
    struct doca_flow_port *ports[], 
    uint16_t num_ports)
{
    for (uint16_t port_id=0; port_id<num_ports; port_id++)
    {
        doca_error_t res;
        struct doca_flow_pipe *pipe = NULL;
        struct 
        doca_flow_match match = {};
        struct doca_flow_monitor mon = { .counter_type = DOCA_FLOW_RESOURCE_TYPE_NON_SHARED };
        struct doca_flow_fwd miss = { .type = DOCA_FLOW_FWD_DROP };

        struct doca_flow_fwd hairpin = { .type = DOCA_FLOW_FWD_PORT, .port_id = port_id ^ 1 };
        struct doca_flow_pipe_cfg hairpin_cfg = {
            .attr = {
                .name = "HAIRPIN",
            },
            .port = ports[port_id],
            .match = &match,
        };

        res = doca_flow_pipe_create(&hairpin_cfg, &hairpin, &miss, &pipe);
        if (res != DOCA_SUCCESS) {
		    DOCA_LOG_ERR("Failed to create Pipe: %s", doca_error_get_descr(res));
        }

        uint16_t rss_queues[] = { 0 };
        struct doca_flow_fwd fwd = { .type = DOCA_FLOW_FWD_RSS, .num_of_queues=1, .rss_queues=rss_queues };
        struct doca_flow_fwd sample_miss = { .type = DOCA_FLOW_FWD_PIPE, .next_pipe = pipe };
        struct doca_flow_pipe_cfg cfg = {
            .attr = {
                .name = "THIRD_PIPE",
            },
            .port = ports[port_id],
            .match = &match,
            .monitor = &mon,
        };

        res = doca_flow_pipe_create(&cfg, &fwd, &sample_miss, &pipe);
        if (res != DOCA_SUCCESS) {
		    DOCA_LOG_ERR("Failed to create Pipe: %s", doca_error_get_descr(res));
        }

        for (int j=0; j<5; j++) {
            struct doca_flow_pipe_entry *entry = NULL;
            res = doca_flow_pipe_add_entry(0, pipe, NULL, NULL, NULL, NULL, 0, NULL, &entry);
        }

        struct doca_flow_match match_tcp = { 
            .parser_meta.outer_l3_type = DOCA_FLOW_L3_META_IPV4,
            .parser_meta.outer_l4_type = DOCA_FLOW_L4_META_TCP 
        };
        struct doca_flow_fwd fwd_next = { .type = DOCA_FLOW_FWD_PIPE, .next_pipe = pipe };
        struct doca_flow_pipe_cfg cfg_next = {
            .attr = {
                .name = "SECOND_PIPE",
            },
            .port = ports[port_id],
            .match = &match_tcp,
            .monitor = &mon,
        };
        res = doca_flow_pipe_create(&cfg_next, &fwd_next, &miss, &pipe);
        if (res != DOCA_SUCCESS) {
		    DOCA_LOG_ERR("Failed to create Pipe: %s", doca_error_get_descr(res));
        }

        for (int j=0; j<2; j++) {
            struct doca_flow_pipe_entry *entry = NULL;
            res = doca_flow_pipe_add_entry(0, pipe, NULL, NULL, NULL, NULL, 0, NULL, &entry);
        }

        struct doca_flow_match match_ipv4 = { .parser_meta.outer_l3_type = DOCA_FLOW_L3_META_IPV4 };
        struct doca_flow_pipe_cfg cfg_root = {
            .attr = {
                .name = "ROOT_PIPE",
                .is_root = true,
            },
            .port = ports[port_id],
            .match = &match_ipv4,
            .monitor = NULL, // "Counter action on root table is not supported in HW steering mode"
        };
        struct doca_flow_fwd fwd_root = { .type = DOCA_FLOW_FWD_PIPE, .next_pipe = pipe };
        res = doca_flow_pipe_create(&cfg_root, &fwd_root, &miss, &pipe);
        if (res != DOCA_SUCCESS) {
		    DOCA_LOG_ERR("Failed to create Pipe: %s", doca_error_get_descr(res));
        }

        struct doca_flow_pipe_entry *entry = NULL;
        res = doca_flow_pipe_add_entry(0, pipe, NULL, NULL, NULL, NULL, 0, NULL, &entry);
    }
}

int main(int argc, char *argv[])
{
	doca_error_t result = doca_log_backend_create_standard();
    if (result != DOCA_SUCCESS) {
        fprintf(stderr, "Failed to open doca log backend; giving up\n");
        return -1;
    }

	struct application_dpdk_config dpdk_config = {
		.port_config.nb_ports = 2,
		.port_config.nb_queues = 1,
		.port_config.nb_hairpin_q = 1,
	};

	/* Parse cmdline/json arguments */
	doca_argp_init("SAMPLE", &dpdk_config);
	doca_argp_set_dpdk_program(dpdk_init);
	doca_argp_start(argc, argv);

	install_signal_handler();

	dpdk_queues_and_ports_init(&dpdk_config);

    struct doca_flow_port *ports[dpdk_config.port_config.nb_ports];
	flow_init(&dpdk_config, ports);

    create_flows(ports, dpdk_config.port_config.nb_ports);

    // Now we immediately shut down

	flow_destroy(ports, dpdk_config.port_config.nb_ports);
	doca_argp_destroy();

    return 0;
}
