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

#include "routeparser.h"

#include <QtXml/QXmlStreamReader>
#include <QDebug>
#include <QStringList>

#include <QGeoRouteSegment>
#include <QGeoManeuver>

RouteParser::RouteParser()
{
    m_reader = new QXmlStreamReader();
}

RouteParser::~RouteParser()
{
    delete m_reader;
}

bool RouteParser::parse(QIODevice *source)
{
    m_errorMessage.clear();

    QByteArray data = source->readAll();
//    qDebug() << "Got response:" << data;
    m_reader->addData(data);

    while(!m_reader->atEnd()) {
        m_reader->readNext();

        if(m_reader->name() == "Error") {
//            qDebug() << "Got error:" << m_reader->attributes().value("message");
            m_errorMessage = m_reader->attributes().value("message").toString();
            return false;
        }

        // Response
        if (m_reader->name() == "Response") {
            m_reader->readNext();

            while(!(m_reader->tokenType() == QXmlStreamReader::EndElement && m_reader->name() == "Response")) {
                m_reader->readNext();

                // Summary
                if(m_reader->name() == "RouteSummary") {
                    parseSummary();
                }
                // RouteGeometry
                if (m_reader->name() == "RouteGeometry") {
                    parseTrack();
                }

                // Segments
                if(m_reader->name() == "RouteInstructionsList") {
                    parseSegments();
                    int i = 1;
                    while(i < m_segments.count()) {
//                        qDebug() << "appending route segment of" << m_segments.at(i).path().count() << "points";
                        m_segments[i-1].setNextRouteSegment(m_segments[i]);
                        i++;
                    }
                    if(!m_segments.isEmpty()) {
                        m_route.setFirstRouteSegment(m_segments.first());
                    }
                }
            }
        }
    }

//    qDebug() << "Track is" << m_route.path();
//    qDebug() << "Distance is" << m_route.distance();
//    qDebug() << "Bounding box is" << m_route.bounds().topLeft() << m_route.bounds().bottomRight();
//    qDebug() << "Got" << m_segments.count() << "segments";
    return true;

}

QGeoRoute RouteParser::route()
{
    return m_route;
}

QString RouteParser::errorMessage()
{
    return m_errorMessage;
}

void RouteParser::parseSummary()
{
    if(m_reader->name() == "RouteSummary") {
        m_reader->readNext();

        while(!(m_reader->tokenType() == QXmlStreamReader::EndElement && m_reader->name() == "RouteSummary")) {
            m_reader->readNext();

            if(m_reader->name() == "TotalTime") {
                m_reader->readNext();
                QString time = m_reader->text().toString();
                m_route.setTravelTime(parseTime(time));
                m_reader->readNext();
            }

            if(m_reader->name() == "TotalDistance") {
                m_route.setDistance(m_reader->attributes().value("value").toString().toDouble());
                m_reader->readNext();
            }

            if(m_reader->name() == "BoundingBox") {
                QGeoBoundingBox bounds;
                while(!(m_reader->tokenType() == QXmlStreamReader::EndElement && m_reader->name() == "BoundingBox")) {
                    m_reader->readNext();

                    // topLeft
                    if (m_reader->tokenType() == QXmlStreamReader::StartElement && m_reader->name() == "pos") {
                        m_reader->readNext();
                        QGeoCoordinate coord;
                        coord.setLongitude(m_reader->text().toString().split(' ').first().toDouble());
                        coord.setLatitude(m_reader->text().toString().split(' ').last().toDouble());
//                        qDebug() << "got coordinate for boundin box" << coord;
                        bounds.setTopLeft(coord);
                    }

                    m_reader->readNext();
                    // bottomRight
                    if (m_reader->tokenType() == QXmlStreamReader::StartElement && m_reader->name() == "pos") {
                        m_reader->readNext();
                        QGeoCoordinate coord;
                        coord.setLongitude(m_reader->text().toString().split(' ').first().toDouble());
                        coord.setLatitude(m_reader->text().toString().split(' ').last().toDouble());
//                        qDebug() << "got coordinate for bounding box" << coord;
                        bounds.setBottomRight(coord);
                    }
                }
                m_route.setBounds(bounds);
            }
        }
    }
}

void RouteParser::parseTrack()
{
    if (m_reader->name() == "RouteGeometry") {
        m_reader->readNext();

        QList<QGeoCoordinate> path;
        while(!(m_reader->tokenType() == QXmlStreamReader::EndElement && m_reader->name() == "RouteGeometry")) {
            m_reader->readNext();
            if (m_reader->tokenType() == QXmlStreamReader::StartElement && m_reader->name() == "pos") {
                m_reader->readNext();
                QGeoCoordinate coord;
                coord.setLongitude(m_reader->text().toString().split(' ').first().toDouble());
                coord.setLatitude(m_reader->text().toString().split(' ').last().toDouble());
//                qDebug() << "got coordinate" << coord;
                path.append(coord);
            }
        }
        m_route.setPath(path);
    }
}

void RouteParser::parseSegments()
{
    if(m_reader->name() == "RouteInstructionsList") {
        m_reader->readNext();

        while(!(m_reader->tokenType() == QXmlStreamReader::EndElement && m_reader->name() == "RouteInstructionsList")) {
            m_reader->readNext();

            if(m_reader->name() == "RouteInstruction") {
                QGeoRouteSegment segment;
                QGeoManeuver maneuver;

                maneuver.setTimeToNextInstruction(parseTime(m_reader->attributes().value("duration").toString()));

                m_reader->readNext();
                while(!(m_reader->tokenType() == QXmlStreamReader::EndElement && m_reader->name() == "RouteInstruction")) {
                    m_reader->readNext();

                    if(m_reader->name() == "Instruction") {
                        m_reader->readNext();
//                        qDebug() << "Instruction:" << m_reader->text();
                        maneuver.setInstructionText(m_reader->text().toString());
                    }

                    if(m_reader->name() == "distance") {
//                        qDebug() << "distance" << m_reader->attributes().value("value").toString();
                        maneuver.setDistanceToNextInstruction(m_reader->attributes().value("value").toString().toDouble());
                    }

                    if(m_reader->name() == "LineString") {
                        m_reader->readNext();

                        QList<QGeoCoordinate> path;
                        while(!(m_reader->tokenType() == QXmlStreamReader::EndElement && m_reader->name() == "LineString")) {
                            m_reader->readNext();

                            if (m_reader->tokenType() == QXmlStreamReader::StartElement && m_reader->name() == "pos") {
                                m_reader->readNext();
                                QGeoCoordinate coord;
                                coord.setLongitude(m_reader->text().toString().split(' ').first().toDouble());
                                coord.setLatitude(m_reader->text().toString().split(' ').last().toDouble());
//                                qDebug() << "got Segment coordinate" << coord;

                                path.append(coord);
                            }

                        }
                        segment.setPath(path);
//                        qDebug() << "segment finished";
                    }
                }
                segment.setManeuver(maneuver);
                m_segments.append(segment);
            }
        }
    }
}

int RouteParser::parseTime(const QString &timeStr)
{
    int seconds = 0;
    QString value;
    for(int i = 0; i < timeStr.size(); ++i) {
        QChar ch = timeStr.at(i);
        if(ch.digitValue() != -1) {
            value.append(ch);
        } else  if(ch == 'H') {
            seconds += value.toInt() * 60 * 60;
            value.clear();
        } else if(ch == 'M') {
            seconds += value.toInt() * 60;
            value.clear();
        } else if(ch == 'S') {
            seconds += value.toInt();
            value.clear();
        }

    }
    return seconds;
}

