cmake_minimum_required(VERSION 2.6)

# collect all source files
file(GLOB_RECURSE PLUGIN_SOURCES *.cpp)
file(GLOB_RECURSE PLUGIN_HEADERS *.h *.hpp)
set(SOURCE_FILES ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})

message(STATUS "Plugin source files: " ${PLUGIN_SOURCES})
message(STATUS "Plugin header files: " ${PLUGIN_HEADERS})

# check if in Debug build
if (CMAKE_BUILD_TYPE EQUAL "Debug")
    message("*** DEBUG CONFIGURATION")
endif (CMAKE_BUILD_TYPE EQUAL "Debug")

# zterate over compatible Maya versions and try to build if SDK / Maya is available on the machine.
foreach(MAYA_VERSION ${MAYA_BUILD_VERSIONS})
    message(STATUS "Configure Version: " ${MAYA_VERSION})

    find_package(Maya REQUIRED)

    # ... if Maya version exists
    if(EXISTS ${MAYA_${MAYA_VERSION}_LOCATION})

        # create target
        set(MAYA_TARGET_NAME "maya${MAYA_VERSION}")
        add_library(${MAYA_TARGET_NAME} SHARED ${SOURCE_FILES})

        # link / include Maya for target
        target_include_directories(${MAYA_TARGET_NAME} PUBLIC ${MAYA_${MAYA_VERSION}_INCLUDE_DIR} include ../third_party/include)
        target_link_libraries(${MAYA_TARGET_NAME} ${MAYA_${MAYA_VERSION}_LIBRARIES} ${MAYA_${MAYA_VERSION}_LIBRARY})

        # set target output name / directory
        set(PLUGINS_OUTPUT_DIR "${PLUGINS_ROOT_DIRECTORY}/${MAYA_TARGET_NAME}")
        set_target_properties(${MAYA_TARGET_NAME} PROPERTIES PREFIX "" )
        set_target_properties(${MAYA_TARGET_NAME} PROPERTIES OUTPUT_NAME "hyperdrive" )
        set_target_properties(${MAYA_TARGET_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PLUGINS_OUTPUT_DIR})

        install(TARGETS ${MAYA_TARGET_NAME} LIBRARY DESTINATION ${PLUGINS_OUTPUT_DIR})
        message(STATUS "Successfully configured build for Maya: " ${MAYA_${MAYA_VERSION}_LOCATION})
    else()
        message(WARNING "Skip configuring target. Maya ${MAYA_VERSION} not found at path: " ${MAYA_${MAYA_VERSION}_LOCATION})
    endif()
endforeach()

message( STATUS "*** MAYA INSTUCTIONS: Set environment variable 'MAYA_MODULE_PATH' for Maya to include: " ${REPO_ROOT_DIRECTORY})
