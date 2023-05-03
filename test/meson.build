test_exe = executable('sample_flow_program', 
	[
        'sample_flow_program.c',
        common_dir_path + '/dpdk_utils.c',
        common_dir_path + '/offload_rules.c',
        common_dir_path + '/utils.c',
    ],
	cpp_args : cpp_args,
	dependencies : app_dependencies,
	include_directories : app_inc_dirs,
    install: false)

test('sample_flow_program', test_exe,
    args: [
        '-a17:00.0,dv_flow_en=2', 
        '-a17:00.1,dv_flow_en=2', 
        '-c0x1'
    ],
    env: ['LD_PRELOAD=' + spy_lib.full_path()])
