TEMPLATE = lib
CONFIG += plugin
TARGET = $$qtLibraryTarget(qtgeoservices_google)
PLUGIN_TYPE=geoservices

include(../common.pri)

QT += network

CONFIG += mobility
MOBILITY = location

HEADERS += \
            qgeomappingmanagerengine_google.h \
            qgeomapreply_google.h \
            qgeoserviceproviderplugin_google.h

SOURCES += \
            qgeomappingmanagerengine_google.cpp \
            qgeomapreply_google.cpp \
            qgeoserviceproviderplugin_google.cpp

symbian {
    TARGET.EPOCALLOWDLLDATA = 1
    TARGET.CAPABILITY = ALL -TCB
    pluginDep.sources = $${TARGET}.dll
    pluginDep.path = $${QT_PLUGINS_BASE_DIR}/$${PLUGIN_TYPE}
    DEPLOYMENT += pluginDep
}

