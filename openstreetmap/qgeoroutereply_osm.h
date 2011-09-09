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

#ifndef QGEOROUTEREPLYOSM_H
#define QGEOROUTEREPLYOSM_H

#include <QGeoRouteReply>
#include <QNetworkReply>

QTM_USE_NAMESPACE

class QGeoRouteReplyOsm : public QGeoRouteReply
{
    Q_OBJECT
public:
    explicit QGeoRouteReplyOsm(QObject *parent = 0);
    public:
        QGeoRouteReplyOsm(QNetworkReply *reply, const QGeoRouteRequest &request, QObject *parent = 0);
        ~QGeoRouteReplyOsm();

//        void abort();

        QNetworkReply* networkReply() const;

    private slots:
        void replyDestroyed();
        void networkFinished();
        void networkError(QNetworkReply::NetworkError error);

    private:
        QNetworkReply *m_reply;

};

#endif // QGEOROUTEREPLYOSM_H

