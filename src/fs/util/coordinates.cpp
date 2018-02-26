/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#include "fs/util/coordinates.h"

#include "geo/pos.h"
#include "atools.h"

#include <cmath>
#include <QRegularExpression>

using atools::geo::Pos;

namespace atools {
namespace fs {
namespace util {

// N48194W123096
const static QString COORDS_FLIGHTPLAN_FORMAT_GFP("%1%2%3%4%5%6");
const static QRegularExpression LONG_FORMAT_REGEXP_GFP("^([NS])([0-9]{2})([0-9]{3})"
                                                       "([EW])([0-9]{3})([0-9]{3})$");

// 46N078W
const static QRegularExpression LONG_FORMAT_REGEXP_DEG("^([0-9]{2})([NS])"
                                                       "([0-9]{3})([EW])$");

// 4510N06810W
const static QString COORDS_FLIGHTPLAN_FORMAT_DEG_MIN("%1%2%3%4%5%6");
const static QRegularExpression LONG_FORMAT_REGEXP_DEG_MIN("^([0-9]{2})([0-9]{2})([NS])"
                                                           "([0-9]{3})([0-9]{2})([EW])$");

// 481200N0112842E
const static QString COORDS_FLIGHTPLAN_FORMAT_DEG_MIN_SEC("%1%2%3%4%5%6%7%8");
const static QRegularExpression LONG_FORMAT_REGEXP_DEG_MIN_SEC("^([0-9]{2})([0-9]{2})([0-9]{2})([NS])"
                                                               "([0-9]{3})([0-9]{2})([0-9]{2})([EW])$");

// Pattern allows trailing garbage
// 50:40:42 N 003:13:30 E
// 31:30:00 N 086:44:59 W
static QRegularExpression MATCH_COORD_OPENAIR_MIN_SEC("^([\\d]+):([\\d]+):([\\d\\.]+)\\s*([NS])\\s*"
                                                      "([\\d]+):([\\d]+):([\\d\\.]+)\\s*([EW])");

// Pattern allows trailing garbage
// 39:06.2 N 121:35.5 W
static QRegularExpression MATCH_COORD_OPENAIR_MIN("^([\\d]+):([\\d\\.]+)\\s*([NS])\\s*"
                                                  "([\\d]+):([\\d\\.]+)\\s*([EW])");

// 5020N
const static QRegularExpression LONG_FORMAT_REGEXP_NAT("^([0-9]{2})"
                                                       "([0-9]{2})N$");

// N6400 W07000 or N6400/W07000
const static QString COORDS_FLIGHTPLAN_FORMAT_PAIR("%1%2/%3%4");
const static QRegularExpression LONG_FORMAT_REGEXP_PAIR("^([NS])([0-9]{2})([0-9]{2})[ /]"
                                                        "([EW])([0-9]{3})([0-9]{2})$");
const static QRegularExpression LONG_FORMAT_REGEXP_PAIR2("^([0-9]{2})([0-9]{2})([NS])[ /]"
                                                         "([0-9]{3})([0-9]{2})([EW])$");
const static QRegularExpression LONG_FORMAT_REGEXP_PAIR_LAT("^([NS])([0-9]{2})([0-9]{2})$");
const static QRegularExpression LONG_FORMAT_REGEXP_PAIR_LON("^([EW])([0-9]{3})([0-9]{2})$");

atools::geo::Pos degMinSecFormatFromCapture(const QStringList& captured);
atools::geo::Pos degMinFormatFromCapture(const QStringList& captured);

// N48194W123096
// Examples:
// :F:N44124W122451 (User Waypoint: N44° 12.4' W122° 45.1'
// :F:N14544W017479 (User Waypoint: N14° 54.4' W17° 47.9'
// :F:S31240E136502 (User Waypoint: S31° 24.0' E136° 50.2'
QString toGfpFormat(const atools::geo::Pos& pos)
{
  if(pos.isValid())
    return COORDS_FLIGHTPLAN_FORMAT_GFP.
           arg(pos.getLatY() > 0.f ? "N" : "S").
           arg(atools::absInt(pos.getLatYDeg()), 2, 10, QChar('0')).
           arg(std::abs(pos.getLatYMin() + pos.getLatYSec() / 60.f) * 10.f, 3, 'f', 0, QChar('0')).
           arg(pos.getLonX() > 0.f ? "E" : "W").
           arg(atools::absInt(pos.getLonXDeg()), 3, 10, QChar('0')).
           arg(std::abs(pos.getLonXMin() + pos.getLonXSec() / 60.f) * 10.f, 3, 'f', 0, QChar('0'));
  else
    return QString();
}

// 4510N06810W
QString toDegMinFormat(const atools::geo::Pos& pos)
{
  if(pos.isValid())
    return COORDS_FLIGHTPLAN_FORMAT_DEG_MIN.
           arg(atools::absInt(pos.getLatYDeg()), 2, 10, QChar('0')).
           arg(std::abs(pos.getLatYMin() + pos.getLatYSec() / 60.f), 2, 'f', 0, QChar('0')).
           arg(pos.getLatY() > 0.f ? "N" : "S").
           arg(atools::absInt(pos.getLonXDeg()), 3, 10, QChar('0')).
           arg(std::abs(pos.getLonXMin() + pos.getLonXSec() / 60.f), 2, 'f', 0, QChar('0')).
           arg(pos.getLonX() > 0.f ? "E" : "W");
  else
    return QString();
}

// Garmin format N48194W123096
atools::geo::Pos fromGfpFormat(const QString& str)
{
  QRegularExpressionMatch match = LONG_FORMAT_REGEXP_GFP.match(str.simplified().toUpper());

  if(match.hasMatch())
  {
    QStringList captured = match.capturedTexts();

    if(captured.size() == 7)
    {
      bool latOk, lonOk, latMinOk, lonMinOk;
      QString ns = captured.at(1);
      int latYDeg = captured.at(2).toInt(&latOk);
      float latYMin = captured.at(3).toFloat(&latMinOk) / 10.f;
      float latYSec = (latYMin - std::floor(latYMin)) * 60.f;

      QString ew = captured.at(4);
      int lonXDeg = captured.at(5).toInt(&lonOk);
      float lonXMin = captured.at(6).toFloat(&lonMinOk) / 10.f;
      float lonXSec = (lonXMin - std::floor(lonXMin)) * 60.f;

      if(latOk && lonOk && latMinOk && lonMinOk &&
         -90 <= latYDeg && latYDeg <= 90 &&
         -180 <= lonXDeg && lonXDeg <= 180)
        return atools::geo::Pos(lonXDeg, static_cast<int>(lonXMin), lonXSec, ew == "W",
                                latYDeg, static_cast<int>(latYMin), latYSec, ns == "S");
    }
  }
  return atools::geo::EMPTY_POS;
}

// Degrees only 46N078W
atools::geo::Pos fromDegFormat(const QString& str)
{
  QRegularExpressionMatch match = LONG_FORMAT_REGEXP_DEG.match(str.simplified().toUpper());

  if(match.hasMatch())
  {
    QStringList captured = match.capturedTexts();

    if(captured.size() == 5)
    {
      bool latOk, lonOk;
      int latYDeg = captured.at(1).toInt(&latOk);
      QString ns = captured.at(2);

      int lonXDeg = captured.at(3).toInt(&lonOk);
      QString ew = captured.at(4);

      if(latOk && lonOk &&
         -90.f <= latYDeg && latYDeg <= 90.f &&
         -180.f <= lonXDeg && lonXDeg <= 180.f)
        return atools::geo::Pos(lonXDeg, 0, 0.f, ew == "W",
                                latYDeg, 0, 0.f, ns == "S");
    }
  }
  return atools::geo::EMPTY_POS;
}

// Degrees and minutes 4510N06810W
atools::geo::Pos fromDegMinFormat(const QString& str)
{
  QRegularExpressionMatch match = LONG_FORMAT_REGEXP_DEG_MIN.match(str.simplified().toUpper());

  if(match.hasMatch())
  {
    QStringList captured = match.capturedTexts();

    if(captured.size() == 7)
    {
      bool latOk, lonOk, latMinOk, lonMinOk;
      int latYDeg = captured.at(1).toInt(&latOk);
      int latYMin = captured.at(2).toInt(&latMinOk);
      QString ns = captured.at(3);

      int lonXDeg = captured.at(4).toInt(&lonOk);
      int lonXMin = captured.at(5).toInt(&lonMinOk);
      QString ew = captured.at(6);

      if(latOk && lonOk && latMinOk && lonMinOk &&
         -90 <= latYDeg && latYDeg <= 90 &&
         -180 <= lonXDeg && lonXDeg <= 180)
        return atools::geo::Pos(lonXDeg, lonXMin, 0.f, ew == "W",
                                latYDeg, latYMin, 0.f, ns == "S");
    }
  }

  return atools::geo::EMPTY_POS;
}

// Degrees, minutes and seconds 481200N0112842E
atools::geo::Pos fromDegMinSecFormat(const QString& str)
{
  QRegularExpressionMatch match = LONG_FORMAT_REGEXP_DEG_MIN_SEC.match(str.simplified().toUpper());

  if(match.hasMatch())
    return degMinSecFormatFromCapture(match.capturedTexts());

  return atools::geo::EMPTY_POS;
}

// Degrees and minutes in pair N6400 W07000 or N6400/W07000
atools::geo::Pos fromDegMinPairFormat(const QString& str)
{
  QRegularExpressionMatch match = LONG_FORMAT_REGEXP_PAIR.match(str.simplified().toUpper());

  bool latOk = false, lonOk = false, latMinOk = false, lonMinOk = false;
  QString ns, ew;
  int latYDeg = 0, latYMin = 0, lonXDeg = 0, lonXMin = 0;

  if(match.hasMatch())
  {
    QStringList captured = match.capturedTexts();
    if(captured.size() == 7)
    {
      ns = captured.at(1);
      latYDeg = captured.at(2).toInt(&latOk);
      latYMin = captured.at(3).toInt(&latMinOk);
      ew = captured.at(4);
      lonXDeg = captured.at(5).toInt(&lonOk);
      lonXMin = captured.at(6).toInt(&lonMinOk);
    }
  }
  else
  {
    QRegularExpressionMatch match2 = LONG_FORMAT_REGEXP_PAIR2.match(str.simplified().toUpper());

    if(match2.hasMatch())
    {
      QStringList captured = match2.capturedTexts();
      if(captured.size() == 7)
      {
        latYDeg = captured.at(1).toInt(&latOk);
        latYMin = captured.at(2).toInt(&latMinOk);
        ns = captured.at(3);
        lonXDeg = captured.at(4).toInt(&lonOk);
        lonXMin = captured.at(5).toInt(&lonMinOk);
        ew = captured.at(6);
      }
    }
  }

  if(latOk && lonOk && latMinOk && lonMinOk &&
     -90 <= latYDeg && latYDeg <= 90 &&
     -180 <= lonXDeg && lonXDeg <= 180)
    return atools::geo::Pos(lonXDeg, lonXMin, 0.f, ew == "W",
                            latYDeg, latYMin, 0.f, ns == "S");
  else
    return atools::geo::EMPTY_POS;
}

// NAT type 5020N
// first two figures are the latitude north and the second two figures are the longitude west
atools::geo::Pos fromNatFormat(const QString& str)
{
  QRegularExpressionMatch match = LONG_FORMAT_REGEXP_NAT.match(str.simplified().toUpper());

  if(match.hasMatch())
  {
    QStringList captured = match.capturedTexts();

    if(captured.size() == 3)
    {
      bool latOk, lonOk;
      int latYDeg = captured.at(1).toInt(&latOk);
      int lonXDeg = captured.at(2).toInt(&lonOk);

      if(latOk && lonOk &&
         0 <= latYDeg && latYDeg <= 90 &&
         0 <= lonXDeg && lonXDeg <= 180)
        return atools::geo::Pos(lonXDeg, 0, 0.f, true, latYDeg, 0, 0.f, false);
    }
  }
  return atools::geo::EMPTY_POS;
}

atools::geo::Pos fromAnyWaypointFormat(const QString& str)
{
  if(str.size() == 15)
    // Skyvector 481050N0113157E (N48°10.83' E11°31.96')
    return fromDegMinSecFormat(str);
  else if(str.size() == 13)
    // Garmin format N48194W123096
    return fromGfpFormat(str);
  else if(str.size() == 12)
    // "N6400 W07000" or "N6400/W07000"
    return fromDegMinPairFormat(str);
  else if(str.size() == 7)
    // Degrees only 46N078W
    return fromDegFormat(str);
  else if(str.size() == 11)
    // Degrees and minutes 4510N06810W
    return fromDegMinFormat(str);
  else if(str.size() == 5)
    // NAT type 5020N
    return fromNatFormat(str);

  return atools::geo::EMPTY_POS;
}

geo::Pos fromOpenAirFormat(const QString& coordStr)
{
  QRegularExpressionMatch match = MATCH_COORD_OPENAIR_MIN_SEC.match(coordStr.toUpper());
  if(match.hasMatch())
    return degMinSecFormatFromCapture(match.capturedTexts());
  else
  {
    match = MATCH_COORD_OPENAIR_MIN.match(coordStr);
    if(match.hasMatch())
      return degMinFormatFromCapture(match.capturedTexts());
  }

  return atools::geo::EMPTY_POS;
}

atools::geo::Pos degMinFormatFromCapture(const QStringList& captured)
{
  if(captured.size() == 7)
  {
    bool latOk, lonOk, latMinOk, lonMinOk;
    int latYDeg = captured.at(1).toInt(&latOk);
    float latYMin = captured.at(2).toFloat(&latMinOk);
    QString ns = captured.at(3);

    int lonXDeg = captured.at(4).toInt(&lonOk);
    float lonXMin = captured.at(5).toFloat(&lonMinOk);
    QString ew = captured.at(6);

    if(latOk && lonOk && latMinOk && lonMinOk &&
       -90 <= latYDeg && latYDeg <= 90 &&
       -180 <= lonXDeg && lonXDeg <= 180)
      return atools::geo::Pos((lonXDeg + lonXMin / 60.f) * (ew == "W" ? -1.f : 1.f),
                              (latYDeg + latYMin / 60.f) * (ns == "S" ? -1.f : 1.f));
  }
  return atools::geo::EMPTY_POS;
}

atools::geo::Pos degMinSecFormatFromCapture(const QStringList& captured)
{
  if(captured.size() == 9)
  {
    bool latOk, lonOk, latMinOk, lonMinOk, latSecOk, lonSecOk;
    int latYDeg = captured.at(1).toInt(&latOk);
    int latYMin = captured.at(2).toInt(&latMinOk);
    float latYSec = captured.at(3).toFloat(&latSecOk);
    QString ns = captured.at(4);

    int lonXDeg = captured.at(5).toInt(&lonOk);
    int lonXMin = captured.at(6).toInt(&lonMinOk);
    float lonXSec = captured.at(7).toFloat(&lonSecOk);
    QString ew = captured.at(8);

    if(latOk && lonOk && latMinOk && lonMinOk && latSecOk && lonSecOk &&
       -90 <= latYDeg && latYDeg <= 90 &&
       -180 <= lonXDeg && lonXDeg <= 180)
      return atools::geo::Pos(lonXDeg, lonXMin, lonXSec, ew == "W",
                              latYDeg, latYMin, latYSec, ns == "S");
  }
  return atools::geo::EMPTY_POS;
}

geo::Pos fromAnyFormat(const QString& coords)
{
  float lonX = 0.f, latY = 0.f;

  // Convert local dependent decimal point to dot
  QString coordStr(coords.simplified().toUpper().replace(QLocale().decimalPoint(), "."));

  // N49° 26' 41.57" E9° 12' 5.49"
  static const QRegularExpression FORMAT_DEG_MIN_SEC_REGEXP(
    "([NS])\\s*([0-9]+)\\s*°\\s*([0-9]+)\\s*'\\s*([0-9\\.]+)\\s*\"\\s*"
    "([EW])\\s*([0-9]+)\\s*°\\s*([0-9]+)\\s*'\\s*([0-9\\.]+)\\s*\"");

  // N54* 16.82' W008* 35.95'
  static const QRegularExpression FORMAT_DEG_MIN_REGEXP(
    "([NS])\\s*([0-9]+)\\s*[\\*°]\\s*([0-9\\.]+)\\s*[']?\\s*'\\s*"
    "([EW])\\s*([0-9]+)\\s*[\\*°]\\s*([0-9\\.]+)\\s*[']?\\s*'");

  // N 52 33.58 E 13 17.26
  static const QRegularExpression FORMAT_DEG_MIN2_REGEXP(
    "([NS])\\s*([0-9]+)\\s+([0-9\\.]+)\\s*"
    "([EW])\\s*([0-9]+)\\s+([0-9\\.]+)");

  // 49° 26' 41,57" N 9° 12' 5,49" E
  static const QRegularExpression FORMAT_DEG_MIN_SEC_REGEXP2(
    "([0-9]+)\\s*°\\s*([0-9]+)\\s*'\\s*([0-9\\.]+)\\s*\"\\s*([NS])\\s*"
    "([0-9]+)\\s*°\\s*([0-9]+)\\s*'\\s*([0-9\\.]+)\\s*\"\\s*([EW])");

  // 49° 26,69' N 9° 12,09' E
  static const QRegularExpression FORMAT_DEG_MIN3_REGEXP(
    "([0-9]+)\\s*°\\s*([0-9\\.]+)\\s*'\\s*([NS])\\s*"
    "([0-9]+)\\s*°\\s*([0-9\\.]+)\\s*'\\s*([EW])");

  // 49,4449° N 9,2015° E
  static const QRegularExpression FORMAT_DEG_REGEXP(
    "([0-9\\.]+)\\s*°?\\s*([NS])\\s*"
    "([0-9\\.]+)\\s*°?\\s*([EW])");

  // N 49,4449° E 9,2015°
  static const QRegularExpression FORMAT_DEG2_REGEXP(
    "([NS])\\s*([0-9\\.]+)\\s*°?\\s*"
    "([EW])\\s*([0-9\\.]+)\\s*°?");

  // N49° 26' 41.57" E9° 12' 5.49"
  QRegularExpressionMatch match = FORMAT_DEG_MIN_SEC_REGEXP.match(coordStr);
  if(match.hasMatch())
  {
    QString ns = match.captured(1);
    int latYDeg = match.captured(2).toInt();
    int latYMin = match.captured(3).toInt();
    float latYSec = match.captured(4).toFloat();

    QString ew = match.captured(5);
    int lonXDeg = match.captured(6).toInt();
    int lonXMin = match.captured(7).toInt();
    float lonXSec = match.captured(8).toFloat();

    latY = (latYDeg + latYMin / 60.f + latYSec / 3600.f) * (ns == "S" ? -1.f : 1.f);
    lonX = (lonXDeg + lonXMin / 60.f + lonXSec / 3600.f) * (ew == "W" ? -1.f : 1.f);
    Pos pos(lonX, latY);
    if(pos.isValidRange())
      return pos;
  }

  // N54* 16.82' W008* 35.95'
  match = FORMAT_DEG_MIN_REGEXP.match(coordStr);
  if(match.hasMatch())
  {
    QString ns = match.captured(1);
    int latYDeg = match.captured(2).toInt();
    float latYMin = match.captured(3).toFloat();

    QString ew = match.captured(4);
    int lonXDeg = match.captured(5).toInt();
    float lonXMin = match.captured(6).toFloat();

    latY = (latYDeg + latYMin / 60.f) * (ns == "S" ? -1.f : 1.f);
    lonX = (lonXDeg + lonXMin / 60.f) * (ew == "W" ? -1.f : 1.f);
    Pos pos(lonX, latY);
    if(pos.isValidRange())
      return pos;
  }

  // N 52 33.58 E 13 17.26
  match = FORMAT_DEG_MIN2_REGEXP.match(coordStr);
  if(match.hasMatch())
  {
    QString ns = match.captured(1);
    int latYDeg = match.captured(2).toInt();
    float latYMin = match.captured(3).toFloat();

    QString ew = match.captured(4);
    int lonXDeg = match.captured(5).toInt();
    float lonXMin = match.captured(6).toFloat();

    latY = (latYDeg + latYMin / 60.f) * (ns == "S" ? -1.f : 1.f);
    lonX = (lonXDeg + lonXMin / 60.f) * (ew == "W" ? -1.f : 1.f);
    Pos pos(lonX, latY);
    if(pos.isValidRange())
      return pos;
  }

  // 49° 26' 41,57" N 9° 12' 5,49" E
  match = FORMAT_DEG_MIN_SEC_REGEXP2.match(coordStr);
  if(match.hasMatch())
  {
    int latYDeg = match.captured(1).toInt();
    int latYMin = match.captured(2).toInt();
    float latYSec = match.captured(3).toFloat();
    QString ns = match.captured(4);

    int lonXDeg = match.captured(5).toInt();
    int lonXMin = match.captured(6).toInt();
    float lonXSec = match.captured(7).toFloat();
    QString ew = match.captured(8);

    latY = (latYDeg + latYMin / 60.f + latYSec / 3600.f) * (ns == "S" ? -1.f : 1.f);
    lonX = (lonXDeg + lonXMin / 60.f + lonXSec / 3600.f) * (ew == "W" ? -1.f : 1.f);
    Pos pos(lonX, latY);
    if(pos.isValidRange())
      return pos;
  }

  // 49° 26,69' N 9° 12,09' E
  match = FORMAT_DEG_MIN3_REGEXP.match(coordStr);
  if(match.hasMatch())
  {
    int latYDeg = match.captured(1).toInt();
    float latYMin = match.captured(2).toFloat();
    QString ns = match.captured(3);

    int lonXDeg = match.captured(4).toInt();
    float lonXMin = match.captured(5).toFloat();
    QString ew = match.captured(6);

    latY = (latYDeg + latYMin / 60.f) * (ns == "S" ? -1.f : 1.f);
    lonX = (lonXDeg + lonXMin / 60.f) * (ew == "W" ? -1.f : 1.f);
    Pos pos(lonX, latY);
    if(pos.isValidRange())
      return pos;
  }

  // 49,4449° N 9,2015° E
  match = FORMAT_DEG_REGEXP.match(coordStr);
  if(match.hasMatch())
  {
    float latYDeg = match.captured(1).toFloat();
    QString ns = match.captured(2);

    float lonXDeg = match.captured(3).toFloat();
    QString ew = match.captured(4);

    latY = latYDeg * (ns == "S" ? -1.f : 1.f);
    lonX = lonXDeg * (ew == "W" ? -1.f : 1.f);
    Pos pos(lonX, latY);
    if(pos.isValidRange())
      return pos;
  }

  // N 49,4449° E 9,2015°
  match = FORMAT_DEG2_REGEXP.match(coordStr);
  if(match.hasMatch())
  {
    QString ns = match.captured(1);
    float latYDeg = match.captured(2).toFloat();

    QString ew = match.captured(3);
    float lonXDeg = match.captured(4).toFloat();

    latY = latYDeg * (ns == "S" ? -1.f : 1.f);
    lonX = lonXDeg * (ew == "W" ? -1.f : 1.f);
    Pos pos(lonX, latY);
    if(pos.isValidRange())
      return pos;
  }

  return fromAnyWaypointFormat(coordStr);
}

} // namespace util
} // namespace fs
} // namespace atools
