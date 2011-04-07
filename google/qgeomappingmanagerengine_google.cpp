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
**
****************************************************************************/

#include "qgeomappingmanagerengine_google.h"
#include "qgeomapreply_google.h"
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

QGeoMappingManagerEngineGoogle::QGeoMappingManagerEngineGoogle(const QMap<QString, QVariant> &parameters, QGeoServiceProvider::Error *error, QString *errorString)
    : QGeoTiledMappingManagerEngine(parameters),
      m_parameters(parameters),
      m_nam(NULL)
{
    QNetworkDiskCache *cache = NULL;

    Q_UNUSED(error)
    Q_UNUSED(errorString)

    setTileSize(QSize(256, 256));
    setMinimumZoomLevel(0.0);
    setMaximumZoomLevel(17.0);

    QList<QGraphicsGeoMap::MapType> types;
    types << QGraphicsGeoMap::StreetMap;
    types << QGraphicsGeoMap::SatelliteMapDay;
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
	dir.mkdir("maptiles-google");
	dir.cd("maptiles-google");

	cache->setCacheDirectory(dir.path());
    }

    if (keys.contains("mapping.proxy")) {
        QString proxy = m_parameters.value("mapping.proxy").toString();
        if (!proxy.isEmpty())
	  m_nam->setProxy(parseProxy(proxy));
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

QGeoMappingManagerEngineGoogle::~QGeoMappingManagerEngineGoogle() {}

QGeoMapData* QGeoMappingManagerEngineGoogle::createMapData()
{
    QGeoMapData *data = QGeoTiledMappingManagerEngine::createMapData();
    if (!data)
        return 0;

    data->setConnectivityMode(QGraphicsGeoMap::OnlineMode);
    return data;
}

QGeoTiledMapReply* QGeoMappingManagerEngineGoogle::getTileImage(const QGeoTiledMapRequest &request)
{
    QString rawRequest = getRequestString(request);

    QNetworkRequest netRequest = QNetworkRequest(QUrl(rawRequest));
    netRequest.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
    netRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    QString ua = QFileInfo(QApplication::applicationFilePath()).fileName();
    ua.remove(QChar('"'), Qt::CaseInsensitive);
    ua += " (Qt";
    ua += qVersion();
    ua += " QtMobility 1.1 ) google GeoMappingManager";
    netRequest.setRawHeader("User-Agent", ua.toAscii());

    if (m_nam->cache())
	m_nam->cache()->metaData(netRequest.url()).setLastModified(QDateTime::currentDateTime());

    QNetworkReply* netReply = m_nam->get(netRequest);

    QGeoTiledMapReply* mapReply = new QGeoMapReplyGoogle(netReply, request, this);

    return mapReply;
}

QString QGeoMappingManagerEngineGoogle::getRequestString(const QGeoTiledMapRequest &request) const
{
    QString requestString;

    requestString = mapTypeToServer(request.mapType());

    return requestString
      .arg(request.column())
      .arg(request.row())
      .arg(request.zoomLevel());
}

QString QGeoMappingManagerEngineGoogle::mapTypeToServer(QGraphicsGeoMap::MapType type) const
{
    if (type == QGraphicsGeoMap::SatelliteMapDay ||
	type == QGraphicsGeoMap::SatelliteMapNight) {
      return "http://khm.google.com/kh?v=51&x=%1&s=&y=%2&z=%3";
    } else
      return "http://mt.google.com/vt/lyrs=&x=%1&s=&y=%2&z=%3";
}
