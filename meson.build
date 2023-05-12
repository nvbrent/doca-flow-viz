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

project('doca-flow-viz', 'C', 'CPP',
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
app_dependencies += dependency('threads', required : true)
app_dependencies += dependency('doca', required : true)

doca_common_dir_path = '/opt/mellanox/doca/applications/common/src'

subdir('lib')
subdir('test')
