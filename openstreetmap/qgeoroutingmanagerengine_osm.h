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

#ifndef QGEOROUTINGMANAGERENGINEOSM_H
#define QGEOROUTINGMANAGERENGINEOSM_H

#include <qgeoserviceprovider.h>
#include <QGeoRoutingManagerEngine>

#include <QNetworkAccessManager>

QTM_USE_NAMESPACE

class QGeoRoutingManagerEngineOsm : public QGeoRoutingManagerEngine
{
    Q_OBJECT
public:
    explicit QGeoRoutingManagerEngineOsm(const QMap<QString, QVariant> &parameters,
                                         QGeoServiceProvider::Error *error,
                                         QString *errorString);

    QGeoRouteReply *calculateRoute(const QGeoRouteRequest &request);

signals:

public slots:

private:
    enum PointType {
        StartPoint,
        ViaPoint,
        EndPoint
    };

    QNetworkAccessManager *m_networkManager;

    QString xmlHeader() const;
    QString requestHeader( const QString &unit, const QString &routePreference ) const;
    QString requestPoint( PointType pointType, const QGeoCoordinate &coordinate ) const;
    QString requestFooter(const QGeoRouteRequest &request) const;
    QString xmlFooter() const;

    QGeoRouteReply* parse( const QByteArray &content ) const;

private slots:
    void routingFinished();
    void routingError(QGeoRouteReply::Error error, const QString &errorString);

};

#endif // QGEOROUTINGMANAGERENGINEOSM_H

