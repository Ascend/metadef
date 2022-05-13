project(metadef_benchmark)

file(GLOB_RECURSE EXE_GRAPH_SRCS CONFIGURE_DEPENDS "*.cc")

add_executable(exec_graph_benchmark ${EXE_GRAPH_SRCS})

target_link_libraries(exec_graph_benchmark PUBLIC $<BUILD_INTERFACE:intf_pub>)

target_include_directories(exec_graph_benchmark PRIVATE
        ${AIR_CODE_DIR}/tests/ut/ge/runtime/v2
        ${AIR_CODE_DIR}/runtime/v2
        ${AIR_CODE_DIR}/inc/framework
        )

target_link_libraries(exec_graph_benchmark PUBLIC benchmark exe_graph error_manager
        slog_stub ascend_protobuf c_sec mmpa_stub -lrt -ldl
        )

set_target_properties(exec_graph_benchmark PROPERTIES CXX_STANDARD 17)
target_compile_options(exec_graph_benchmark PRIVATE -O2)