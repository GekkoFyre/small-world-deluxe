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

#include "src/models/tableview/gk_settings_dlg_sdr_devices.hpp"
#include <utility>
#include <QDateTime>
#include <QVBoxLayout>
#include <QHeaderView>

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;
using namespace AmateurRadio;
using namespace Control;
using namespace Spectrograph;
using namespace System;
using namespace Events;
using namespace Logging;
using namespace GkSdr;

/**
 * @brief GkEventLoggerTableViewModel::GkEventLoggerTableViewModel
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param database
 * @param parent
 */
GkSettingsDlgSdrDevs::GkSettingsDlgSdrDevs(QWidget *parent)
    : QAbstractTableModel(parent)
{
    table = new QTableView(parent);
    QPointer<QVBoxLayout> layout = new QVBoxLayout(parent);
    proxyModel = new QSortFilterProxyModel(parent);
    proxyModel->setSourceModel(this);

    return;
}

/**
 * @brief GkSettingsDlgSdrDevs::~GkSettingsDlgSdrDevs
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
GkSettingsDlgSdrDevs::~GkSettingsDlgSdrDevs()
{
    return;
}

/**
 * @brief GkSettingsDlgSdrDevs::populateData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param sdr_devs
 */
void GkSettingsDlgSdrDevs::populateData(const QList<GkSoapySdrTableView> sdr_devs)
{
    dataBatchMutex.lock();

    if (sdr_devs.isEmpty()) {
        QList<GkSoapySdrTableView> noDevsFound;
        GkSoapySdrTableView sdr_none_found;
        sdr_none_found.event_no = 0;
        sdr_none_found.dev_name = tr("No devices found.");
        sdr_none_found.dev_hw_key = "";
        noDevsFound.push_back(sdr_none_found);

        m_data = noDevsFound;
        return;
    }

    m_data.clear();
    m_data = sdr_devs;

    dataBatchMutex.unlock();
    return;
}

/**
 * @brief GkSettingsDlgSdrDevs::insertData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param sdr_dev
 */
void GkSettingsDlgSdrDevs::insertData(const GkSoapySdrTableView &sdr_dev)
{
    dataBatchMutex.lock();

    beginInsertRows(QModelIndex(), m_data.count(), m_data.count());
    m_data.append(sdr_dev);
    endInsertRows();

    auto top = this->createIndex((m_data.count() - 1), 0, nullptr);
    auto bottom = this->createIndex((m_data.count() - 1), GK_TABLEVIEW_SOAPYSDR_DEVICE_TOTAL_IDX, nullptr);
    emit dataChanged(top, bottom);

    dataBatchMutex.unlock();
    return;
}

/**
 * @brief GkSettingsDlgSdrDevs::removeData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param sdr_dev
 */
void GkSettingsDlgSdrDevs::removeData(const GkSoapySdrTableView &sdr_dev)
{
    dataBatchMutex.lock();

    for (int i = 0; i < m_data.size(); ++i) {
        if (m_data[i].event_no == sdr_dev.event_no) {
            beginRemoveRows(QModelIndex(), (m_data.count() - 1), (m_data.count() - 1));
            m_data.removeAt(i); // Remove any occurrence of this value, one at a time!
            endRemoveRows();
        }
    }

    auto top = this->createIndex((m_data.count() - 1), 0, nullptr);
    auto bottom = this->createIndex((m_data.count() - 1), GK_TABLEVIEW_SOAPYSDR_DEVICE_TOTAL_IDX, nullptr);
    emit dataChanged(top, bottom);

    dataBatchMutex.unlock();
    return;
}

/**
 * @brief GkSettingsDlgSdrDevs::rowCount
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 * @return
 */
qint32 GkSettingsDlgSdrDevs::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_data.length();
}

/**
 * @brief GkSettingsDlgSdrDevs::columnCount
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 * @return
 */
qint32 GkSettingsDlgSdrDevs::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return GK_TABLEVIEW_SOAPYSDR_DEVICE_TOTAL_IDX; // Make sure to add the total of columns from within `GkSettingsDlgSdrDevs::headerData()`!
}

/**
 * @brief GkSettingsDlgSdrDevs::data
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 * @param role
 * @return
 */
QVariant GkSettingsDlgSdrDevs::data(const QModelIndex &index, qint32 role) const
{


    if (!index.isValid() || role != Qt::DisplayRole) {
        return QVariant();
    }

    const QString dev_name = m_data[index.row()].dev_name;
    QString dev_hw_key = m_data[index.row()].dev_hw_key;

    if (dev_hw_key.isEmpty()) {
        dev_hw_key = tr("N/A.");
    }

    switch (index.column()) {
        case GK_TABLEVIEW_SOAPYSDR_DEVICE_NAME_IDX:
            return dev_name;
        case GK_TABLEVIEW_SOAPYSDR_DEVICE_HWARE_KEY_IDX:
            return dev_hw_key;
    }

    return QVariant();
}

/**
 * @brief GkSettingsDlgSdrDevs::headerData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param section
 * @param orientation
 * @param role
 * @return
 */
QVariant GkSettingsDlgSdrDevs::headerData(qint32 section, Qt::Orientation orientation, qint32 role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
            case GK_TABLEVIEW_SOAPYSDR_DEVICE_NAME_IDX:
                return tr("SDR Device");
            case GK_TABLEVIEW_SOAPYSDR_DEVICE_HWARE_KEY_IDX:
                return tr("H/W Key");
        }
    }

    return QVariant();
}
