app_srcs = [
	'flow_viz.cpp',
	'flow_viz_mermaid.cpp',
	'flow_viz_util.cpp',
	'flow_viz_wrappers.cpp',
]

app_inc_dirs = [include_directories(doca_common_dir_path)]

flow_viz_lib = shared_library(meson.project_name(), 
    app_srcs,
	dependencies : app_dependencies,
	include_directories : app_inc_dirs)
