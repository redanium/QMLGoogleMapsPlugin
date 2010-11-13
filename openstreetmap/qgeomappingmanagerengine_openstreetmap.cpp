/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qgeomappingmanagerengine_openstreetmap.h"
#include "qgeomapreply_openstreetmap.h"

#include <qgeotiledmaprequest.h>

#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QNetworkProxy>
#include <QSize>
#include <QDir>
#include <QDateTime>

#include <QDebug>

#define LARGE_TILE_DIMENSION 256

// TODO: Tweak the max size or create something better
#if defined(Q_OS_SYMBIAN)
#define DISK_CACHE_MAX_SIZE 10*1024*1024  //10MB
#else
#define DISK_CACHE_MAX_SIZE 50*1024*1024  //50MB
#endif

#if defined(Q_OS_SYMBIAN) || defined(Q_OS_WINCE_WM) || defined(Q_WS_MAEMO_5) || defined(Q_WS_MAEMO_6)
#undef DISK_CACHE_ENABLED
#else
#define DISK_CACHE_ENABLED 1
#endif

QGeoMappingManagerEngineOpenStreetMap::QGeoMappingManagerEngineOpenStreetMap(const QMap<QString, QVariant> &parameters, QGeoServiceProvider::Error *error, QString *errorString)
        : QGeoTiledMappingManagerEngine(parameters),
	  m_servers(QStringList("http://tile.openstreetmap.org/"))
{
    Q_UNUSED(error)
    Q_UNUSED(errorString)

    setTileSize(QSize(256, 256));
    setMinimumZoomLevel(0.0);
    setMaximumZoomLevel(17.0);

    QList<QGraphicsGeoMap::MapType> types;
    types << QGraphicsGeoMap::StreetMap;
    setSupportedMapTypes(types);

    QList<QGraphicsGeoMap::ConnectivityMode> modes;
    modes << QGraphicsGeoMap::OnlineMode;
    setSupportedConnectivityModes(modes);

    m_networkManager = new QNetworkAccessManager(this);

    if (parameters.contains("mapping.proxy")) {
        QString proxy = parameters.value("mapping.proxy").toString();
	qint16 port = 8080;
	int sep;

	if ((sep = proxy.indexOf(":")) != -1) {
	  port = proxy.mid(sep + 1).toInt();
	  proxy = proxy.left(sep);
	}

        if (!proxy.isEmpty() && port) {
            m_networkManager->setProxy(QNetworkProxy(QNetworkProxy::HttpProxy, proxy, port));
	}
    }

    if (parameters.contains("mapping.servers") || parameters.contains("mapping.server")) {
	QVariant servers;

	if (parameters.contains("mapping.servers"))
	    servers = parameters.value("mapping.servers");
	else
	    servers =  parameters.value("mapping.server");

	if (!servers.toStringList().isEmpty()) {
	    m_servers = servers.toStringList();
	} else if (!servers.toString().isEmpty()) {
	    m_servers = QStringList(servers.toString());
	}
    }

#ifdef DISK_CACHE_ENABLED
    m_cache = new QNetworkDiskCache(this);

    QDir dir = QDir::temp();
    dir.mkdir("maptiles");
    dir.cd("maptiles");

    m_cache->setCacheDirectory(dir.path());

    if (parameters.contains("mapping.cache.directory")) {
        QString cacheDir = parameters.value("mapping.cache.directory").toString();
        if (!cacheDir.isEmpty())
            m_cache->setCacheDirectory(cacheDir);
    }

    if (parameters.contains("mapping.cache.size")) {
        bool ok = false;
        qint64 cacheSize = parameters.value("mapping.cache.size").toString().toLongLong(&ok);
        if (ok)
            m_cache->setMaximumCacheSize(cacheSize);
    }

    if (m_cache->maximumCacheSize() > DISK_CACHE_MAX_SIZE)
        m_cache->setMaximumCacheSize(DISK_CACHE_MAX_SIZE);

    m_networkManager->setCache(m_cache);
#endif
}

QGeoMappingManagerEngineOpenStreetMap::~QGeoMappingManagerEngineOpenStreetMap() {}

QGeoMapData* QGeoMappingManagerEngineOpenStreetMap::createMapData()
{
    QGeoMapData *data = QGeoTiledMappingManagerEngine::createMapData();
    if (!data)
        return 0;

    data->setConnectivityMode(QGraphicsGeoMap::OnlineMode);
    return data;
}

QGeoTiledMapReply* QGeoMappingManagerEngineOpenStreetMap::getTileImage(const QGeoTiledMapRequest &request)
{
    // TODO add error detection for if request.connectivityMode() != QGraphicsGeoMap::OnlineMode
    QString rawRequest = getRequestString(request);

    QNetworkRequest netRequest((QUrl(rawRequest))); // The extra pair of parens disambiguates this from a function declaration
    netRequest.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);

#ifdef DISK_CACHE_ENABLED
    netRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    m_cache->metaData(netRequest.url()).setLastModified(QDateTime::currentDateTime());
#endif

    QNetworkReply* netReply = m_networkManager->get(netRequest);

    QGeoTiledMapReply* mapReply = new QGeoMapReplyOpenStreetMap(netReply, request);

    // TODO goes badly on linux
    //qDebug() << "request: " << QString::number(reinterpret_cast<int>(mapReply), 16) << " " << request.row() << "," << request.column();
    // this one might work better. It follows defined behaviour, unlike reinterpret_cast
    //qDebug("request: %p %i,%i @ %i", mapReply, request.row(), request.column(), request.zoomLevel());
    return mapReply;
}

QString QGeoMappingManagerEngineOpenStreetMap::getRequestString(const QGeoTiledMapRequest &request) const
{
    static const QChar dot('.');
    static const QChar slash('/');
    static const QString dotpng(".png");

    QString requestString;
    requestString += m_servers.at((request.row() + request.column()) % m_servers.size());
    requestString += QString::number(request.zoomLevel());
    requestString += slash;
    requestString += QString::number(request.column());
    requestString += slash;
    requestString += QString::number(request.row());
    requestString += dotpng;

    return requestString;
}
