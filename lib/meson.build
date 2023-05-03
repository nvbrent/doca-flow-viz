APP_NAME = meson.project_name()

app_srcs = [
	'wrappers.c',
	'wrapper_init.c',
	'counter_spy.cpp',
]

protoc = find_program('protoc', required : true)
protobuf_dep = dependency('protobuf', required : true)
grpc_cpp_plugin = find_program('grpc_cpp_plugin')
grpc_dep = dependency('grpc++', required : true)

app_dependencies += protobuf_dep
app_dependencies += grpc_dep

protoc_gen = generator(protoc, \
    output    : [
        '@BASENAME@.pb.cc', 
        '@BASENAME@.pb.h'],
    arguments : [
        '--proto_path=@SOURCE_ROOT@/protos', 
        '--cpp_out=@BUILD_DIR@', 
        '@INPUT@'])

grpc_gen = generator(protoc, \
    output    : [
        '@BASENAME@.grpc.pb.cc', 
        '@BASENAME@.grpc.pb.h'],
    arguments : [
        '--proto_path=@SOURCE_ROOT@/protos',
        '--grpc_out=@BUILD_DIR@',
        '--plugin=protoc-gen-grpc=' + grpc_cpp_plugin.full_path(),
        '@INPUT@'])

gen_srcs = [protoc_gen.process(proto_files), grpc_gen.process(proto_files)]
app_srcs += gen_srcs

spy_lib = shared_library(APP_NAME, app_srcs,
	cpp_args : cpp_args,
	dependencies : app_dependencies,
	include_directories : app_inc_dirs)
