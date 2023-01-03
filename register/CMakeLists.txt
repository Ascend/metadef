include(${METADEF_DIR}/cmake/build_type.cmake)
set(SRC_LIST
    "register.cpp"
    "register_pass.cpp"
    "prototype_pass_registry.cc"
    "ops_kernel_builder_registry.cc"
    "graph_optimizer/fusion_common/op_slice_info.cc"
    "graph_optimizer/fusion_common/fusion_turbo_utils.cc"
    "graph_optimizer/fusion_common/fusion_turbo.cc"
    "graph_optimizer/graph_fusion/graph_fusion_pass_base.cc"
    "graph_optimizer/graph_fusion/connection_matrix.cc"
    "graph_optimizer/graph_fusion/fusion_pass_registry.cc"
    "graph_optimizer/graph_fusion/fusion_pattern.cc"
    "graph_optimizer/graph_fusion/pattern_fusion_base_pass.cc"
    "graph_optimizer/graph_fusion/graph_pass_util.cc"
    "graph_optimizer/graph_fusion/pattern_fusion_base_pass_impl.cc"
    "graph_optimizer/graph_fusion/pattern_fusion_base_pass_impl.h"
    "graph_optimizer/buffer_fusion/buffer_fusion_pass_registry.cc"
    "graph_optimizer/buffer_fusion/buffer_fusion_pass_base.cc"
    "graph_optimizer/buffer_fusion/buffer_fusion_pattern.cc"
    "graph_optimizer/fusion_statistic/fusion_statistic_recorder.cc"
    "op_kernel_registry.cpp"
    "auto_mapping_util.cpp"
    "ffts_plus_update_manager.cc"
    "ffts_node_converter_registry.cc"
    "ffts_node_calculater_registry.cc"
    "host_cpu_context.cc"
    "tensor_assign.cpp"
    "infer_data_slice_registry.cc"
    "infer_axis_slice_registry.cc"
    "kernel_register_data.cc"
    "kernel_registry_impl.cc"
    "node_converter_registry.cc"
    "op_impl_registry.cc"
    "op_impl_space_registry.cc"
    "op_impl_registry_holder_manager.cc"
    "scope/scope_graph.cc"
    "scope/scope_pass.cc"
    "scope/scope_pattern.cc"
    "scope/scope_util.cc"
    "scope/scope_pass_registry.cc"
    "shape_inference.cc"
    "tik2/tik2_py.cc"
    "tik2/op_check.cc"
    "${METADEF_DIR}/exe_graph/runtime/kernel_run_context_builder.cc"
    "${METADEF_DIR}/exe_graph/lowering/bg_kernel_context_extend.cc"
    "${METADEF_DIR}/exe_graph/lowering/buffer_pool.cc"
    "${METADEF_DIR}/exe_graph/runtime/compute_node_info.cc"
    "${METADEF_DIR}/exe_graph/lowering/bg_ir_attrs.cc"
)

############ libregister.so ############
add_library(register SHARED
    ${SRC_LIST}
    $<TARGET_OBJECTS:metadef_tensorflow_protos_obj>
)

target_compile_options(register PRIVATE
    $<$<OR:$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>,$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>: -fno-common -Wextra -Wfloat-equal>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:/utf-8>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Debug>>:/MTd>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Release>>:/MT>
)

target_compile_definitions(register PRIVATE
    google=ascend_private
    $<IF:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,OS_TYPE=WIN,OS_TYPE=0>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:SECUREC_USING_STD_SECURE_LIB=0 NOMINMAX>
    $<$<STREQUAL:${ENABLE_OPEN_SRC},True>:ONLY_COMPILE_OPEN_SRC>
)

target_include_directories(register PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${METADEF_DIR}
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/proto/metadef_protos
)

target_link_options(register PRIVATE
    -Wl,-Bsymbolic
)

target_link_libraries(register
    PRIVATE
        intf_pub
        msprof_headers
        cce_headers
        runtime_headers
        air_headers
        -Wl,--whole-archive
        op_tiling_o2
        -Wl,--no-whole-archive
        -Wl,--no-as-needed
        ascend_protobuf
        c_sec
        slog
        platform
        graph
        static_mmpa
        -Wl,--as-needed
        json
    PUBLIC
        metadef_headers
)

############ libregister.a ############
add_library(register_static STATIC
    ${SRC_LIST}
    $<TARGET_OBJECTS:metadef_tensorflow_protos_obj>
    "op_tiling/op_tiling.cc"
    "op_tiling/op_tiling_info.cc"
    "op_tiling/op_tiling_utils.cc"
    "op_tiling/op_tiling_attr_utils.cc"
    "op_tiling/op_compile_info_manager.cc"
    "op_tiling/op_tiling_registry.cc"
    "op_tiling/op_tiling_py.cc"
)

