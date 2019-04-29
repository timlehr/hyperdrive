cmake_minimum_required(VERSION 2.6)
project(hyperdrive)

set(REPO_ROOT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(BUILD_ROOT_DIRECTORY "${REPO_ROOT_DIRECTORY}/build")
set(PLUGINS_ROOT_DIRECTORY "${REPO_ROOT_DIRECTORY}/plugins")

set(CMAKE_MODULE_PATH "${REPO_ROOT_DIRECTORY}/cmake_modules")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${BUILD_ROOT_DIRECTORY}")

# Maya versions to build Hyperdrive for
set(MAYA_BUILD_VERSIONS 2018 2017)

add_subdirectory(src)