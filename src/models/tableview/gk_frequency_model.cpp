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
 **   Copyright (C) 2020. GekkoFyre.
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
 **   [ 1 ] - https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe
 **
 ****************************************************************************************************/

#include "src/models/tableview/gk_frequency_model.hpp"
#include <utility>
#include <iomanip>
#include <sstream>
#include <QAction>
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

GkFreqTableViewModel::GkFreqTableViewModel(std::shared_ptr<GekkoFyre::GkLevelDb> database, QWidget *parent)
    : QAbstractTableModel(parent)
{
    GkDb = std::move(database);

    table = new QTableView(parent);
    QPointer<QVBoxLayout> layout = new QVBoxLayout(parent);
    proxyModel = new QSortFilterProxyModel(parent);

    table->setModel(proxyModel);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(table, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customMenuRequested(QPoint)));

    table->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(table->horizontalHeader(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customHeaderMenuRequested(QPoint)));
    layout->addWidget(table);

    // table->horizontalHeader()->setSectionResizeMode(GK_FREQ_TABLEVIEW_MODEL_FREQUENCY_IDX, QHeaderView::Stretch);

    menu = new QMenu(parent);
    menu->addAction(new QAction(tr("New"), this));
    menu->addAction(new QAction(tr("Edit"), this));
    menu->addAction(new QAction(tr("Delete"), this));

    proxyModel->setSourceModel(this);

    return;
}

GkFreqTableViewModel::~GkFreqTableViewModel()
{
    return;
}

/**
 * @brief GkFreqTableViewModel::populateData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param frequencies
 */
void GkFreqTableViewModel::populateData(const QList<GkFreqs> &frequencies)
{
    m_data.clear();
    m_data = frequencies;

    return;
}

/**
 * @brief GkFreqTableViewModel::populateData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param frequencies
 * @param populate_freq_db
 */
void GkFreqTableViewModel::populateData(const QList<GkFreqs> &frequencies, const bool &populate_freq_db)
{
    populateData(frequencies);

    if (populate_freq_db) {
        for (const auto &data: frequencies) {
            emit addFreq(data);
        }
    }

    return;
}

/**
 * @brief GkFreqTableViewModel::insertData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param freq_val
 */
void GkFreqTableViewModel::insertData(const GkFreqs &freq_val)
{
    m_data.push_back(freq_val);

    return;
}

/**
 * @brief GkFreqTableViewModel::insertData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param freq_val
 * @param populate_freq_db
 */
void GkFreqTableViewModel::insertData(const GkFreqs &freq_val, const bool &populate_freq_db)
{
    insertData(freq_val);

    if (populate_freq_db) {
        emit addFreq(freq_val);
    }

    return;
}

/**
 * @brief GkFreqTableViewModel::removeData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param freq_val
 */
void GkFreqTableViewModel::removeData(const GkFreqs &freq_val)
{
    for (int i = 0; i < m_data.size(); ++i) {
        if ((m_data[i].frequency == freq_val.frequency) && ((m_data[i].digital_mode == freq_val.digital_mode) ||
                                                            (m_data[i].iaru_region == freq_val.iaru_region))) {
            m_data.removeAt(i); // Remove any occurrence of this value, one at a time!
        }
    }

    return;
}

/**
 * @brief GkFreqTableViewModel::removeData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param freq_val
 * @param remove_freq_db
 */
void GkFreqTableViewModel::removeData(const GkFreqs &freq_val, const bool &remove_freq_db)
{
    if (remove_freq_db) {
        for (int i = 0; i < m_data.size(); ++i) {
            if ((m_data[i].frequency == freq_val.frequency) && ((m_data[i].digital_mode == freq_val.digital_mode) ||
                                                                (m_data[i].iaru_region == freq_val.iaru_region))) {
                emit removeFreq(freq_val);
            }
        }
    }

    removeData(freq_val);

    return;
}

/**
 * @brief GkFreqTableViewModel::rowCount
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 * @return
 */
int GkFreqTableViewModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_data.length();
}

/**
 * @brief GkFreqTableViewModel::columnCount
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 * @return
 * @note BE SURE TO GET THE TOTAL COUNT from the function, `GkFreqTableViewModel::headerData()`!
 * @see GkFreqTableViewModel::headerData()
 */
int GkFreqTableViewModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return 3; // Make sure to add the total of columns from within `GkFreqTableViewModel::headerData()`!
}

/**
 * @brief GkFreqTableViewModel::data
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 * @param role
 * @return
 */
QVariant GkFreqTableViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole) {
        return QVariant();
    }

    double row_freq = m_data[index.row()].frequency;
    quint64 num_base_10 = m_data[index.row()].frequency % 1000;
    QString row_freq_str = "";

    std::stringstream sstream;
    if (num_base_10 > 0 && num_base_10 < 1000) {
        row_freq /= 1000;
        sstream << std::setprecision(GK_FREQ_TABLEVIEW_MODEL_NUM_PRECISION) << row_freq;
        row_freq_str = tr("%1 KHz").arg(QString::fromStdString(sstream.str()));
    } else {
        row_freq /= 1000;
        row_freq /= 1000;
        sstream << std::setprecision(GK_FREQ_TABLEVIEW_MODEL_NUM_PRECISION) << row_freq;
        row_freq_str = tr("%1 MHz").arg(QString::fromStdString(sstream.str()));
    }

    QString row_digital_mode = GkDb->convDigitalModesToStr(m_data[index.row()].digital_mode);
    QString row_iaru_region = GkDb->convIARURegionToStr(m_data[index.row()].iaru_region);

    switch (index.column()) {
    case 0:
        return row_freq_str;
    case 1:
        return row_digital_mode;
    case 2:
        return row_iaru_region;
    }

    return QVariant();
}

/**
 * @brief GkFreqTableViewModel::headerData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param section
 * @param orientation
 * @param role
 * @return
 * @note When updating this function, BE SURE TO UPDATE `GkFreqTableViewModel::columnCount()` with the new total count as well!
 * @see GkFreqTableViewModel::columnCount()
 */
QVariant GkFreqTableViewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case GK_FREQ_TABLEVIEW_MODEL_FREQUENCY_IDX:
            return tr("Frequency");
        case GK_FREQ_TABLEVIEW_MODEL_MODE_IDX:
            return tr("Mode");
        case GK_FREQ_TABLEVIEW_MODEL_IARU_REGION_IDX:
            return tr("IARU Region");
        }
    }

    return QVariant();
}

void GkFreqTableViewModel::customHeaderMenuRequested(QPoint pos)
{
    menu->popup(table->horizontalHeader()->viewport()->mapToGlobal(pos));

    return;
}
