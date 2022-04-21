project(ExeGraphUT)

file(GLOB_RECURSE TEST_SRCS CONFIGURE_DEPENDS "${METADEF_DIR}/tests/ut/exe_graph/*.cc")
file(GLOB_RECURSE FAKER_SRCS CONFIGURE_DEPENDS "${METADEF_DIR}/tests/depends/faker/*.cc")

add_executable(ut_exe_graph ${TEST_SRCS} ${FAKER_SRCS})

target_include_directories(ut_exe_graph PUBLIC
        ${METADEF_DIR}/tests/depends
        )

target_compile_options(ut_exe_graph PRIVATE
        -g --coverage -fprofile-arcs -ftest-coverage
        )

target_compile_definitions(ut_exe_graph PRIVATE
        google=ascend_private
        )

target_compile_options(exe_graph_static PRIVATE  -g --coverage -fprofile-arcs -ftest-coverage)

target_link_libraries(ut_exe_graph
        $<BUILD_INTERFACE:intf_pub>
        graph_static error_manager_static exe_graph_static
        gtest gtest_main slog_stub ascend_protobuf c_sec mmpa_stub -lrt -ldl -lgcov
        )
