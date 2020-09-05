/**
 **  ______  ______  ___   ___  ______  ______  ______  ______
 ** /_____/\/_____/\/___/\/__/\/_____/\/_____/\/_____/\/_____/\
 ** \:::_ \ \::::_\/\::.\ \\ \ \:::_ \ \:::_ \ \::::_\/\:::_ \ \
 **  \:\ \ \ \:\/___/\:: \/_) \ \:\ \ \ \:\ \ \ \:\/___/\:(_) ) )_
 **   \:\ \ \ \::___\/\:. __  ( (\:\ \ \ \:\ \ \ \::___\/\: __ `\ \
 **    \:\/.:| \:\____/\: \ )  \ \\:\_\ \ \:\/.:| \:\____/\ \ `\ \ \
 **     \____/_/\_____\/\__\/\__\/ \_____\/\____/_/\_____\/\_\/ \_\/
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

#include "src/ui/spectrodialog.hpp"
#include "ui_spectrodialog.h"
#include <QMessageBox>
#include <utility>

using namespace GekkoFyre;
using namespace Database;
using namespace Settings;
using namespace Audio;
using namespace AmateurRadio;
using namespace Control;
using namespace Spectrograph;

/**
 * @brief SpectroDialog::SpectroDialog
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param spectroGui
 * @param parent
 */
SpectroDialog::SpectroDialog(QPointer<GekkoFyre::GkSpectroWaterfall> spectroGui, QWidget *parent) :
    QDialog(parent), ui(new Ui::SpectroDialog)
{
    ui->setupUi(this);
    this->adjustSize(); // Resize to contents

    prefillGraphTypes(GkGraphType::GkWaterfall);
    prefillGraphTypes(GkGraphType::GkSinewave);

    prefillGraphTiming(GkGraphTiming::GkGraphTime500Millisec);
    prefillGraphTiming(GkGraphTiming::GkGraphTime1Sec);
    prefillGraphTiming(GkGraphTiming::GkGraphTime2Sec);
    prefillGraphTiming(GkGraphTiming::GkGraphTime5Sec);
    prefillGraphTiming(GkGraphTiming::GkGraphTime10Sec);

    gkSpectroGui = std::move(spectroGui);

    ui->comboBox_colour_map->insertItem(0, tr("Linear Colour Map (RGB)"));
}

/**
 * @brief SpectroDialog::~SpectroDialog
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
SpectroDialog::~SpectroDialog()
{
    delete ui;
}

/**
 * @brief SpectroDialog::on_pushButton_apply_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void SpectroDialog::on_pushButton_apply_clicked()
{
    switch (ui->comboBox_graph_to_display->currentIndex()) {
        case GRAPH_DISPLAY_WATERFALL_STD_IDX:
            emit changeGraphType(GkGraphType::GkWaterfall);
            break;
        case GRAPH_DISPLAY_2D_SINEWAVE_IDX:
            emit changeGraphType(GkGraphType::GkSinewave);
            break;
        default:
            break;
    }

    return;
}

/**
 * @brief SpectroDialog::on_pushButton_exit_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void SpectroDialog::on_pushButton_exit_clicked()
{
    this->close();
}

/**
 * @brief SpectroDialog::on_pushButton_export_graph_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void SpectroDialog::on_pushButton_export_graph_clicked()
{
    QMessageBox::information(this, tr("Information..."), tr("Apologies, but this function does not work yet."), QMessageBox::Ok);

    return;
}

/**
 * @brief SpectroDialog::on_pushButton_print_graph_clicked
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 */
void SpectroDialog::on_pushButton_print_graph_clicked()
{
    return;
}

/**
 * @brief SpectroDialog::prefillGraphTypes
 * @author Phobos A. D'thorga <phobos.gekko@gekkofyre.io>
 * @param graph_type
 */
void SpectroDialog::prefillGraphTypes(const GkGraphType &graph_type)
{
    switch (graph_type) {
    case GkGraphType::GkWaterfall:
        ui->comboBox_graph_to_display->insertItem(GRAPH_DISPLAY_WATERFALL_STD_IDX, tr("Waterfall"));
        break;
    case GkGraphType::GkSinewave:
        ui->comboBox_graph_to_display->insertItem(GRAPH_DISPLAY_2D_SINEWAVE_IDX, tr("2D Curve / Sinewave"));
        break;
    default:
        break;
    }

    return;
}

void SpectroDialog::prefillGraphTiming(const GkGraphTiming &graph_timing)
{
    switch (graph_timing) {
    case GkGraphTiming::GkGraphTime500Millisec:
        ui->comboBox_timing->insertItem(GRAPH_DISPLAY_500_MILLISECS_IDX, tr("500 milliseconds"));
        break;
    case GkGraphTiming::GkGraphTime1Sec:
        ui->comboBox_timing->insertItem(GRAPH_DISPLAY_1_SECONDS_IDX, tr("1 second"));
        break;
    case GkGraphTiming::GkGraphTime2Sec:
        ui->comboBox_timing->insertItem(GRAPH_DISPLAY_2_SECONDS_IDX, tr("2 seconds"));
        break;
    case GkGraphTiming::GkGraphTime5Sec:
        ui->comboBox_timing->insertItem(GRAPH_DISPLAY_5_SECONDS_IDX, tr("5 seconds"));
        break;
    case GkGraphTiming::GkGraphTime10Sec:
        ui->comboBox_timing->insertItem(GRAPH_DISPLAY_10_SECONDS_IDX, tr("10 seconds"));
        break;
    default:
        break;
    }

    return;
}

void SpectroDialog::on_comboBox_timing_currentIndexChanged(int index)
{
    Q_UNUSED(index);

    return;
}

void SpectroDialog::on_horizontalSlider_freq_zoom_valueChanged(int value)
{
    Q_UNUSED(value);

    return;
}
