/*
 * Tux Manager - Linux system monitor
 * Copyright (C) 2026 Petr Bena <petr@bena.rocks>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "cpudetailwidget.h"
#include "ui_cpudetailwidget.h"
#include "configuration.h"
#include "metrics.h"
#include "../colorscheme.h"
#include "../ui/uihelper.h"
#include "../ui/segmentedcontrol.h"

#include <QAction>
#include <QFile>
#include <QLabel>
#include <QMenu>
#include <QVBoxLayout>

using namespace Perf;

CpuDetailWidget::CpuDetailWidget(QWidget *parent) : DetailPage(parent), ui(new Ui::CpuDetailWidget)
{
    this->ui->setupUi(this);

    this->ui->statsPanel->AddStat(this->ui->statUtilLabel, this->ui->statUtilValue);
    this->ui->statsPanel->AddStat(this->ui->statSpeedLabel, this->ui->statSpeedValue);
    this->ui->statsPanel->AddStat(this->ui->statProcessesLabel, this->ui->statProcessesValue);
    this->ui->statsPanel->AddStat(this->ui->statThreadsLabel, this->ui->statThreadsValue);
    this->ui->statsPanel->AddStat(this->ui->statTempLabel, this->ui->statTempValue);
    this->ui->statsPanel->AddStat(this->ui->statUptimeLabel, this->ui->statUptimeValue);
    this->ui->statsPanel->AddDetail(this->ui->statLogicalCpusLabel, this->ui->statLogicalCpusValue);
    this->ui->statsPanel->AddDetail(this->ui->statVmLabel, this->ui->statVmValue);

    this->ui->graphCard->SetHeader(this->ui->graphTitleLabel, this->ui->graphMaxLabel);
    this->ui->graphCard->SetTimeAxis(this->ui->timeLeftLabel, this->ui->timeRightLabel);

    // Graph mode switch in the header, mirrors the "Change graph to" context menu
    this->m_modeSwitch = new SegmentedControl(this);
    this->m_modeSwitch->AddSegment(tr("Overall"), tr("Overall utilization"));
    this->m_modeSwitch->AddSegment(tr("Per core"), tr("Logical processors"));
    connect(this->m_modeSwitch, &SegmentedControl::activated, this, [this](int index)
    {
        this->setGraphMode(index == 1 ? CpuGraphArea::GraphMode::PerCore : CpuGraphArea::GraphMode::Overall);
    });

    this->setupPage({ this->ui->titleLabel, this->ui->modelNameLabel, this->ui->headerLayout, this->ui->bodyLayout,
                      this->ui->statsPanel, this->m_modeSwitch });

    // Embed CpuGraphArea into the plain container widget from the .ui
    this->m_graphArea = new CpuGraphArea(this->ui->graphAreaContainer);
    QVBoxLayout *lay  = new QVBoxLayout(this->ui->graphAreaContainer);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(this->m_graphArea);
    this->ui->graphAreaContainer->setLayout(lay);

    connect(this->m_graphArea, &CpuGraphArea::contextMenuRequested, this, &CpuDetailWidget::onContextMenuRequested);

    this->setGraphMode(CFG->CpuGraphMode == 1
                       ? CpuGraphArea::GraphMode::PerCore
                       : CpuGraphArea::GraphMode::Overall);
    this->m_graphArea->SetShowKernelTime(CFG->CpuShowKernelTimes);
    UIHelper::EnableCopyLabelContextMenu(this->ui->statUtilValue);
    UIHelper::EnableCopyLabelContextMenu(this->ui->statSpeedValue);
    UIHelper::EnableCopyLabelContextMenu(this->ui->statProcessesValue);
    UIHelper::EnableCopyLabelContextMenu(this->ui->statThreadsValue);
    UIHelper::EnableCopyLabelContextMenu(this->ui->statUptimeValue);
    UIHelper::EnableCopyLabelContextMenu(this->ui->statLogicalCpusValue);
    UIHelper::EnableCopyLabelContextMenu(this->ui->statVmValue);
    UIHelper::EnableCopyLabelContextMenu(this->ui->statTempValue);

    this->applyStyle();
}

CpuDetailWidget::~CpuDetailWidget()
{
    delete this->ui;
}

void CpuDetailWidget::Init()
{
    // Populate one-time static labels from metadata
    this->ui->modelNameLabel->setText(Metrics::GetCPU()->CpuModelName());
    this->ui->statLogicalCpusValue->setText(QString::number(Metrics::GetCPU()->CpuLogicalCount()));
    this->m_graphArea->Init();

    connect(Metrics::Get(), &Metrics::updated, this, &CpuDetailWidget::onUpdated);
    this->onUpdated();
}

void CpuDetailWidget::ApplyColorScheme()
{
    this->applyStyle();
    this->m_graphArea->ApplyColorScheme();
}

void CpuDetailWidget::applyStyle()
{
    this->applyPageStyle(ColorScheme::GetCurrent()->CpuTitleColor);
    this->updateGraphLegend();
}

void CpuDetailWidget::updateGraphLegend()
{
    // Kernel time is part of the total, so it stays a darker fill under the CPU line; the legend
    // only appears while it is drawn.
    const ColorScheme *scheme = ColorScheme::GetCurrent();
    if (this->m_graphArea && this->m_graphArea->GetShowKernelTime())
    {
        this->ui->graphCard->SetLegend({ { tr("CPU"), scheme->CpuGraphLineColor },
                                         { tr("Kernel"), scheme->CpuGraphSecondaryFillColor, true } });
    } else
    {
        this->ui->graphCard->SetLegend({});
    }
}

void CpuDetailWidget::setGraphMode(CpuGraphArea::GraphMode mode)
{
    this->m_graphArea->SetMode(mode);
    CFG->CpuGraphMode = (mode == CpuGraphArea::GraphMode::PerCore) ? 1 : 0;
    this->m_modeSwitch->SetCurrentIndex(mode == CpuGraphArea::GraphMode::PerCore ? 1 : 0);
}

// ── Private slots ─────────────────────────────────────────────────────────────

void CpuDetailWidget::onUpdated()
{
    const double pct = Metrics::GetCPU()->CpuPercent();

    // Stats panel
    this->ui->statUtilValue->setText(QString::number(pct, 'f', 1) + "%");

    const double curMhz = Metrics::GetCPU()->CpuCurrentMhz();
    if (curMhz > 0.0)
        this->ui->statSpeedValue->setText(
                tr("%1 GHz").arg(curMhz / 1000.0, 0, 'f', 2));
    else
        this->ui->statSpeedValue->setText(tr("—"));

    const int cpuTempC = Metrics::GetCPU()->CpuTemperatureC();
    this->ui->statTempValue->setText(cpuTempC >= 0 ? tr("%1 °C").arg(cpuTempC) : tr("—"));

    this->ui->statProcessesValue->setText(QString::number(Metrics::GetKernel()->ProcessCount()));
    this->ui->statThreadsValue->setText(QString::number(Metrics::GetKernel()->ThreadCount()));

    // Uptime from /proc/uptime
    QFile f("/proc/uptime");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        const double uptimeSec = f.readAll().simplified().split(' ').value(0).toDouble();
        f.close();
        const int days    = static_cast<int>(uptimeSec / 86400);
        const int hours   = static_cast<int>(uptimeSec / 3600)  % 24;
        const int minutes = static_cast<int>(uptimeSec / 60)    % 60;
        const int seconds = static_cast<int>(uptimeSec)         % 60;
        QString upStr;
        if (days > 0)
            upStr = tr("%1d %2:%3:%4")
                    .arg(days)
                    .arg(hours,   2, 10, QChar('0'))
                    .arg(minutes, 2, 10, QChar('0'))
                    .arg(seconds, 2, 10, QChar('0'));
        else
            upStr = tr("%1:%2:%3")
                    .arg(hours,   2, 10, QChar('0'))
                    .arg(minutes, 2, 10, QChar('0'))
                    .arg(seconds, 2, 10, QChar('0'));
        this->ui->statUptimeValue->setText(upStr);
    }

    // Update the graph area
    this->m_graphArea->UpdateData();

    if (Metrics::GetCPU()->CpuIsVirtualMachine())
    {
        const QString vendor = Metrics::GetCPU()->CpuVmVendor();
        if (vendor.isEmpty())
            this->ui->statVmValue->setText(tr("Yes"));
        else
            this->ui->statVmValue->setText(tr("Yes (%1)").arg(vendor));
    } else
    {
        this->ui->statVmValue->setText(tr("No"));
    }
}

void CpuDetailWidget::onContextMenuRequested(const QPoint &globalPos)
{
    QMenu menu(this);
    menu.setTitle(tr("CPU graph options"));

    // ── Change graph to ───────────────────────────────────────────────────────
    QMenu *graphMenu = menu.addMenu(tr("Change graph to"));

    QAction *actOverall  = graphMenu->addAction(tr("Overall utilization"));
    QAction *actPerCore  = graphMenu->addAction(tr("Logical processors"));
    actOverall->setCheckable(true);
    actPerCore->setCheckable(true);

    const bool isOverall = (this->m_graphArea->GetMode() == CpuGraphArea::GraphMode::Overall);
    actOverall->setChecked( isOverall);
    actPerCore->setChecked(!isOverall);

    connect(actOverall, &QAction::triggered, this, [this]()
    {
        this->setGraphMode(CpuGraphArea::GraphMode::Overall);
    });
    connect(actPerCore, &QAction::triggered, this, [this]()
    {
        this->setGraphMode(CpuGraphArea::GraphMode::PerCore);
    });

    menu.addSeparator();

    // ── Show kernel times ─────────────────────────────────────────────────────
    QAction *actKernel = menu.addAction(tr("Show kernel times"));
    actKernel->setCheckable(true);
    actKernel->setChecked(this->m_graphArea->GetShowKernelTime());
    connect(actKernel, &QAction::triggered, this, [this](bool checked)
    {
        this->m_graphArea->SetShowKernelTime(checked);
        CFG->CpuShowKernelTimes = checked;
        this->updateGraphLegend();
    });

    menu.addSeparator();

    // ── Copy ─────────────────────────────────────────────────────────────────
    UIHelper::AddGraphContextMenuItems(&menu, this->m_graphArea);

    menu.exec(globalPos);
}
