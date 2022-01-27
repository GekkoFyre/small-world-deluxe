/**
 **     __                 _ _   __    __           _     _ 
 **    / _\_ __ ___   __ _| | | / / /\ \ \___  _ __| | __| |
 **    \ \| '_ ` _ \ / _` | | | \ \/  \/ / _ \| '__| |/ _` |
 **    _\ \ | | | | | (_| | | |  \  /\  / (_) | |  | | (_| |
 **    \__/_| |_| |_|\__,_|_|_|   \/  \/ \___/|_|  |_|\__,_|
 **                                                         
 **                  ___     _                              
 **                 /   \___| |_   ___  _____               
 **                / /\ / _ \ | | | \ \/ / _ \              
 **               / /_//  __/ | |_| |>  <  __/              
 **              /___,' \___|_|\__,_/_/\_\___|              
 **
 **
 **   If you have downloaded the source code for "Small World Deluxe" and are reading this,
 **   then thank you from the bottom of our hearts for making use of our hard work, sweat
 **   and tears in whatever you are implementing this into!
 **
 **   Copyright (C) 2020 - 2022. GekkoFyre.
 **
 **   Small World Deluxe is free software: you can redistribute it and/or modify
 **   it under the terms of the GNU General Public License as published by
 **   the Free Software Foundation, either version 3 of the License, or
 **   (at your option) any later version.
 **
 **   Small world is distributed in the hope that it will be useful,
 **   but WITHOUT ANY WARRANTY; without even the implied warranty of
 **   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **   GNU General Public License for more details.
 **
 **   You should have received a copy of the GNU General Public License
 **   along with Small World Deluxe.  If not, see <http://www.gnu.org/licenses/>.
 **
 **
 **   The latest source code updates can be obtained from [ 1 ] below at your
 **   discretion. A web-browser or the 'git' application may be required.
 **
 **   [ 1 ] - https://code.gekkofyre.io/amateur-radio/small-world-deluxe
 **
 ****************************************************************************************************/

#include "src/models/tableview/gk_qdatetime_filter_proxy_model.hpp"

using namespace GekkoFyre;
using namespace GkAudioFramework;
using namespace Database;
using namespace Settings;
using namespace Audio;
using namespace AmateurRadio;
using namespace Control;
using namespace Spectrograph;
using namespace System;
using namespace Events;
using namespace Logging;
using namespace Network;
using namespace GkXmpp;
using namespace Security;

/**
 * @brief GkQDateTimeFilterProxyModel::GkQDateTimeFilterProxyModel
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 */
GkQDateTimeFilterProxyModel::GkQDateTimeFilterProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    setParent(parent);
    return;
}

/**
 * @brief GkQDateTimeFilterProxyModel::~GkQDateTimeFilterProxyModel
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
GkQDateTimeFilterProxyModel::~GkQDateTimeFilterProxyModel()
{
    return;
}

/**
 * @brief GkQDateTimeFilterProxyModel::setFilterMinimumDateTime
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param timestamp
 */
void GkQDateTimeFilterProxyModel::setFilterMinimumDateTime(QDateTime timestamp)
{
    if (timestamp.isValid()) {
        minTimestamp = std::move(timestamp);
        invalidateFilter();
    }

    return;
}

/**
 * @brief GkQDateTimeFilterProxyModel::setFilterMaximumDateTime
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param timestamp
 */
void GkQDateTimeFilterProxyModel::setFilterMaximumDateTime(QDateTime timestamp)
{
    if (timestamp.isValid()) {
        maxTimestamp = std::move(timestamp);
        invalidateFilter();
    }

    return;
}

/**
 * @brief GkQDateTimeFilterProxyModel::filterAcceptsRow
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param sourceRow
 * @param sourceParent
 * @return
 */
bool GkQDateTimeFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}

/**
 * @brief GkQDateTimeFilterProxyModel::lessThan
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param left
 * @param right
 * @return
 */
bool GkQDateTimeFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    const QVariant leftData = sourceModel()->data(left);
    const QVariant rightData = sourceModel()->data(right);
    if (leftData.userType() == QMetaType::QDateTime) {
        return leftData.toDateTime() < rightData.toDateTime();
    }

    const QDateTime leftTimestamp = leftData.toDateTime();
    const QDateTime rightTimestamp = rightData.toDateTime();

    return QString::localeAwareCompare(QString::number(leftTimestamp.toMSecsSinceEpoch()), QString::number(rightTimestamp.toMSecsSinceEpoch())) < 0;
}

/**
 * @brief GkQDateTimeFilterProxyModel::dateInRange
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param timestamp
 * @return
 */
bool GkQDateTimeFilterProxyModel::dateInRange(QDateTime timestamp) const
{
    return (!minTimestamp.isValid() || timestamp > minTimestamp) && (!maxTimestamp.isValid() || timestamp < maxTimestamp);
}
