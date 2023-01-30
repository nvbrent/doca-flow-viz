/*
 * Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
 *
 * This software product is a proprietary product of NVIDIA CORPORATION &
 * AFFILIATES (the "Company") and all right, title, and interest in and to the
 * software product, including all associated intellectual property rights, are
 * and shall remain exclusively with the Company.
 *
 * This software product is governed by the End User License Agreement
 * provided with the software product.
 *
 */

#include <getopt.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <dpdk_utils.h>
#include <sig_db.h>
#include <utils.h>

#include <doca_argp.h>
#include <doca_log.h>
#include <doca_flow.h>

#include <rte_malloc.h>
#include <rte_ethdev.h>
#include <rte_hash.h>
#include <rte_jhash.h>

DOCA_LOG_REGISTER(SAMPLE);

////////////////////////////////////////////////////////////////////////////////
// Signal Handling

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
// RSS Packet Processing

static int
packet_parsing_example(const struct rte_mbuf *packet)
{
	struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(packet, struct rte_ether_hdr *);
	uint16_t ether_type = htons(eth_hdr->ether_type);

	if (ether_type == RTE_ETHER_TYPE_IPV4) {
		DOCA_LOG_DBG("Received IPV4");
	} else if (ether_type == RTE_ETHER_TYPE_IPV6) {
		DOCA_LOG_DBG("received IPV6");
	}

	return 0;
}

#define MAX_RX_BURST_SIZE 256

void
example_burst_rx(uint16_t port_id, uint16_t queue_id)
{
	struct rte_mbuf *rx_packets[MAX_RX_BURST_SIZE];

	uint32_t lcore_id = rte_lcore_id();

	double tsc_to_seconds = 1.0 / (double)rte_get_timer_hz();

	while (!force_quit) {
		uint64_t t_start = rte_rdtsc();

		uint16_t nb_rx_packets = rte_eth_rx_burst(port_id, queue_id, rx_packets, MAX_RX_BURST_SIZE);
		for (int i=0; i<nb_rx_packets; i++) {
			packet_parsing_example(rx_packets[i]);
		}

		double sec = (double)(rte_rdtsc() - t_start) * tsc_to_seconds;

		if (nb_rx_packets) {
			printf("L-Core %d processed %d packets in %f seconds", lcore_id, nb_rx_packets, sec);
		}
	}
}

