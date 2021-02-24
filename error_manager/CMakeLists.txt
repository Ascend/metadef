set(SRC_LIST
    "error_manager.cc"
)

############ liberror_manager.so ############
add_library(error_manager SHARED ${SRC_LIST})

target_compile_options(error_manager PRIVATE
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>:-Werror -fno-common>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Debug>>:/MTd>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Release>>:/MT>
)

target_include_directories(error_manager PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${TOP_DIR}
    ${TOP_DIR}/inc
    ${TOP_DIR}/metadef/inc
    ${TOP_DIR}/graphengine/inc
    ${TOP_DIR}/inc/external
    ${TOP_DIR}/metadef/inc/external
    ${TOP_DIR}/graphengine/inc/external
)

target_link_libraries(error_manager PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    static_mmpa
    -Wl,--no-as-needed
    c_sec
    slog
    -Wl,--as-needed
    json
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>:-lrt>
    -ldl
)

############ liberror_manager.a ############
add_library(error_manager_static STATIC ${SRC_LIST})

target_include_directories(error_manager_static PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${TOP_DIR}
    ${TOP_DIR}/inc
    ${TOP_DIR}/metadef/inc
    ${TOP_DIR}/graphengine/inc
    ${TOP_DIR}/inc/external
    ${TOP_DIR}/metadef/inc/external
    ${TOP_DIR}/graphengine/inc/external
)

target_compile_definitions(error_manager_static PRIVATE
    LOG_CPP
)

target_compile_options(error_manager_static PRIVATE
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>:-Werror -fno-common>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Debug>>:/MTd>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Release>>:/MT>
)

target_link_libraries(error_manager_static PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    json
    c_sec
)

set_target_properties(error_manager_static PROPERTIES
    OUTPUT_NAME error_manager
)

############ install ############
set(INSTALL_BASE_DIR "")
set(INSTALL_LIBRARY_DIR lib)

install(TARGETS error_manager error_manager_static OPTIONAL
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
    ARCHIVE DESTINATION ${INSTALL_LIBRARY_DIR}
)
