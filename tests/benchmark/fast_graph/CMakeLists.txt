set(CMAKE_CXX_STANDARD 11)

set(BENCHMARK_TEST  "compute_graph_base_unittest.cc")

add_executable(fast_graph_benchmark ${BENCHMARK_TEST})

target_include_directories(fast_graph_benchmark PRIVATE
        ${AIR_CODE_DIR}/tests/ut/ge/runtime/v2
        ${AIR_CODE_DIR}/runtime/v2
        ${AIR_CODE_DIR}/inc/framework
        ${METADEF_DIR}/inc
        ${METADEF_DIR}/inc/external
        ${METADEF_DIR}/third_party/inc
        ${METADEF_DIR}/third_party/inc/external
        ${CMAKE_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/proto/metadef_protos
        ${CMAKE_BINARY_DIR}/proto/ge
        ${CMAKE_BINARY_DIR}/proto/ge/proto
        )

target_compile_definitions(fast_graph_benchmark PRIVATE
        google=ascend_private
        FUNC_VISIBILITY
        )

set_target_properties(fast_graph_benchmark PROPERTIES CXX_STANDARD 11)

target_compile_options(fast_graph_benchmark PRIVATE -O2 -std=c++11)


target_link_libraries(fast_graph_benchmark  PRIVATE benchmark
        intf_pub
        metadef_headers
        -Wl,--no-as-needed
        graph
        graph_base
        c_sec
        error_manager
        slog
        ascend_protobuf
        $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>:-lrt>
        -ldl
)


