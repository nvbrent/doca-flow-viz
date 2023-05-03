APP_NAME = meson.project_name()

app_srcs = [
	'wrappers.c',
	'wrapper_init.c',
	'counter_spy.cpp',
]

spy_lib = shared_library(APP_NAME, app_srcs,
	cpp_args : cpp_args,
	dependencies : app_dependencies,
	include_directories : app_inc_dirs)
