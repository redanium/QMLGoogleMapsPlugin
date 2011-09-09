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

#include "qgeocodeparser.h"

#include <QXmlStreamReader>
#include <QIODevice>
#include <QStringList>

QGeoCodeParser::QGeoCodeParser()
        : m_reader(0)
{
}

QGeoCodeParser::~QGeoCodeParser()
{
    if (m_reader)
        delete m_reader;
}

bool QGeoCodeParser::parse(QIODevice* source)
{
    if (m_reader)
        delete m_reader;
    m_reader = new QXmlStreamReader(source);

    if (!parseRootElement()) {
        m_errorString = m_reader->errorString();
        return false;
    }

    m_errorString = "";

    return true;
}

QList<QGeoPlace> QGeoCodeParser::results() const
{
    return m_results;
}

QString QGeoCodeParser::errorString() const
{
    return m_errorString;
}

bool QGeoCodeParser::parseRootElement()
{
    if (m_reader->readNextStartElement()) {
        if (m_reader->name() == "searchresults") {
            while (m_reader->readNextStartElement()) {
                if (m_reader->name().toString() == "place") {
                    QGeoPlace place;
                    if (!parsePlace(&place))
                        return false;
                    m_results.append(place);
                } else {
                    m_reader->raiseError(QString("The element \"searchresults\" did not expect a child element named \"%1\".").arg(m_reader->name().toString()));
                    return false;
                }
            }
        } else if (m_reader->name() == "reversegeocode") {
            while (m_reader->readNextStartElement()) {
                if (m_reader->name().toString() == "addressparts") {
                    QGeoPlace place;
                    if (!parsePlace(&place))
                        return false;
                    m_results.append(place);
                } else if (m_reader->name().toString() == "result") {
                    m_reader->skipCurrentElement();
                } else {
                    m_reader->raiseError(QString("The element \"reversegeocode\" did not expect a child element named \"%1\".").arg(m_reader->name().toString()));
                    return false;
                }
            }
        } else {
            m_reader->raiseError("The place not found, request needs more attributes or there is error in request.");
            return false;
        }
    }
    return true;
}

bool QGeoCodeParser::parsePlace(QGeoPlace *place)
{
    Q_ASSERT(m_reader->isStartElement() && (m_reader->name() == "place" || m_reader->name() == "addressparts"));

    if (m_reader->name() == "place") {
        QGeoBoundingBox bounds;
        if (!parseBoundingBox(&bounds,m_reader->attributes().value("boundingbox").toString()))
            return false;
        place->setViewport(bounds);
        place->setCoordinate(
                QGeoCoordinate(m_reader->attributes().value("lat").toString().toDouble(),
                               m_reader->attributes().value("lon").toString().toDouble()) );
    }

    QGeoAddress address;
    m_reader->readNext();
    while (!(m_reader->tokenType() == QXmlStreamReader::EndElement &&  (m_reader->name() == "place" || m_reader->name() == "addressparts"))) {
        if (m_reader->tokenType() == QXmlStreamReader::StartElement) {
            if (m_reader->name() == "house") {
            } else if (m_reader->name() == "road") {
                address.setStreet(m_reader->readElementText());
            } else if (m_reader->name() == "village") {
                address.setDistrict(m_reader->readElementText());
            } else if (m_reader->name() == "state") {
                address.setState(m_reader->readElementText());
            } else if (m_reader->name() == "town") {
            } else if (m_reader->name() == "city") {
                address.setCity(m_reader->readElementText());
            } else if (m_reader->name() == "county") {
                address.setCounty(m_reader->readElementText());
            } else if (m_reader->name() == "postcode") {
                address.setPostcode(m_reader->readElementText());
            } else if (m_reader->name() == "country") {
                address.setCountry(m_reader->readElementText());
            } else if (m_reader->name() == "country_code") {
                address.setCountryCode(m_reader->readElementText());
            } else {
                m_reader->skipCurrentElement();
            }
        }
        m_reader->readNext();
    }
    place->setAddress(address);
    return true;
}

bool QGeoCodeParser::parseBoundingBox(QGeoBoundingBox *bounds, QString str)
{
    QStringList list = str.split(",");
    if (list.size() != 4) {
        return false;
    }
    bounds->setBottomLeft(QGeoCoordinate(list[0].toDouble(),list[2].toDouble()));
    bounds->setTopRight(QGeoCoordinate(list[1].toDouble(),list[3].toDouble()));

    return true;
}

