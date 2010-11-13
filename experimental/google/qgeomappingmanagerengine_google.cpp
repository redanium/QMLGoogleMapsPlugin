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
** This file is part of the Ovi services plugin for the Maps and
** Navigation API.  The use of these services, whether by use of the
** plugin or by other means, is governed by the terms and conditions
** described by the file OVI_SERVICES_TERMS_AND_CONDITIONS.txt in
** this package, located in the directory containing the Ovi services
** plugin source code.
**
****************************************************************************/

#include "qgeomappingmanagerengine_google.h"
#include "qgeomapreply_google.h"

#include <qgeotiledmaprequest.h>

#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QNetworkProxy>
#include <QSize>
#include <QDir>
#include <QDateTime>

#include <QDebug>

#define PI 3.14159265

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <math.h>

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

QGeoMappingManagerEngineGoogle::QGeoMappingManagerEngineGoogle(const QMap<QString, QVariant> &parameters, QGeoServiceProvider::Error *error, QString *errorString)
    : QGeoTiledMappingManagerEngine(parameters),
      m_tileSize(256, 256),
      m_format("png32"),
      m_sensor(false)
{
    Q_UNUSED(error)
    Q_UNUSED(errorString)

    setMinimumZoomLevel(0.0);
    setMaximumZoomLevel(21.0);

    QList<QGraphicsGeoMap::MapType> types;
    types << QGraphicsGeoMap::StreetMap;
    types << QGraphicsGeoMap::SatelliteMapDay;
    types << QGraphicsGeoMap::TerrainMap;
    /* FIXME add QGraphicsGeoMap::HybridMap */
    setSupportedMapTypes(types);

    QList<QGraphicsGeoMap::ConnectivityMode> modes;
    modes << QGraphicsGeoMap::OnlineMode;
    setSupportedConnectivityModes(modes);

    m_networkManager = new QNetworkAccessManager(this);

    if (parameters.contains("mapping.tilesize")) {
        QSize tileSize = parameters.value("mapping.tilesize").toSize();

	if (tileSize.width() > 0 && tileSize.width() <= 640 &&
	    tileSize.height() > 0 && tileSize.height() <= 640) {
	    m_tileSize = tileSize;
	}
	/* FIXME: error ? */
    }

    setTileSize(m_tileSize);

    if (parameters.contains("mapping.format")) {
	QString format = parameters.value("mapping.format").toString();
	if (!format.isEmpty())
	    m_format = format;
    }

    if (parameters.contains("mapping.language")) {
	QString language = parameters.value("mapping.language").toString();
	if (!language.isEmpty())
	    m_language = language;
    }

    if (parameters.contains("mapping.sensor")) {
	bool sensor = parameters.value("mapping.sensor").toBool();
	m_sensor = sensor;
    }

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
    // TODO add error detection for if request.connectivityMode() != QGraphicsGeoMap::OnlineMode
    QString rawRequest = getRequestString(request);

    QNetworkRequest netRequest((QUrl(rawRequest))); // The extra pair of parens disambiguates this from a function declaration
    netRequest.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);

#ifdef DISK_CACHE_ENABLED
    netRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    m_cache->metaData(netRequest.url()).setLastModified(QDateTime::currentDateTime());
#endif

    QNetworkReply* netReply = m_networkManager->get(netRequest);

    QGeoTiledMapReply* mapReply = new QGeoMapReplyGoogle(netReply, request);

    // TODO goes badly on linux
    //qDebug() << "request: " << QString::number(reinterpret_cast<int>(mapReply), 16) << " " << request.row() << "," << request.column();
    // this one might work better. It follows defined behaviour, unlike reinterpret_cast
    //qDebug("request: %p %i,%i @ %i", mapReply, request.row(), request.column(), request.zoomLevel());
    return mapReply;
}

QString QGeoMappingManagerEngineGoogle::getRequestString(const QGeoTiledMapRequest &request) const
{
    static const QString http("http");
    static const QString host("maps.google.com");
    static const QString path("/maps/api/staticmap");
    static const QString zoom("zoom");
    static const QString center("center");
    static const QString format("format");
    static const QString language("language");
    static const QString sensor("sensor");
    static const QString size("size");
    static const QString strue("true");
    static const QString sfalse("false");
    static const QString maptype("maptype");

    QUrl url;
    QPointF coord = positionToCoordinate(QPoint(request.column(), request.row()), request.zoomLevel());
    qDebug() << coord << request.column() << request.row();
    url.setScheme(http);
    url.setHost(host);
    url.setPath(path);
    url.addQueryItem(center, QString("%1,%2").arg(coord.x()).arg(coord.y()));
    url.addQueryItem(size, QString("%1x%2").arg(m_tileSize.width()).arg(m_tileSize.height()));
    url.addQueryItem(zoom, QString("%1").arg(request.zoomLevel()));
    url.addQueryItem(format, m_format);
    url.addQueryItem(sensor, m_sensor ? strue : sfalse);
    if (!m_language.isEmpty())
	url.addQueryItem(language, m_language);
    url.addQueryItem(maptype, mapTypeToStr(request.mapType()));
    qWarning() << url.toString();
    return url.toString();
}

static qreal rmod(const qreal a, const qreal b)
{
    quint64 div = static_cast<quint64>(a / b);
    return a - static_cast<qreal>(div) * b;
}

QPointF QGeoMappingManagerEngineGoogle::positionToCoordinate(const QPoint & pixel, int zoom) const
{
/*
    int tiles = pow(2, zoom);
    int width = tileSize().width();
    int height = tillongitude, latitude);eSize().height();

    qreal longitude = (point.y() * width * (360. / (tiles * width))) - 180.;
    qreal latitude = (180. / PI) * atan(sinh((1. - point.x() * height * (2. / (tiles * height))) * PI));

    return QPointF(-longitude, latitude);
*//*maximumZoomLevel()*/
    QSize worldReferenceSize = (1 << qRound(zoom)) * tileSize();

    qreal fx = qreal(pixel.x() * tileSize().width()) / worldReferenceSize.width();
    qreal fy = qreal(pixel.y() * tileSize().height()) / worldReferenceSize.height();
    qDebug () << fx << fy;
    if (fy < 0.0f)
        fy = 0.0f;
    else if (fy > 1.0f)
        fy = 1.0f;

    qreal lat;

    if (fy == 0.0f)
        lat = 90.0f;
    else if (fy == 1.0f)
        lat = -90.0f;
    else
        lat = (180.0f / PI) * (2.0f * atan(exp(PI * (1.0f - 2.0f * fy))) - (PI / 2.0f));

    qreal lng;
    if (fx >= 0) {
        lng = rmod(fx, 1.0f);
    } else {
        lng = rmod(1.0f - rmod(-1.0f * fx, 1.0f), 1.0f);
    }

    lng = lng * 360.0f - 180.0f;

    return QPointF(lat, lng);
}

QString QGeoMappingManagerEngineGoogle::mapTypeToStr(QGraphicsGeoMap::MapType type) const
{
    if  (type == QGraphicsGeoMap::StreetMap)
        return "roadmap";
    else if (type == QGraphicsGeoMap::SatelliteMapDay ||
             type == QGraphicsGeoMap::SatelliteMapNight) {
        return "satellite";
    } else if (type == QGraphicsGeoMap::TerrainMap)
        return "terrain";
    else
        return "roadmap";
}
