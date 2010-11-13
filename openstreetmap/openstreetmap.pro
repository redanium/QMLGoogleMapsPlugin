TEMPLATE = lib
CONFIG += plugin
TARGET = $$qtLibraryTarget(qtgeoservices_openstreetmap)
PLUGIN_TYPE=geoservices

QT += network

CONFIG += mobility
MOBILITY = location

HEADERS += \
            qgeomappingmanagerengine_openstreetmap.h \
            qgeomapreply_openstreetmap.h \
            qgeoserviceproviderplugin_openstreetmap.h

SOURCES += \
            qgeomappingmanagerengine_openstreetmap.cpp \
            qgeomapreply_openstreetmap.cpp \
            qgeoserviceproviderplugin_openstreetmap.cpp

symbian {
    TARGET.EPOCALLOWDLLDATA = 1
    TARGET.CAPABILITY = ALL -TCB
    pluginDep.sources = $${TARGET}.dll
    pluginDep.path = $${QT_PLUGINS_BASE_DIR}/$${PLUGIN_TYPE}
    DEPLOYMENT += pluginDep
}

