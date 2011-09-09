/*****************************************************************************
 * Copyright: 2011 Michael Zanetti <mzanetti@kde.org>                        *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Lesser General Public License as published  *
 * by the Free Software Foundation, either version 2.1 of the License, or    *
 * (at your option) any later version.                                       *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU General Public License for more details.                              *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 ****************************************************************************/

#include "qgeoroutingmanagerengine_osm.h"
#include "parseproxy.h"
#include "qgeoroutereply_osm.h"

#include <QHttpRequestHeader>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QXmlStreamReader>
#include <QDebug>

QGeoRoutingManagerEngineOsm::QGeoRoutingManagerEngineOsm(const QMap<QString, QVariant> &parameters,
                                                         QGeoServiceProvider::Error *error,
                                                         QString *errorString):
    QGeoRoutingManagerEngine(parameters),
    m_networkManager(0)
{
    QList<QString> keys = parameters.keys();

    if (keys.contains("places.networkaccessmanager")) {
        QNetworkAccessManager *nam;
        nam = (QNetworkAccessManager *)parameters
                .value("places.networkaccessmanager").value<void *>();
        if (nam)
            m_networkManager = nam;
    }

    if (!m_networkManager) {
        m_networkManager = new QNetworkAccessManager(this);
    }

    if (keys.contains("places.proxy")) {
        QString proxy = parameters.value("places.proxy").toString();
        if (!proxy.isEmpty())
            m_networkManager->setProxy(parseProxy(proxy));
    }

    // The TODO list :D
    setSupportsRouteUpdates(false);
    setSupportsAlternativeRoutes(false);
    // ...

    if (error)
        *error = QGeoServiceProvider::NoError;

    if (errorString)
        *errorString = "";
}

QGeoRouteReply* QGeoRoutingManagerEngineOsm::calculateRoute(const QGeoRouteRequest &request)
{
    qDebug() << "should calculate route";

    QString requestStr = xmlHeader();
    QString unit = "M"; // QGeoRoute stores distance always in meters
    QString preference = "Fastest";
    if(request.travelModes() & QGeoRouteRequest::PedestrianTravel) {
        preference = "Pedestrian";
    } else {
        if(request.routeOptimization().testFlag(QGeoRouteRequest::ShortestRoute)) {
            preference = "Shortest";
        }
    }
    requestStr += requestHeader(unit, preference);
    requestStr += requestPoint(StartPoint, request.waypoints().first() );
    for(int i = 1; i < request.waypoints().count() - 1; ++i) {
        requestStr += requestPoint(ViaPoint, request.waypoints().at(i));
    }
    requestStr += requestPoint( EndPoint, request.waypoints().last());

    requestStr += requestFooter( request );
    requestStr += xmlFooter();
//    qDebug() << "POST: " << requestStr;

    // Please refrain from making this URI public. To use it outside the scope
    // of QtMobility you need permission from the openrouteservice.org team.
    QUrl url = QUrl( "http://openls.geog.uni-heidelberg.de/qtmobility/route" );

//    qDebug() << "requesting url:" << url.toString() << requestStr;
    QNetworkReply *networkReply = m_networkManager->post( QNetworkRequest( url ), requestStr.toLatin1() );

    QGeoRouteReplyOsm *reply = new QGeoRouteReplyOsm(networkReply, request, this);

    connect(reply,
            SIGNAL(finished()),
            this,
            SLOT(routingFinished()));

    connect(reply,
            SIGNAL(error(QGeoRouteReply::Error, QString)),
            this,
            SLOT(routingError(QGeoRouteReply::Error, QString)));

    return reply;
}

QString QGeoRoutingManagerEngineOsm::xmlHeader() const
{
    QString result = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    result += "<xls:XLS xmlns:xls=\"http://www.opengis.net/xls\" xmlns:sch=\"http://www.ascc.net/xml/schematron\" ";
    result += "xmlns:gml=\"http://www.opengis.net/gml\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" ";
    result += "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ";
    result += "xsi:schemaLocation=\"http://www.opengis.net/xls ";
    result += "http://schemas.opengis.net/ols/1.1.0/RouteService.xsd\" version=\"1.1\" xls:lang=\"%1\">\n";
    result += "<xls:RequestHeader/>\n";
    return result.arg(locale().name().split('_').first());
}

QString QGeoRoutingManagerEngineOsm::requestHeader( const QString &unit, const QString &routePreference ) const
{
    QString result = "<xls:Request methodName=\"RouteRequest\" requestID=\"123456789\" version=\"1.1\">\n";
    result += "<xls:DetermineRouteRequest distanceUnit=\"%1\">\n";
    result += "<xls:RoutePlan>\n";
    result += "<xls:RoutePreference>%2</xls:RoutePreference>\n";
    result += "<xls:WayPointList>\n";
    return result.arg( unit ).arg( routePreference );
}

QString QGeoRoutingManagerEngineOsm::requestPoint( PointType pointType, const QGeoCoordinate &coordinate ) const
{
    QString result = "<xls:%1>\n";
    result += "<xls:Position>\n";
    result += "<gml:Point srsName=\"EPSG:4326\">\n";
    result += "<gml:pos>%2 %3</gml:pos>\n";
    result += "</gml:Point>\n";
    result += "</xls:Position>\n";
    result += "</xls:%1>\n";
    result = result.arg( pointType == StartPoint ? "StartPoint" : ( pointType == ViaPoint ? "ViaPoint" : "EndPoint" ) );
    result = result.arg( coordinate.longitude(), 0, 'f', 14 );
    result = result.arg( coordinate.latitude(), 0, 'f', 14 );
    return result;
}

QString QGeoRoutingManagerEngineOsm::requestFooter(const QGeoRouteRequest &request) const
{
    QString result = "</xls:WayPointList>\n";

    if(!request.featureTypes().contains(QGeoRouteRequest::TollFeature) ||
            !request.featureTypes().contains(QGeoRouteRequest::HighwayFeature)) {
        result += "<xls:AvoidList>\n";
        if(!request.featureTypes().contains(QGeoRouteRequest::TollFeature)) {
            result += "<xls:AvoidFeature>Tollway</xls:AvoidFeature>";
        }
        if(!request.featureTypes().contains(QGeoRouteRequest::HighwayFeature)) {
            result += "<xls:AvoidFeature>Highway</xls:AvoidFeature>";
        }
        result += "</xls:AvoidList>\n";
    }

    result += "</xls:RoutePlan>\n";
    result += "<xls:RouteInstructionsRequest provideGeometry=\"true\" />\n";
    result += "<xls:RouteGeometryRequest/>\n";
    result += "</xls:DetermineRouteRequest>\n";
    result += "</xls:Request>\n";
    return result;
}

QString QGeoRoutingManagerEngineOsm::xmlFooter() const
{
    return "</xls:XLS>\n";
}

void QGeoRoutingManagerEngineOsm::routingFinished()
{
    QGeoRouteReply *reply = qobject_cast<QGeoRouteReply*>(sender());

    if (!reply)
        return;

    if (receivers(SIGNAL(finished(QGeoRouteReply*))) == 0) {
        reply->deleteLater();
        return;
    }

    emit finished(reply);
}

void QGeoRoutingManagerEngineOsm::routingError(QGeoRouteReply::Error error, const QString &errorString)
{
    QGeoRouteReply *reply = qobject_cast<QGeoRouteReply*>(sender());

    if (!reply)
        return;

    if (receivers(SIGNAL(error(QGeoRouteReply*, QGeoRouteReply::Error, QString))) == 0) {
        reply->deleteLater();
        return;
    }

    emit this->error(reply, error, errorString);

}

