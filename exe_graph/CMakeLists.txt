project(ExeGraph)

file(GLOB_RECURSE EXE_GRAPH_SRCS CONFIGURE_DEPENDS "${METADEF_DIR}/exe_graph/*.cc")
add_library(exe_graph_static STATIC ${EXE_GRAPH_SRCS})  # for ut/st

target_include_directories(exe_graph_static PUBLIC
        ${METADEF_DIR}
        ${METADEF_DIR}/inc
        ${METADEF_DIR}/inc/external
        ${METADEF_DIR}/third_party/fwkacllib/inc
        ${METADEF_DIR}/third_party/graphengine/inc
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
        #### blue zone ####
        ${PROTOBUF_SHARED_PKG_DIR}/include
        #### yellow zone ####
        ${ASCEND_PROTOBUF_SHARED_PKG_DIR}/include
        )
target_link_libraries(exe_graph_static PUBLIC
        $<BUILD_INTERFACE:intf_pub>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:slog_headers>>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:mmpa_headers>>
        c_sec
        graph_static
        $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>:-lrt>
        -ldl
        )

target_compile_definitions(exe_graph_static PRIVATE
        google=ascend_private
        )

add_library(exe_graph SHARED ${EXE_GRAPH_SRCS})

target_include_directories(exe_graph PUBLIC
        ${METADEF_DIR}
        ${METADEF_DIR}/inc
        ${METADEF_DIR}/inc/external
        )
target_link_libraries(exe_graph PUBLIC
        $<BUILD_INTERFACE:intf_pub>
        graph
        )

target_compile_definitions(exe_graph_static PRIVATE
        google=ascend_private
        )

target_compile_options(exe_graph PRIVATE -O2)