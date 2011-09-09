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

#ifndef ROUTEPARSER_H
#define ROUTEPARSER_H

#include <QGeoCoordinate>
#include <QGeoBoundingBox>
#include <QGeoRouteSegment>
#include <QGeoRoute>
#include <QIODevice>
#include <QXmlStreamReader>
#include <QTime>

QTM_USE_NAMESPACE

class RouteParser
{
public:
    RouteParser();
    ~RouteParser();

    bool parse(QIODevice *source);

    /** If parse() returned false, this will return the error message. Therwise an empty strng */
    QString errorMessage();

    QGeoRoute route();

private:
    QGeoRoute m_route;
    QXmlStreamReader *m_reader;
    QList<QGeoRouteSegment> m_segments;
    QString m_errorMessage;

    void parseSummary();
    void parseTrack();
    void parseSegments();
    int parseTime(const QString &timeStr);
};

#endif // ROUTEPARSER_H

