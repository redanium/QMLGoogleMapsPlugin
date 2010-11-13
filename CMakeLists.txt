# Copyright (C) 2009-2010 Corentin Chary <corentin.chary@gmail.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public License
# along with this library; see the file COPYING.LIB.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301, USA.

PROJECT(qtm-geoservices-extras)

#set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII" )

# Stop cmake 2.6 from whining
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
IF(COMMAND cmake_policy)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND cmake_policy)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")

# Options
option(STATIC_PLUGINS "build static plugins [default: off]" OFF)

# compile in debug mode
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

IF("${CMAKE_BUILD_TYPE}" MATCHES "^Rel.*")
  ADD_DEFINITIONS("-DQT_NO_DEBUG_OUTPUT")
  ADD_DEFINITIONS("-DQT_NO_DEBUG_OUTPUT")
ENDIF("${CMAKE_BUILD_TYPE}" MATCHES "^Rel.*")

# Find Qt4
FIND_PACKAGE( Qt4 REQUIRED )
FIND_PACKAGE( QtMobility REQUIRED)

#add extra search paths for libraries and includes
SET (PLUGINS_INSTALL_DIR "plugins/geoservices/" CACHE STRING "Directory where lib will install")

set(QTM_GEOSERVICES_EXTRA_MAJOR_VERSION "0")
set(QTM_GEOSERVICES_EXTRA_MINOR_VERSION "1")
set(QTM_GEOSERVICES_EXTRA_PATCH_VERSION "0")

set(QTM_GEOSERVICES_EXTRA_VERSION_STRING "${QTM_GEOSERVICES_EXTRA_MAJOR_VERSION}.${QTM_GEOSERVICES_EXTRA_MINOR_VERSION}.${QTM_GEOSERVICES_EXTRA_PATCH_VERSION}")

# Include the cmake file needed to use qt4
set(QT_USE_QTGUI TRUE)
set(QT_USE_QTNETWORK TRUE)
INCLUDE( ${QT_USE_FILE} )

# Subdirs
add_subdirectory(google)
add_subdirectory(openstreetmap)

# CPack
set(CPACK_VERSION_MAJOR ${QTM_GEOSERVICES_EXTRA_MAJOR_VERSION})
set(CPACK_VERSION_MINOR ${QTM_GEOSERVICES_EXTRA_MINOR_VERSION})
set(CPACK_VERSION_PATCH ${QTM_GEOSERVICES_EXTRA_PATCH_VERSION})
set(CPACK_VERSION ${QTM_GEOSERVICES_EXTRA_VERSION_STRING})
set(CPACK_PACKAGE_VERSION ${QTM_GEOSERVICES_EXTRA_VERSION_STRING})
set(CPACK_PACKAGE_CONTACT "Corentin Chary <corentin.chary@gmail.com>")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Google and Open Street Map plugins for Qt Mobility Location API.")
set(CPACK_PACKAGE_VENDOR "http://xf.iksaif.net/dev/qtm-geoservices-extras.html")
set(CPACK_SOURCE_IGNORE_FILES
  "~$"
  "^${PROJECT_SOURCE_DIR}/\\\\.git"
  "^${PROJECT_SOURCE_DIR}/build/")
#set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_CURRENT_BUILD_DIR};qttvdb;ALL;/")
set(CPACK_COMPONENTS_ALL plugins)
set(CPACK_COMPONENT_PLUGINS_DISPLAY_NAME "Plugins")
set(CPACK_COMPONENT_PLUGINS_DESCRIPTION "Google and Open Street Map plugins for Qt Mobility Location API.")
set(CPACK_PACKAGE_FILE_NAME "qtm-geoservices-extras-${CPACK_VERSION}-${CMAKE_SYSTEM_NAME}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "qtm-geoservices-extras-${CPACK_VERSION}")

if(NOT WIN32)
  set(CPACK_SOURCE_GENERATOR "TGZ;TBZ2")
else()
  set(CPACK_GENERATOR "ZIP;NSIS")
  set(CPACK_SOURCE_GENERATOR "ZIP")
  set(CPACK_BINARY_CYGWIN "OFF")
  set(CPACK_SOURCE_CYGWIN "OFF")
  if (STATIC_PLUGINS)
    set(CPACK_PACKAGE_FILE_NAME "qtm-geoservices-extras-${CPACK_VERSION}-static-win32")
  else ()
    set(CPACK_PACKAGE_FILE_NAME "qtm-geoservices-extras-${CPACK_VERSION}-win32")
  endif()
  set(CPACK_BINARY_NSIS "ON")
  set(CPACK_NSIS_MODIFY_PATH "OFF")
  set(CPACK_NSIS_DISPLAY_NAME "Qt Mobility Extras Geoservices")
  set(CPACK_NSIS_HELP_LINK "http://xf.iksaif.net/dev/qtm-geoservices-extras.html")
  set(CPACK_NSIS_URL_INFO_ABOUT "http://xf.iksaif.net/dev/qtm-geoservices-extras.html")
  set(CPACK_NSIS_CONTACT "Corentin Chary <corentin.chary@gmail.com>")
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "qtm-geoservices-extras")
endif()

include(CPack)