target_compile_options(register_static PRIVATE
    $<$<OR:$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>,$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>: -fno-common -Wextra -Wfloat-equal>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:/utf-8>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Debug>>:/MTd>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Release>>:/MT>
)

target_compile_definitions(register_static PRIVATE
    google=ascend_private
    $<IF:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,OS_TYPE=WIN,OS_TYPE=0>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:SECUREC_USING_STD_SECURE_LIB=0 NOMINMAX>
    $<$<STREQUAL:${ENABLE_OPEN_SRC},True>:ONLY_COMPILE_OPEN_SRC>
    LOG_CPP
)

target_include_directories(register_static PUBLIC
    ${METADEF_DIR}/graph
    ${METADEF_DIR}/inc
    ${METADEF_DIR}/inc/external
    ${METADEF_DIR}/inc/register
)

target_include_directories(register_static PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${METADEF_DIR}
    ${METADEF_DIR}/..
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/proto/metadef_protos
)

target_link_libraries(register_static PRIVATE
    c_sec
    json
    intf_pub
    slog_headers
    msprof_headers
    mmpa_headers
    cce_headers
    air_headers
    runtime_headers
    metadef_headers
)

target_link_libraries(register_static PRIVATE
        ascend_protobuf_static
        )

set_target_properties(register_static PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS TRUE
    OUTPUT_NAME $<IF:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,libregister,register>
)

############ libop_tiling_o2.a ############
add_library(op_tiling_o2 STATIC
    "op_tiling/op_tiling.cc"
    "op_tiling/op_tiling_info.cc"
    "op_tiling/op_tiling_utils.cc"
    "op_tiling/op_tiling_attr_utils.cc"
    "op_tiling/op_compile_info_manager.cc"
    "op_tiling/op_tiling_registry.cc"
    "op_tiling/op_tiling_py.cc")

add_dependencies(op_tiling_o2
    metadef_protos
)

target_include_directories(op_tiling_o2 PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${METADEF_DIR}
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/proto/metadef_protos
)

target_compile_options(op_tiling_o2 PRIVATE
    -O2
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Debug>>:/MTd>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Release>>:/MT>
    $<$<OR:$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>,$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>: -fno-common -Wextra -Wfloat-equal>
)

target_compile_definitions(op_tiling_o2 PRIVATE
    $<IF:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,OS_TYPE=WIN,OS_TYPE=0>
    $<$<STREQUAL:${ENABLE_OPEN_SRC},True>:ONLY_COMPILE_OPEN_SRC>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:SECUREC_USING_STD_SECURE_LIB=0 NOMINMAX>
    LOG_CPP
)

target_link_libraries(op_tiling_o2 PRIVATE
    intf_pub
    slog_headers
    mmpa_headers
    metadef_headers
    air_headers
    ascend_protobuf
    json
    c_sec
)

############ librt2_registry.a ############
add_library(rt2_registry_static STATIC
    "op_impl_registry.cc"
)

target_compile_options(rt2_registry_static PRIVATE
    $<$<OR:$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>,$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>: -fno-common -O2 -Werror -Wextra -Wfloat-equal>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:/utf-8>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Debug>>:/MTd>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Release>>:/MT>
)

target_compile_definitions(rt2_registry_static PRIVATE
    $<IF:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,OS_TYPE=WIN,OS_TYPE=0>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:SECUREC_USING_STD_SECURE_LIB=0 NOMINMAX>
    $<$<STREQUAL:${ENABLE_OPEN_SRC},True>:ONLY_COMPILE_OPEN_SRC>
)

target_include_directories(rt2_registry_static PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${METADEF_DIR}/inc
    ${METADEF_DIR}/inc/external
    ${METADEF_DIR}/third_party/graphengine/inc
    ${METADEF_DIR}/third_party/graphengine/inc/external
    ${METADEF_DIR}/third_party/fwkacllib/inc
)

set_target_properties(rt2_registry_static PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS TRUE
    OUTPUT_NAME $<IF:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,librt2_registry,rt2_registry>
)

target_link_libraries(rt2_registry_static
    PRIVATE
    c_sec
    PUBLIC
    metadef_headers
)

############ install ############
install(TARGETS register_static rt2_registry_static
    ARCHIVE DESTINATION ${INSTALL_LIBRARY_DIR} OPTIONAL
)
