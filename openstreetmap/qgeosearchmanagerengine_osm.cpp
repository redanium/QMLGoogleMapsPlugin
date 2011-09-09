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

#include "qgeosearchmanagerengine_osm.h"
#include "qgeosearchreply_osm.h"
#include "parseproxy.h"

#include <qgeoaddress.h>
#include <qgeocoordinate.h>
#include <QNetworkProxy>
#include <QMap>
#include <QApplication>
#include <QFileInfo>
#include <QLocale>

//http://wiki.openstreetmap.org/wiki/Nominatim

QGeoSearchManagerEngineOsm::QGeoSearchManagerEngineOsm(
    const QMap<QString, QVariant> &parameters, QGeoServiceProvider::Error *error,
    QString *errorString)
        : QGeoSearchManagerEngine(parameters),
	  m_networkManager(NULL),
	  m_host("nominatim.openstreetmap.org")
{
    QList<QString> keys = parameters.keys();

    if (keys.contains("places.networkaccessmanager")) {
	QNetworkAccessManager *nam;
	nam = (QNetworkAccessManager *)parameters
	    .value("places.networkaccessmanager").value<void *>();
	if (nam)
	    m_networkManager = nam;
    }

    if (!m_networkManager)
	m_networkManager = new QNetworkAccessManager(this);

    if (keys.contains("places.proxy")) {
        QString proxy = parameters.value("places.proxy").toString();
        if (!proxy.isEmpty())
	  m_networkManager->setProxy(parseProxy(proxy));
    }

    if (keys.contains("places.host")) {
        QString host = parameters.value("places.host").toString();
        if (!host.isEmpty())
            m_host = host;
    }

    setSupportsGeocoding(true);
    setSupportsReverseGeocoding(true);

    QGeoSearchManager::SearchTypes supportedSearchTypes;
    supportedSearchTypes |= QGeoSearchManager::SearchLandmarks;
    supportedSearchTypes |= QGeoSearchManager::SearchGeocode;
    setSupportedSearchTypes(supportedSearchTypes);

    if (error)
        *error = QGeoServiceProvider::NoError;

    if (errorString)
        *errorString = "";
}

QGeoSearchManagerEngineOsm::~QGeoSearchManagerEngineOsm() {}

QGeoSearchReply* QGeoSearchManagerEngineOsm::geocode(const QGeoAddress &address,
        QGeoBoundingArea *bounds)
{
    if (!supportsGeocoding()) {
        QGeoSearchReply *reply = new QGeoSearchReply(QGeoSearchReply::UnsupportedOptionError, "Geocoding is not supported by this service provider.", this);
        emit error(reply, reply->error(), reply->errorString());
        return reply;
    }
    QString searchString;

    if(!address.street().isEmpty()) {
        searchString += address.street();
        searchString += ",";
    }
    if(!address.city().isEmpty()) {
        searchString += address.city();
        searchString += ",";
    }
    if(!address.postcode().isEmpty()) {
        searchString += address.postcode();
        searchString += ",";
    }
    if(!address.district().isEmpty()) {
        searchString += address.district();
        searchString += ",";
    }
    if(!address.county().isEmpty()) {
        searchString += address.county();
        searchString += ",";
    }
    if(!address.state().isEmpty()) {
        searchString += address.state();
        searchString += ",";
    }
    if(!address.country().isEmpty()) {
        searchString += address.country();
        searchString += ",";
    }
    if(!address.countryCode().isEmpty()) {
        searchString += address.countryCode();
        searchString += ",";
    }

    return search(searchString,QGeoSearchManager::SearchGeocode, -1, 0,bounds);
}

