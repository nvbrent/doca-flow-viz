#absl_log_dep = dependency('absl_log', required : true)

gen_srcs = [
    protoc_gen.process(proto_files), 
    grpc_gen.process(proto_files)
]

spy_rpc_lib = static_library('counter_spy_rpc', 
    gen_srcs, 
    dependencies : [dep_protobuf, dep_grpc] )

app_srcs = [
	'wrappers.c',
	'wrapper_init.c',
	'counter_spy.cpp',
]

app_inc_dirs = [include_directories(common_dir_path)]
app_inc_dirs += spy_rpc_lib.private_dir_include()

spy_lib = shared_library(meson.project_name(), 
    app_srcs,
	dependencies : app_dependencies,
    link_with: spy_rpc_lib,
	include_directories : app_inc_dirs)
