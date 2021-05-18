target_sources(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/options_iot_hub.c
    ${CMAKE_CURRENT_LIST_DIR}/connection_iot_hub.c
    ${CMAKE_CURRENT_LIST_DIR}/connection_iot_hub.h
    )

set(MANIFEST_HELPER ${CMAKE_CURRENT_LIST_DIR}/manifest_helper.psm1 PARENT_SCOPE)