int
sample_lcore_func(void *lcore_args)
{
	uint32_t lcore_id = rte_lcore_id();
	uint16_t port_id = (uint16_t)lcore_id; // assumes 1-to-1 mapping
	uint16_t queue_id = 0; // assumes only 1 queue
	example_burst_rx(port_id, queue_id);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Store data in rte_hash

#define MAX_HT_ENTRIES 4096

struct sample_key
{
	rte_be32_t src_ip;
	rte_be32_t dst_ip;
};

struct sample_entry
{
	struct sample_key key;
	uint64_t num_packets;
	uint64_t num_bytes;
};

struct rte_hash_parameters sample_ht_params = {
	.name = "sample_ht",
	.entries = MAX_HT_ENTRIES,
	.key_len = sizeof(struct sample_key),
	.hash_func = rte_jhash,
	.hash_func_init_val = 0,
	.extra_flag = 0, // see RTE_HASH_EXTRA_FLAGS_RW_CONCURRENCY_LF
};

void
sample_hash_ops(void)
{
	struct rte_hash * ht = rte_hash_create(&sample_ht_params);

	struct sample_entry * entry = rte_zmalloc(NULL, sizeof(struct sample_entry), 0);
	entry->key.src_ip = RTE_BE32(0x11223344);
	entry->key.dst_ip = RTE_BE32(0x55667788);
	entry->num_packets = 1;
	entry->num_bytes = 0x1000;

	rte_hash_add_key_data(ht, &entry->key, entry);

	struct sample_key lookup_key = {
		.src_ip = RTE_BE32(0x11223344),
		.dst_ip = RTE_BE32(0x55667788),
	};
	struct sample_entry * lookup = NULL;
	if (rte_hash_lookup_data(ht, &lookup_key, (void**)&lookup) >= 0)
	{
		rte_hash_del_key(ht, &lookup_key);
		rte_free(lookup);
	}
}

////////////////////////////////////////////////////////////////////////////////
// Parsing args with argp

struct sample_config
{
	// TODO: config fields here
	bool sample_flag;
};

static doca_error_t
sample_callback(void *config, void *param)
{
	struct sample_config * sample = config;
	sample->sample_flag = *(bool *)param;
	return DOCA_SUCCESS;
}

void
sample_register_argp_params(void)
{
	struct doca_argp_param * sample_flag_param = NULL;
	int ret = doca_argp_param_create(&sample_flag_param);
	if (ret != DOCA_SUCCESS)
		DOCA_LOG_ERR("Failed to create ARGP param: %s", doca_get_error_string(ret));
	doca_argp_param_set_short_name(sample_flag_param, "f");
	doca_argp_param_set_long_name(sample_flag_param, "flag");
	doca_argp_param_set_description(sample_flag_param, "Sets the sample flag");
	doca_argp_param_set_callback(sample_flag_param, sample_callback);
	doca_argp_param_set_type(sample_flag_param, DOCA_ARGP_TYPE_BOOLEAN);
	ret = doca_argp_register_param(sample_flag_param);
	if (ret != DOCA_SUCCESS)
		DOCA_LOG_ERR("Failed to register program param: %s", doca_get_error_string(ret));
	
	// Repeat for each parameter
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
	struct doca_flow_error err = {};
	struct doca_flow_port * port = doca_flow_port_start(&port_cfg, &err);
	if (port == NULL) {
		DOCA_LOG_ERR("failed to initialize doca flow port: %s", err.message);
		return NULL;
	}
	return port;
}

int
flow_init(struct application_dpdk_config *dpdk_config)
{
	struct doca_flow_cfg arp_sc_flow_cfg = {
		.mode_args = "vnf,hws,isolated",
		.queues = dpdk_config->port_config.nb_queues,
		.resource.nb_counters = 1024,
	};
	struct doca_flow_error err = {};

	if (doca_flow_init(&arp_sc_flow_cfg, &err) < 0) {
		DOCA_LOG_ERR("failed to init doca: %s", err.message);
		return -1;
	}
	DOCA_LOG_DBG("DOCA flow init done");

	struct doca_flow_port *ports[dpdk_config->port_config.nb_ports];
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
flow_destroy(uint16_t num_ports)
{
	struct rte_flow_error error = {};
	for (uint16_t port_id = 0; port_id < num_ports; port_id++)
	{
		rte_flow_flush(port_id, &error);
		rte_eth_dev_stop(port_id);
		rte_eth_dev_close(port_id);
	}
}

int
main(int argc, char **argv)
{
	struct application_dpdk_config dpdk_config = {
		.port_config.nb_ports = 2,
		.port_config.nb_queues = 1,
		.port_config.nb_hairpin_q = 1,
	};

	struct sample_config config;

	/* Parse cmdline/json arguments */
	doca_argp_init("SAMPLE", &config);
	doca_argp_set_dpdk_program(dpdk_init);
	sample_register_argp_params();
	doca_argp_start(argc, argv);

	install_signal_handler();

	dpdk_queues_and_ports_init(&dpdk_config);

	flow_init(&dpdk_config);

	uint32_t lcore_id;
	RTE_LCORE_FOREACH_WORKER(lcore_id) {
		rte_eal_remote_launch(sample_lcore_func, &config, lcore_id);
	}
	RTE_LCORE_FOREACH_WORKER(lcore_id) {
		rte_eal_wait_lcore(lcore_id);
	}
	
	flow_destroy(dpdk_config.port_config.nb_ports);
	doca_argp_destroy();

	return 0;
}
