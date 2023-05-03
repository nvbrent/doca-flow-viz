protoc = find_program('protoc', required : true)
grpc_python_plugin = find_program('grpc_python_plugin')

protoc_gen_py = generator(protoc, \
    output    : [
        '@BASENAME@_pb2.py'],
    arguments : [
        '--proto_path=@SOURCE_ROOT@/protos', 
        '--python_out=@BUILD_DIR@',
        '@INPUT@'])

grpc_gen_py = generator(protoc, \
    output    : [
        '@BASENAME@_pb2_grpc.py'],
    arguments : [
        '--proto_path=@SOURCE_ROOT@/protos',
        '--grpc_out=@BUILD_DIR@',
        '--plugin=protoc-gen-grpc=' + grpc_python_plugin.full_path(),
        '@INPUT@'])

gen_srcs_py = [
    protoc_gen_py.process(proto_files), 
    grpc_gen_py.process(proto_files)]

custom_target('dummy', 
    command: [ 'cp', '@INPUT@', '@OUTPUT@' ],
    input: gen_srcs_py, 
    output: gen_srcs_py,
    build_by_default : true)
