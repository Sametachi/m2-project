include(GitWc)

find_package(Git REQUIRED)
GIT_WC_INFO(${CMAKE_CURRENT_LIST_DIR} V_VERSION)

# Get git branch
string(FIND ${V_VERSION_WC_REVISION_NAME} " " TMP_AFTER_SPACE)
string(LENGTH V_VERSION_WC_VERSION_NAME TMP_LENGTH)
math(EXPR TMP_LENGTH "${TMP_LENGTH} - ${TMP_AFTER_SPACE}")
string(SUBSTRING ${V_VERSION_WC_REVISION_NAME} ${TMP_AFTER_SPACE} ${TMP_LENGTH} V_VERSION_WC_BRANCH)
string(STRIP ${V_VERSION_WC_BRANCH} V_VERSION_WC_BRANCH)

# Get number of commits after first one
execute_process(COMMAND ${GIT_EXECUTABLE} rev-list --count ${V_VERSION_WC_BRANCH} WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR} OUTPUT_VARIABLE V_VERSION_WC_REVISION RESULT_VARIABLE REV_LIST_RES)

if (NOT "${REV_LIST_RES}" STREQUAL "0")
    message(FATAL_ERROR "Cannot get git revision")
endif()

string(STRIP ${V_VERSION_WC_REVISION} V_VERSION_WC_REVISION)

# Set revision for client and server
math(EXPR V_VERSION_WC_REVISION_CLIENT "${CLIENT_BASE_REVISION} + ${V_VERSION_WC_REVISION}")
math(EXPR V_VERSION_WC_REVISION_SERVER "${SERVER_BASE_REVISION} + ${V_VERSION_WC_REVISION}")

# Configure files
configure_file(${CMAKE_BASE_PATH}/ver/Version.h.in ${CMAKE_BINARY_DIR}/generated/Version.h @ONLY)
configure_file(${CMAKE_BASE_PATH}/ver/GitVersion.h.in ${CMAKE_BINARY_DIR}/generated/GitVersion.h @ONLY)

macro(LinkVersion PROJECT)
    target_include_directories(${PROJECT} PRIVATE ${CMAKE_BINARY_DIR}/generated)
endmacro()
