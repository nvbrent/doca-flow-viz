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
	default_options: ['buildtype=debug']
)

cpp_args = ['-std=c++11']

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

common_dir_path = '/opt/mellanox/doca/applications/common/src'

app_inc_dirs = [include_directories(common_dir_path)]

proto_files = files('protos/counter_spy.proto')

subdir('lib')
subdir('test')
