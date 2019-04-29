# Copyright 2017 Chad Vernon
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
# associated documentation files (the "Software"), to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge, publish, distribute,
# sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all copies or
# substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
# NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#.rst:
# FindMaya
# --------
#
# Find Maya headers and libraries.
#
# Imported targets
# ^^^^^^^^^^^^^^^^
#
# This module defines the following :prop_tgt:`IMPORTED` target:
#
# ``Maya::Maya``
#   The Maya libraries, if found.
#
# Result variables
# ^^^^^^^^^^^^^^^^
#
# This module will set the following variables in your project:
#
# ``Maya_FOUND``
#   Defined if a Maya installation has been detected
# ``MAYA_INCLUDE_DIR``
#   Where to find the headers (maya/MFn.h)
# ``MAYA_LIBRARIES``
#   All the Maya libraries.
#

# CUSTOMIZED FOR HYPERDRIVE NEEDS
# THIS CMAKE SCRIPT IS MISSING BIG CHUNKS.
# FOR YOUR OWN PLUGIN, PLEASE USE THE ORIGINAL SCRIPT BY CHAD VERNON.
# UNMODIFIED FORK: https://github.com/timlehr/cgcmake

# OS Specific environment setup
set(MAYA_COMPILE_DEFINITIONS "REQUIRE_IOSTREAM;_BOOL")
set(MAYA_INSTALL_BASE_SUFFIX "")
set(MAYA_LIB_SUFFIX "lib")
set(MAYA_TARGET_TYPE LIBRARY)

if(WIN32)
    # Windows
    set(MAYA_INSTALL_BASE_DEFAULT "C:/Program Files/Autodesk")
    set(MAYA_COMPILE_DEFINITIONS "${MAYA_COMPILE_DEFINITIONS};NT_PLUGIN")
    set(OPENMAYA OpenMaya.lib)
    set(MAYA_PLUGIN_EXTENSION ".mll")
    set(MAYA_TARGET_TYPE RUNTIME)
elseif(APPLE)
    # Apple
    set(MAYA_INSTALL_BASE_DEFAULT /Applications/Autodesk)
    set(OPENMAYA libOpenMaya.dylib)
    set(MAYA_LIB_SUFFIX "Maya.app/Contents/MacOS")
    set(MAYA_COMPILE_DEFINITIONS "${MAYA_COMPILE_DEFINITIONS};OSMac_")
    set(MAYA_PLUGIN_EXTENSION ".bundle")
else()
    # Linux
    set(MAYA_COMPILE_DEFINITIONS "${MAYA_COMPILE_DEFINITIONS};LINUX")
    set(MAYA_INSTALL_BASE_DEFAULT /usr/autodesk)
    set(OPENMAYA libOpenMaya.so)
    if(MAYA_VERSION LESS 2016)
        # Pre Maya 2016 on Linux
        set(MAYA_INSTALL_BASE_SUFFIX -x64)
    endif()
    set(MAYA_PLUGIN_EXTENSION ".so")
endif()

set(MAYA_INSTALL_BASE_PATH ${MAYA_INSTALL_BASE_DEFAULT} CACHE STRING
    "Root path containing your maya installations, e.g. /usr/autodesk or /Applications/Autodesk/")

set(MAYA_${MAYA_VERSION}_LOCATION ${MAYA_INSTALL_BASE_PATH}/maya${MAYA_VERSION}${MAYA_INSTALL_BASE_SUFFIX})

# Maya include directory
find_path(MAYA_${MAYA_VERSION}_INCLUDE_DIR maya/MFn.h
    PATHS
        ${MAYA_${MAYA_VERSION}_LOCATION}
        $ENV{MAYA_LOCATION}
    PATH_SUFFIXES
        "include/"
        "devkit/include/"
)

find_library(MAYA_${MAYA_VERSION}_LIBRARY 
    NAMES    
        ${OPENMAYA}
    PATHS
        ${MAYA_${MAYA_VERSION}_LOCATION}
        $ENV{MAYA_${MAYA_VERSION}_LOCATION}
    PATH_SUFFIXES
        "${MAYA_LIB_SUFFIX}/"
    DOC "Maya library path"
    NO_DEFAULT_PATH
)

# Maya libraries
set(_MAYA_LIBRARIES OpenMaya OpenMayaAnim OpenMayaFX OpenMayaRender OpenMayaUI Foundation)
foreach(MAYA_LIB ${_MAYA_LIBRARIES})
    find_library(MAYA_${MAYA_VERSION}_${MAYA_LIB}_LIBRARY 
    NAMES 
        ${MAYA_LIB} 
    PATHS 
        ${MAYA_${MAYA_VERSION}_LOCATION}
        $ENV{MAYA_${MAYA_VERSION}_LOCATION}
    PATH_SUFFIXES
        "${MAYA_LIB_SUFFIX}/"
    NO_DEFAULT_PATH)
    set(MAYA_${MAYA_VERSION}_LIBRARIES ${MAYA_${MAYA_VERSION}_LIBRARIES} ${MAYA_${MAYA_VERSION}_${MAYA_LIB}_LIBRARY})
endforeach()

function(MAYA_PLUGIN _target)
    if (WIN32)
        set_target_properties(${_target} PROPERTIES
            LINK_FLAGS "/export:initializePlugin /export:uninitializePlugin")
    endif()
    set_target_properties(${_target} PROPERTIES
        COMPILE_DEFINITIONS "${MAYA_COMPILE_DEFINITIONS}"
        PREFIX ""
        SUFFIX ${MAYA_PLUGIN_EXTENSION})
endfunction()
