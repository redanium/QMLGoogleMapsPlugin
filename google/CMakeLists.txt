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

project(qtgeoservices_google)

include_directories (
  ${CMAKE_CURRENT_BUILD_DIR}
  ${QT_MOBILITY_LOCATION_INCLUDE_DIR}
  ${QT_INCLUDE_DIR}
)

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

if (STATIC_PLUGINS)
  add_definitions(-DQT_STATICPLUGIN)
  set(PLUGIN_TYPE STATIC)
else()
  add_definitions(-DQT_SHARED)
  set(PLUGIN_TYPE SHARED)
endif()


set (google_SRCS
  qgeomappingmanagerengine_google.cpp
  qgeomapreply_google.cpp
  qgeoserviceproviderplugin_google.cpp
)

set (google_HDRS
  qgeomappingmanagerengine_google.h
  qgeomapreply_google.h
  qgeoserviceproviderplugin_google.h
)

qt4_wrap_cpp(google_MOCS ${lugdulov_HDRS})

add_library (qtgeoservices_google ${PLUGIN_TYPE} ${google_SRCS} ${google_MOCS})
target_link_libraries(qtgeoservices_google ${QT_LIBRARIES})

install(TARGETS qtgeoservices_google
  COMPONENT plugins
  LIBRARY DESTINATION ${PLUGINS_INSTALL_DIR}
  RUNTIME DESTINATION ${PLUGINS_INSTALL_DIR}
  ARCHIVE DESTINATION ${PLUGINS_INSTALL_DIR}
)
