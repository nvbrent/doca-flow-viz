#
# Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
#
# This software product is a proprietary product of NVIDIA CORPORATION &
# AFFILIATES (the "Company") and all right, title, and interest in and to the
# software product, including all associated intellectual property rights, are
# and shall remain exclusively with the Company.
#
# This software product is governed by the End User License Agreement
# provided with the software product.
#

project('doca-counter-spy', 'C', 'CPP',
	version: '0.0.1',
	default_options: [
		'buildtype=debug',
		'cpp_std=c++17']
)

add_project_arguments('-D DOCA_ALLOW_EXPERIMENTAL_API', language: ['c','cpp'])

dependency_libdpdk = dependency('libdpdk', required : false)
if not dependency_libdpdk.found()
	error('Please set PKG_CONFIG_PATH to DPDK packages path, for further instructions please refer to the DOCA Reference Applications guide')
endif
app_dependencies = [dependency_libdpdk]
#app_dependencies += dependency('json-c', required : true)
app_dependencies += dependency('threads', required : true)
app_dependencies += dependency('doca', required : true)
app_dependencies += dependency('grpc', required : true)

protoc          = find_program('protoc', required : true)
grpc_cpp_plugin = find_program('grpc_cpp_plugin')

dep_protobuf = dependency('protobuf', required : true)
dep_grpc     = dependency('grpc++', required : true)

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

common_dir_path = '/opt/mellanox/doca/applications/common/src'

proto_files = files('protos/counter_spy.proto')

subdir('lib')
subdir('test')
