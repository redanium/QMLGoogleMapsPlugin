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

project(qtgeoservices_openstreetmap)

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


set (openstreetmap_SRCS
  qgeomappingmanagerengine_openstreetmap.cpp
  qgeomapreply_openstreetmap.cpp
  qgeoserviceproviderplugin_openstreetmap.cpp
)

set (openstreetmap_HDRS
  qgeomappingmanagerengine_openstreetmap.h
  qgeomapreply_openstreetmap.h
  qgeoserviceproviderplugin_openstreetmap.h
)

qt4_wrap_cpp(openstreetmap_MOCS ${openstreetmap_HDRS})

add_library (qtgeoservices_openstreetmap ${PLUGIN_TYPE} ${openstreetmap_SRCS} ${openstreetmap_MOCS})
target_link_libraries(qtgeoservices_openstreetmap ${QT_LIBRARIES})

install(TARGETS qtgeoservices_openstreetmap
  COMPONENT plugins
  LIBRARY DESTINATION ${PLUGINS_INSTALL_DIR}
  RUNTIME DESTINATION ${PLUGINS_INSTALL_DIR}
  ARCHIVE DESTINATION ${PLUGINS_INSTALL_DIR}
)