QGeoSearchReply* QGeoSearchManagerEngineOsm::reverseGeocode(const QGeoCoordinate &coordinate,
        QGeoBoundingArea *bounds)
{
    if (!supportsReverseGeocoding()) {
        QGeoSearchReply *reply = new QGeoSearchReply(QGeoSearchReply::UnsupportedOptionError, "Reverse geocoding is not supported by this service provider.", this);
        emit error(reply, reply->error(), reply->errorString());
        return reply;
    }
    QString requestString = "http://";
    requestString += m_host;
    requestString += "/reverse";
    requestString += "?format=xml&addressdetails=1&osm_type=N&zoom=18";
    requestString += "&accept-language=";
    requestString += locale().name();

    requestString += "&lat=";
    requestString += QString::number(coordinate.latitude());
    requestString += "&lon=";
    requestString += QString::number(coordinate.longitude());

    return search(requestString,bounds);
}

QGeoSearchReply* QGeoSearchManagerEngineOsm::search(const QString &searchString,
        QGeoSearchManager::SearchTypes searchTypes,
        int limit,
        int offset,
        QGeoBoundingArea *bounds)
{
    if ((searchTypes != QGeoSearchManager::SearchTypes(QGeoSearchManager::SearchAll))
            && ((searchTypes & supportedSearchTypes()) != searchTypes) ) {

        QGeoSearchReply *reply = new QGeoSearchReply(QGeoSearchReply::UnsupportedOptionError,
                "The selected search type is not supported by this service provider.", this);
        emit error(reply, reply->error(), reply->errorString());
        return reply;
    }

    QString requestString = "http://";
    requestString += m_host;
    requestString += "/search?q=";
    requestString += searchString;
    requestString += "&format=xml&polygon=0&addressdetails=1";
    requestString += "&accept-language=";
    requestString += locale().name();

    if (bounds && (bounds->type() == QGeoBoundingArea::BoxType) && bounds->isValid() && !bounds->isEmpty()) {
        QGeoBoundingBox* bbox = (QGeoBoundingBox*)bounds;
        requestString += "&viewbox=";
        requestString += QString::number(bbox->topLeft().longitude());
        requestString += ",";
        requestString += QString::number(bbox->bottomLeft().latitude());
        requestString += ",";
        requestString += QString::number(bbox->bottomLeft().longitude());
        requestString += ",";
        requestString += QString::number(bbox->topLeft().latitude());
    }
    return search(requestString, bounds, limit, offset);
}

QGeoSearchReply* QGeoSearchManagerEngineOsm::search(QString requestString,
                                                    QGeoBoundingArea *bounds,
                                                    int limit,
                                                    int offset)
{
    QNetworkRequest netRequest = QNetworkRequest(QUrl(requestString));

    QString ua = QFileInfo(QApplication::applicationFilePath()).fileName();
    ua.remove(QChar('"'), Qt::CaseInsensitive);
    ua += " (Qt";
    ua += qVersion();
    ua += " QtMobility 1.1 ) osm GeoSearchManager";
    netRequest.setRawHeader("User-Agent", ua.toAscii());

    QNetworkReply *networkReply = m_networkManager->get(netRequest);

    QGeoSearchReplyOsm *reply = new QGeoSearchReplyOsm(networkReply, limit, offset, bounds, this);

    connect(reply,
            SIGNAL(finished()),
            this,
            SLOT(placesFinished()));

    connect(reply,
            SIGNAL(error(QGeoSearchReply::Error, QString)),
            this,
            SLOT(placesError(QGeoSearchReply::Error, QString)));

    return reply;
}

void QGeoSearchManagerEngineOsm::placesFinished()
{
    QGeoSearchReply *reply = qobject_cast<QGeoSearchReply*>(sender());

    if (!reply)
        return;

    if (receivers(SIGNAL(finished(QGeoSearchReply*))) == 0) {
        reply->deleteLater();
        return;
    }

    emit finished(reply);
}

void QGeoSearchManagerEngineOsm::placesError(QGeoSearchReply::Error error, const QString &errorString)
{
    QGeoSearchReply *reply = qobject_cast<QGeoSearchReply*>(sender());

    if (!reply)
        return;

    if (receivers(SIGNAL(error(QGeoSearchReply*, QGeoSearchReply::Error, QString))) == 0) {
        reply->deleteLater();
        return;
    }

    emit this->error(reply, error, errorString);
}

