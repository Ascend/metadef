project(ExeGraph)

file(GLOB_RECURSE EXE_GRAPH_SRCS CONFIGURE_DEPENDS "${METADEF_DIR}/exe_graph/*.cc")
add_library(exe_graph_static STATIC ${EXE_GRAPH_SRCS})  # for ut/st

target_include_directories(exe_graph_static PUBLIC
        ${METADEF_DIR}
        ${METADEF_DIR}/inc
        ${METADEF_DIR}/inc/external
        )

target_include_directories(exe_graph_static PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/proto/metadef_protos
        ${METADEF_DIR}
        ${METADEF_DIR}/inc
        ${METADEF_DIR}/inc/external
        #### yellow zone ####
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${METADEF_DIR}/../inc>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${METADEF_DIR}/../ops/built-in/op_proto/inc>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${METADEF_DIR}/../cann/ops/built-in/op_proto/inc>
        #### temp independent ####
        ${METADEF_DIR}/../graphengine/inc
        ${METADEF_DIR}/../graphengine/inc/framework
        ${METADEF_DIR}/../graphengine/inc/external
        #### temp in ge ####
        ${METADEF_DIR}/../inc
        ${METADEF_DIR}/../inc/framework
        ${METADEF_DIR}/../inc/external
        ${METADEF_DIR}/../../ops/built-in/op_proto/inc
        ${METADEF_DIR}/../../cann/ops/built-in/op_proto/inc
        #### temp in ge ####
        ${METADEF_DIR}/../../graphengine/inc
        ${METADEF_DIR}/../../graphengine/inc/framework
        ${METADEF_DIR}/../../graphengine/inc/external
        ${METADEF_DIR}/../../inc
        #### blue zone ####
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${ASCEND_DIR}/driver/include>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${ASCEND_DIR}/fwkacllib/include>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${METADEF_DIR}/../third_party/fwkacllib/inc/ops>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${METADEF_DIR}/../third_party/fwkacllib/inc>
        #### blue independent compile #####
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${METADEF_DIR}/third_party/graphengine/inc>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${METADEF_DIR}/third_party/graphengine/ge/inc>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${METADEF_DIR}/third_party/graphengine/inc/external>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${METADEF_DIR}/third_party/fwkacllib/inc>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${METADEF_DIR}/third_party/fwkacllib/inc/ops>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${METADEF_DIR}/third_party>
        ${METADEF_DIR}/third_party/transformer/inc
        )

target_link_libraries(exe_graph_static PRIVATE
        $<BUILD_INTERFACE:intf_pub>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:slog_headers>>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:mmpa_headers>>
        c_sec
        graph_static
        ascend_protobuf_static
        slog
        $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>:-lrt>
        -ldl
        )

target_compile_definitions(exe_graph_static PRIVATE
        google=ascend_private
        $<$<STREQUAL:${ENABLE_OPEN_SRC},True>:ONLY_COMPILE_OPEN_SRC>
        $<IF:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,OS_TYPE=WIN,OS_TYPE=0>
        $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:SECUREC_USING_STD_SECURE_LIB=0 NOMINMAX>
        )

target_compile_options(exe_graph_static PRIVATE
        $<$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>:-O2 -fPIC -Wextra -Wfloat-equal>
        $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Debug>>:/MTd>
        $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Release>>:/MT>
)

add_library(exe_graph SHARED ${EXE_GRAPH_SRCS})


target_include_directories(exe_graph PUBLIC
        ${METADEF_DIR}
        ${METADEF_DIR}/inc
        ${METADEF_DIR}/inc/external
        ${METADEF_DIR}/third_party/fwkacllib/inc
        ${METADEF_DIR}/third_party/graphengine/inc
        )

target_include_directories(exe_graph PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/proto/metadef_protos
        ${METADEF_DIR}
        ${METADEF_DIR}/inc
        ${METADEF_DIR}/inc/external
        #### yellow zone ####
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${METADEF_DIR}/../inc>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${METADEF_DIR}/../ops/built-in/op_proto/inc>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${METADEF_DIR}/../cann/ops/built-in/op_proto/inc>
        #### temp independent ####
        ${METADEF_DIR}/../graphengine/inc
        ${METADEF_DIR}/../graphengine/inc/framework
        ${METADEF_DIR}/../graphengine/inc/external
        #### temp in ge ####
        ${METADEF_DIR}/../inc
        ${METADEF_DIR}/../inc/framework
        ${METADEF_DIR}/../inc/external
        ${METADEF_DIR}/../../ops/built-in/op_proto/inc
        ${METADEF_DIR}/../../cann/ops/built-in/op_proto/inc
        #### temp in ge ####
        ${METADEF_DIR}/../../graphengine/inc
        ${METADEF_DIR}/../../graphengine/inc/framework
        ${METADEF_DIR}/../../graphengine/inc/external
        ${METADEF_DIR}/../../inc
        #### blue zone ####
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${ASCEND_DIR}/driver/include>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${ASCEND_DIR}/fwkacllib/include>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${METADEF_DIR}/../third_party/fwkacllib/inc/ops>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${METADEF_DIR}/../third_party/fwkacllib/inc>
        #### blue independent compile #####
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${METADEF_DIR}/third_party/graphengine/inc>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${METADEF_DIR}/third_party/graphengine/ge/inc>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${METADEF_DIR}/third_party/graphengine/inc/external>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${METADEF_DIR}/third_party/fwkacllib/inc>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${METADEF_DIR}/third_party/fwkacllib/inc/ops>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${METADEF_DIR}/third_party>
        ${METADEF_DIR}/third_party/transformer/inc
        )
target_link_libraries(exe_graph PUBLIC
        $<BUILD_INTERFACE:intf_pub>
        graph
        c_sec
        ascend_protobuf
        slog
        )

target_compile_definitions(exe_graph PRIVATE
        google=ascend_private
        )

target_compile_options(exe_graph PRIVATE -O2)

############ install ############
set(INSTALL_BASE_DIR "")
set(INSTALL_LIBRARY_DIR lib)

install(TARGETS exe_graph OPTIONAL
        LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
)