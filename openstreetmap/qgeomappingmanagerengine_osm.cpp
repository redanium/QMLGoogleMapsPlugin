/****************************************************************************
**
** Copyright (C) 2011 Corentin Chary
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Corentin Chary (corentin.chary@gmail.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qgeomappingmanagerengine_osm.h"
#include "qgeomapreply_osm.h"
#include "parseproxy.h"

#include <qgeotiledmaprequest.h>

#include <QApplication>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QNetworkProxy>
#include <QSize>
#include <QDir>
#include <QDateTime>

#include <QDebug>

QGeoMappingManagerEngineOsm::QGeoMappingManagerEngineOsm(const QMap<QString, QVariant> &parameters, QGeoServiceProvider::Error *error, QString *errorString)
        : QGeoTiledMappingManagerEngine(parameters),
	  m_parameters(parameters),
	  m_nam(NULL),
	  m_servers(QStringList("http://tile.openstreetmap.org/"))
{
    QNetworkDiskCache *cache = NULL;

    Q_UNUSED(error)
    Q_UNUSED(errorString)

    setTileSize(QSize(256, 256));
    setMinimumZoomLevel(0.0);
    setMaximumZoomLevel(18.0);

    QList<QGraphicsGeoMap::MapType> types;
    types << QGraphicsGeoMap::StreetMap;
    setSupportedMapTypes(types);

    QList<QGraphicsGeoMap::ConnectivityMode> modes;
    modes << QGraphicsGeoMap::OnlineMode;
    setSupportedConnectivityModes(modes);

    QList<QString> keys = m_parameters.keys();

    if (keys.contains("mapping.networkaccessmanager")) {
	QNetworkAccessManager *nam;
	nam = (QNetworkAccessManager *)m_parameters
	    .value("mapping.networkaccessmanager").value<void *>();
	if (nam)
	    m_nam = nam;
    }

    if (!m_nam) {
	m_nam = new QNetworkAccessManager(this);
	cache = new QNetworkDiskCache(this);
    }

    if (cache) {
	QDir dir = QDir::temp();
	dir.mkdir("maptiles-osm");
	dir.cd("maptiles-osm");

	cache->setCacheDirectory(dir.path());
    }

    if (keys.contains("mapping.proxy")) {
        QString proxy = m_parameters.value("mapping.proxy").toString();
        if (!proxy.isEmpty())
	  m_nam->setProxy(parseProxy(proxy));
    }

    if (keys.contains("mapping.host")) {
        QString host = m_parameters.value("mapping.host").toString();
        if (!host.isEmpty())
	    m_servers << QStringList(host);
    } else if (keys.contains("mapping.server")) {
        QString server = m_parameters.value("mapping.server").toString();
        if (!server.isEmpty())
	    m_servers = QStringList(server);
    } else if (keys.contains("mapping.servers")) {
	QStringList servers = m_parameters.value("mapping.servers").toStringList();
	if (!servers.isEmpty())
	    m_servers = servers;
    }

    if (cache && keys.contains("mapping.cache.directory")) {
        QString cacheDir = m_parameters.value("mapping.cache.directory").toString();
        if (!cacheDir.isEmpty())
            cache->setCacheDirectory(cacheDir);
    }

    if (cache && keys.contains("mapping.cache.size")) {
        bool ok = false;
        qint64 cacheSize = m_parameters.value("mapping.cache.size").toString().toLongLong(&ok);
        if (ok)
            cache->setMaximumCacheSize(cacheSize);
    }

    if (cache)
	m_nam->setCache(cache);
}

QGeoMappingManagerEngineOsm::~QGeoMappingManagerEngineOsm() {}

QGeoMapData* QGeoMappingManagerEngineOsm::createMapData()
{
    QGeoMapData *data = QGeoTiledMappingManagerEngine::createMapData();
    if (!data)
        return 0;

    data->setConnectivityMode(QGraphicsGeoMap::OnlineMode);
    return data;
}

QGeoTiledMapReply* QGeoMappingManagerEngineOsm::getTileImage(const QGeoTiledMapRequest &request)
{
    QString rawRequest = getRequestString(request);

    QNetworkRequest netRequest = QNetworkRequest(QUrl(rawRequest));
    netRequest.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
    netRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);

    QString ua = QFileInfo(QApplication::applicationFilePath()).fileName();
    ua.remove(QChar('"'), Qt::CaseInsensitive);
    ua += " (Qt";
    ua += qVersion();
    ua += " QtMobility 1.1 ) osm GeoMappingManager";
    netRequest.setRawHeader("User-Agent", ua.toAscii());

    QNetworkReply* netReply = m_nam->get(netRequest);

    QGeoTiledMapReply* mapReply = new QGeoMapReplyOsm(netReply, request, this);

    return mapReply;
}

QString QGeoMappingManagerEngineOsm::getRequestString(const QGeoTiledMapRequest &request) const
{
    QString requestString;

    requestString += m_servers.at((request.row() + request.column()) % m_servers.size());
    requestString += QString::number(request.zoomLevel());
    requestString += '/';
    requestString += QString::number(request.column());
    requestString += '/';
    requestString += QString::number(request.row());
    requestString += ".png";

    return requestString;
}

