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
 **   [ 1 ] - https://code.gekkofyre.io/amateur-radio/small-world-deluxe
 **
 ****************************************************************************************************/

#include "src/models/tableview/gk_frequency_model.hpp"
#include <utility>
#include <iomanip>
#include <sstream>
#include <QMenu>
#include <QPoint>
#include <QAction>
#include <QVBoxLayout>
#include <QClipboard>
#include <QApplication>

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

GkFreqTableViewModel::GkFreqTableViewModel(QWidget *parent) : QTableView(parent)
{
    return;
}

GkFreqTableViewModel::~GkFreqTableViewModel()
{
    return;
}

/**
 * @brief GkFreqTableViewModel::keyPressEvent
 * @author Walletfox.com <https://www.walletfox.com/course/qtableviewcopypaste.php>
 * @param event
 */
void GkFreqTableViewModel::keyPressEvent(QKeyEvent *event)
{
    QModelIndexList selectedRows = selectionModel()->selectedRows();

    // At least one entire row selected
    if (!selectedRows.isEmpty()) {
        if (event->key() == Qt::Key_Insert) {
            model()->insertRows(selectedRows.at(0).row(), selectedRows.size());
        } else if (event->key() == Qt::Key_Delete) {
            model()->removeRows(selectedRows.at(0).row(), selectedRows.size());
        }
    }

    // At least one entire cell selected
    if (!selectedIndexes().isEmpty()) {
        if (event->key() == Qt::Key_Delete) {
            foreach (QModelIndex index, selectedIndexes()) {
                model()->setData(index, QString());
            }
        } else if (event->matches(QKeySequence::Copy)) {
            QString text;
            QItemSelectionRange range = selectionModel()->selection().first();
            for (auto i = range.top(); i <= range.bottom(); ++i) {
                QStringList rowContents;
                for (auto j = range.left(); j <= range.right(); ++j) {
                    rowContents << model()->index(i, j).data().toString();
                }

                text += rowContents.join("\t");
                text += "\n";
            }

            QApplication::clipboard()->setText(text);
        } else if (event->matches(QKeySequence::Paste)) {
            QString text = QApplication::clipboard()->text();
            QStringList rowContents = text.split("\n", QString::SkipEmptyParts);

            QModelIndex initIndex = selectedIndexes().at(0);
            auto initRow = initIndex.row();
            auto initCol = initIndex.column();

            for (auto i = 0; i < rowContents.size(); ++i) {
                QStringList columnContents = rowContents.at(i).split("\t");
                for (auto j = 0; j < columnContents.size(); ++j) {
                    model()->setData(model()->index(initRow + i, initCol + j), columnContents.at(j));
                }
            }
        } else {
            QTableView::keyPressEvent(event);
        }
    }

    return;
}

GkFreqTableModel::GkFreqTableModel(QPointer<GekkoFyre::GkLevelDb> database, QWidget *parent)
    : QAbstractTableModel(parent)
{
    gkDb = std::move(database);

    QPointer<QVBoxLayout> layout = new QVBoxLayout(parent);
    proxyModel = new QSortFilterProxyModel(parent);
    proxyModel->setSourceModel(this);

    view = new GkFreqTableViewModel(parent);
    view->setModel(this);
    view->verticalHeader()->setDefaultAlignment(Qt::AlignCenter);
    view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    return;
}

GkFreqTableModel::~GkFreqTableModel()
{
    return;
}

/**
 * @brief GkFreqTableModel::populateData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param frequencies
 */
void GkFreqTableModel::populateData(const QList<GkFreqs> &frequencies)
{
    dataBatchMutex.lock();

    m_data.clear();
    m_data = frequencies;

    dataBatchMutex.unlock();
    return;
}

/**
 * @brief GkFreqTableModel::populateData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param frequencies
 * @param populate_freq_db
 */
void GkFreqTableModel::populateData(const QList<GkFreqs> &frequencies, const bool &populate_freq_db)
{
    populateData(frequencies);

    if (populate_freq_db) {
        dataBatchMutex.lock();
        for (const auto &data: frequencies) {
            emit addFreq(data);
        }
    }

    dataBatchMutex.unlock();
    return;
}

/**
 * @brief GkFreqTableModel::insertData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param freq_val
 */
void GkFreqTableModel::insertData(const GkFreqs &freq_val)
{
    dataBatchMutex.lock();
    m_data.append(freq_val);
    dataBatchMutex.unlock();

    return;
}

/**
 * @brief GkFreqTableModel::insertData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param freq_val
 * @param populate_freq_db
 */
void GkFreqTableModel::insertData(const GkFreqs &freq_val, const bool &populate_freq_db)
{
    insertData(freq_val);

    if (populate_freq_db) {
        dataBatchMutex.lock();
        emit addFreq(freq_val);
        dataBatchMutex.unlock();
    }

    return;
}

/**
 * @brief GkFreqTableModel::insertRows
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param row
 * @param count
 * @return
 */
bool GkFreqTableModel::insertRows(int row, int count, const QModelIndex &)
{
    beginInsertRows(QModelIndex(), row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        m_data.insert(row, GkFreqs());
    }

    endInsertRows();
    return true;
}

/**
 * @brief GkFreqTableModel::removeRows
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param row
 * @param count
 * @return
 */
bool GkFreqTableModel::removeRows(int row, int count, const QModelIndex &)
{
    beginRemoveRows(QModelIndex(), row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        emit removeFreq(m_data[row]);
        m_data.removeAt(row);
    }

    endRemoveRows();
    return true;
}

/**
 * @brief GkFreqTableModel::rowCount
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 * @return
 */
int GkFreqTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_data.length();
}

Qt::ItemFlags GkFreqTableModel::flags(const QModelIndex &index) const
{
    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

/**
 * @brief GkFreqTableModel::columnCount
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param parent
 * @return
 * @note BE SURE TO GET THE TOTAL COUNT from the function, `GkFreqTableModel::headerData()`!
 * @see GkFreqTableModel::headerData()
 */
int GkFreqTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return GK_FREQ_TABLEVIEW_MODEL_TOTAL_IDX; // Make sure to add the total of columns from within `GkFreqTableModel::headerData()`!
}

/**
 * @brief GkFreqTableModel::data
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param index
 * @param role
 * @return
 */
QVariant GkFreqTableModel::data(const QModelIndex &index, int role) const
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

    QString row_digital_mode = gkDb->convDigitalModesToStr(m_data[index.row()].digital_mode);
    QString row_iaru_region = gkDb->convIARURegionToStr(m_data[index.row()].iaru_region);

    switch (index.column()) {
    case GK_FREQ_TABLEVIEW_MODEL_FREQUENCY_IDX:
        return row_freq_str;
    case GK_FREQ_TABLEVIEW_MODEL_MODE_IDX:
        return row_digital_mode;
    case GK_FREQ_TABLEVIEW_MODEL_IARU_REGION_IDX:
        return row_iaru_region;
    }

    return QVariant();
}

/**
 * @brief GkFreqTableModel::headerData
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param section
 * @param orientation
 * @param role
 * @return
 * @note When updating this function, BE SURE TO UPDATE `GkFreqTableModel::columnCount()` with the new total count as well!
 * @see GkFreqTableModel::columnCount()
 */
QVariant GkFreqTableModel::headerData(int section, Qt::Orientation orientation, int role) const
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
