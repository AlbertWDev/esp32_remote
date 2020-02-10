
set(COMPONENT_REQUIRES esp_https_server json spi_flash tcpip_adapter wifimanager app_update)

set(COMPONENT_ADD_INCLUDEDIRS ".")
set(COMPONENT_SRCDIRS "." "endpoints")

######################################################
###    ADD ALL ENDPOINTS DIRECTORIES AS SOURCES    ###
######################################################
FILE(GLOB_RECURSE ENDPOINT_DIRS 
    LIST_DIRECTORIES true
    RELATIVE ${COMPONENT_PATH}
    ${COMPONENT_PATH}/endpoints/*)
FOREACH(ENDPOINT_DIR ${ENDPOINT_DIRS})
    # Check if this directory contains SRC files
    FILE(GLOB src_files "${COMPONENT_PATH}/${ENDPOINT_DIR}/*.c")
    IF(IS_DIRECTORY "${COMPONENT_PATH}/${ENDPOINT_DIR}" AND src_files)
        #message(STATUS "Remote management endpoint: ${ENDPOINT_DIR}")
        LIST(APPEND COMPONENT_SRCDIRS ${ENDPOINT_DIR})
    ENDIF()
ENDFOREACH()


register_component()
