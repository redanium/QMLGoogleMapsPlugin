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

#include "qgeoroutereply_osm.h"
#include "routeparser.h"

#include <QGeoRouteSegment>
#include <QDebug>

QGeoRouteReplyOsm::QGeoRouteReplyOsm(QNetworkReply *reply,
                                     const QGeoRouteRequest &request,
                                     QObject *parent):
    QGeoRouteReply(request, parent),
    m_reply(reply)
{
    m_reply->setParent(this);

    connect(m_reply,
            SIGNAL(finished()),
            this,
            SLOT(networkFinished()));

    connect(m_reply,
            SIGNAL(error(QNetworkReply::NetworkError)),
            this,
            SLOT(networkError(QNetworkReply::NetworkError)));

    connect(m_reply,
            SIGNAL(destroyed()),
            this,
            SLOT(replyDestroyed()));
}

QGeoRouteReplyOsm::~QGeoRouteReplyOsm()
{

}

void QGeoRouteReplyOsm::replyDestroyed()
{
    m_reply = 0;
}

void QGeoRouteReplyOsm::networkError(QNetworkReply::NetworkError error)
{
    qDebug() << "routing error" << error;
}

void QGeoRouteReplyOsm::networkFinished()
{
    if (!m_reply)
        return;

    if (m_reply->error() != QNetworkReply::NoError) {
        setError(QGeoRouteReply::CommunicationError, m_reply->errorString());
        m_reply->deleteLater();
        return;
    }

    RouteParser parser;
    if (parser.parse(m_reply)) {
        QGeoRoute route = parser.route();
        setRoutes(QList<QGeoRoute>() << route);
        setFinished(true);
    } else {
        setError(QGeoRouteReply::ParseError, parser.errorMessage());
    }

    m_reply->deleteLater();
}

