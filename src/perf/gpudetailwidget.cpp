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

#include "gpudetailwidget.h"
#include "configuration.h"
#include "metrics.h"
#include "ui_gpudetailwidget.h"
#include "graphcard.h"
#include "../colorscheme.h"
#include "../misc.h"
#include "../ui/uihelper.h"
#include "../ui/uimetrics.h"
#include "../ui/widgetstyle.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

using namespace Perf;

namespace
{
    const HistoryBuffer kEmptyHistory;
    constexpr int kMinEngineCardWidth = 180;
    //! Utilization and memory share one row from this graph column width on.
    constexpr int kOverviewRowMinWidth = 900;
}

GpuDetailWidget::GpuDetailWidget(QWidget *parent) : DetailPage(parent), ui(new Ui::GpuDetailWidget)
{
    this->ui->setupUi(this);

    const ColorScheme *scheme = ColorScheme::GetCurrent();
    this->m_selectedEngineBySlot = CFG->GpuEngineSelectorIndices;
    while (this->m_selectedEngineBySlot.size() < 4)
        this->m_selectedEngineBySlot.append(this->m_selectedEngineBySlot.size());
    if (this->m_selectedEngineBySlot.size() > 4)
        this->m_selectedEngineBySlot.resize(4);

    this->ui->statsPanel->AddStat(this->ui->utilLabel, this->ui->utilValueLabel);
    this->ui->statsPanel->AddStat(this->ui->tempLabel, this->ui->tempValueLabel);
    this->ui->statsPanel->AddStat(this->ui->coreClockLabel, this->ui->coreClockValueLabel);
    this->ui->statsPanel->AddStat(this->ui->powerUsageLabel, this->ui->powerUsageValueLabel);
    this->ui->statsPanel->AddMeter(this->ui->dedicatedMemLabel, this->ui->dedicatedMemValueLabel);
    this->ui->statsPanel->AddMeter(this->ui->sharedMemLabel, this->ui->sharedMemValueLabel);
    this->ui->statsPanel->AddDetail(this->ui->gpuMemLabel, this->ui->gpuMemValueLabel);
    this->ui->statsPanel->AddDetail(this->ui->driverLabel, this->ui->driverValueLabel);
    this->ui->statsPanel->AddDetail(this->ui->backendLabel, this->ui->backendValueLabel);

    this->ui->utilizationCard->SetHeader(this->ui->utilGraphLabel, this->ui->utilGraphMaxLabel);
    this->ui->utilizationCard->SetTimeAxis(this->ui->utilTimeLeftLabel, this->ui->utilTimeRightLabel);
    this->ui->dedicatedCard->SetHeader(this->ui->dedicatedMemGraphLabel, this->ui->dedicatedMemGraphMaxLabel);
    this->ui->dedicatedCard->SetTimeAxis(this->ui->dedicatedTimeLeftLabel, this->ui->dedicatedTimeRightLabel);
    this->ui->sharedCard->SetHeader(this->ui->sharedMemGraphLabel, this->ui->sharedMemGraphMaxLabel);
    this->ui->sharedCard->SetTimeAxis(this->ui->sharedTimeLeftLabel, this->ui->sharedTimeRightLabel);
    this->ui->copyCard->SetHeader(this->ui->copyBwGraphLabel, this->ui->copyBwGraphMaxLabel);
    this->ui->copyCard->SetTimeAxis(this->ui->copyTimeLeftLabel, this->ui->copyTimeRightLabel);
    this->ui->overviewLayout->setSpacing(UiMetrics::Space::L);
    this->ui->memoryLayout->setSpacing(UiMetrics::Space::L);
    // Utilization is the headline graph: it gets the most room in either overview arrangement.
    this->ui->overviewLayout->setStretch(0, 2);
    this->ui->overviewLayout->setStretch(1, 1);
    this->ui->graphColumn->setStretchFactor(this->ui->overviewLayout, 3);
    this->ui->graphColumn->setStretchFactor(this->ui->copyCard, 1);

    this->setupPage({ this->ui->titleLabel, this->ui->modelLabel, this->ui->headerLayout, this->ui->bodyLayout, this->ui->statsPanel });

    auto configureGraph = [scheme](GraphWidget *graph)
    {
        graph->SetColor(scheme->GpuGraphLineColor, scheme->GpuGraphFillColor, scheme->GpuGraphSecondaryFillColor);
        graph->SetSampleCapacity(TUX_MANAGER_HISTORY_SIZE);
        graph->SetGridColumns(6);
        graph->SetGridRows(4);
        graph->SetValueFormat(GraphWidget::ValueFormat::Percent);
    };

    // Engine cards are laid out by relayoutEngineCards() so they can reflow with the page width
    this->m_engineGrid = new QGridLayout(this->ui->engineAreaContainer);
    this->m_engineGrid->setContentsMargins(0, 0, 0, 0);
    this->m_engineGrid->setSpacing(UiMetrics::Space::L);

    for (int slot = 0; slot < 4; ++slot)
    {
        auto *card = new GraphCard(this->ui->engineAreaContainer);
        auto *cell = new QVBoxLayout(card);

        auto *top = new QHBoxLayout();
        auto *selector = new QComboBox(card);
        auto *value = new QLabel("0%", card);
        value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        UIHelper::EnableCopyLabelContextMenu(value);
        // Sized to the engine name, but allowed to shrink so the grid can still reflow.
        selector->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        selector->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        selector->setMinimumWidth(selector->fontMetrics().averageCharWidth() * 8);
        UIHelper::DisableWheelInput(selector);
        top->addWidget(selector);
        top->addStretch(1);
        top->addWidget(value);

        auto *graph = new GraphWidget(card);
        configureGraph(graph);
        graph->setMinimumHeight(UiMetrics::CompactGraphMinHeight);
        graph->setMaximumHeight(UiMetrics::SecondaryGraphHeight);
        UIHelper::EnableGraphContextMenu(graph);

        this->m_engineCards.append(card);
        this->m_engineSelectors.append(selector);
        this->m_engineValueLabels.append(value);
        this->m_engineGraphs.append(graph);

        connect(selector,
                qOverload<int>(&QComboBox::currentIndexChanged),
                this,
                [this, slot](int index) { this->onEngineSelectionChanged(slot, index); });

        cell->addLayout(top);
        cell->addWidget(graph, 1);
    }
    this->relayoutEngineCards(0);

    configureGraph(this->ui->utilGraphWidget);
    this->ui->utilGraphWidget->SetSeriesNames(tr("Utilization"));
    UIHelper::EnableGraphContextMenu(this->ui->utilGraphWidget);

    configureGraph(this->ui->dedicatedMemGraphWidget);
    this->ui->dedicatedMemGraphWidget->SetSeriesNames(tr("Dedicated memory usage"));
    UIHelper::EnableGraphContextMenu(this->ui->dedicatedMemGraphWidget);

    configureGraph(this->ui->sharedMemGraphWidget);
    this->ui->sharedMemGraphWidget->SetSeriesNames(tr("Shared memory usage"));
    UIHelper::EnableGraphContextMenu(this->ui->sharedMemGraphWidget);

    configureGraph(this->ui->copyBwGraphWidget);
    this->ui->copyBwGraphWidget->SetValueFormat(GraphWidget::ValueFormat::BytesPerSec);
    UIHelper::EnableGraphContextMenu(this->ui->copyBwGraphWidget);

    UIHelper::EnableCopyLabelContextMenu(this->ui->utilValueLabel);
    UIHelper::EnableCopyLabelContextMenu(this->ui->tempValueLabel);
    UIHelper::EnableCopyLabelContextMenu(this->ui->dedicatedMemValueLabel);
    UIHelper::EnableCopyLabelContextMenu(this->ui->sharedMemValueLabel);
    UIHelper::EnableCopyLabelContextMenu(this->ui->gpuMemValueLabel);
    UIHelper::EnableCopyLabelContextMenu(this->ui->driverValueLabel);
    UIHelper::EnableCopyLabelContextMenu(this->ui->backendValueLabel);
    UIHelper::EnableCopyLabelContextMenu(this->ui->coreClockValueLabel);
    UIHelper::EnableCopyLabelContextMenu(this->ui->powerUsageValueLabel);

    this->applyStyle();
}

GpuDetailWidget::~GpuDetailWidget()
{
    delete this->ui;
}

void GpuDetailWidget::SetGpu(int index)
{
    this->m_gpuIndex = index;

    connect(Metrics::Get(), &Metrics::updated, this, &GpuDetailWidget::onUpdated);
    if (this->m_gpuIndex >= 0 && this->m_gpuIndex < Metrics::GetGPU()->GpuCount())
    {
        this->bindGpuIdentity();
        this->rebuildEngineSelectors();
        this->bindMemoryAndCopySources();
        for (int slot = 0; slot < this->m_engineGraphs.size(); ++slot)
            this->bindEngineGraphSource(slot);
    }
    this->onUpdated();
}

void GpuDetailWidget::ApplyColorScheme()
{
    const ColorScheme *scheme = ColorScheme::GetCurrent();
    this->applyStyle();

    auto applyGraph = [scheme](GraphWidget *graph)
    {
        if (graph)
            graph->SetColor(scheme->GpuGraphLineColor, scheme->GpuGraphFillColor, scheme->GpuGraphSecondaryFillColor);
    };

    for (GraphWidget *graph : this->m_engineGraphs)
        applyGraph(graph);
    applyGraph(this->ui->utilGraphWidget);
    applyGraph(this->ui->dedicatedMemGraphWidget);
    applyGraph(this->ui->sharedMemGraphWidget);
    applyGraph(this->ui->copyBwGraphWidget);
}

void GpuDetailWidget::applyStyle()
{
    const ColorScheme *scheme = ColorScheme::GetCurrent();
    this->applyPageStyle(scheme->GpuTitleColor);
    for (QLabel *value : std::as_const(this->m_engineValueLabels))
        WidgetStyle::ApplyTextStyle(value, QColor(), UiMetrics::TextRole::Heading);
    this->ui->copyCard->SetTwoLineSeries(this->ui->copyBwGraphWidget, tr("TX"), tr("RX"), scheme->GpuGraphLineColor,
                                         scheme->GpuGraphSecondaryLineColor);
}

void GpuDetailWidget::layoutWidthChanged(int graphColumnWidth)
{
    // Utilization beside the memory graphs on wide pages, otherwise utilization on top and the
    // two memory graphs side by side under it.
    const bool row = graphColumnWidth >= kOverviewRowMinWidth;
    this->ui->overviewLayout->setDirection(row ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom);
    this->ui->memoryLayout->setDirection(row ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
    this->relayoutEngineCards(graphColumnWidth);
}

void GpuDetailWidget::relayoutEngineCards(int graphColumnWidth)
{
    // Four engines side by side when there is room for readable graphs, otherwise a 2x2 grid.
    const int columns = (graphColumnWidth >= 4 * kMinEngineCardWidth) ? 4 : 2;
    if (columns == this->m_engineColumns)
        return;
    this->m_engineColumns = columns;

    while (this->m_engineGrid->count() > 0)
        delete this->m_engineGrid->takeAt(0);
    for (int column = 0; column < 4; ++column)
        this->m_engineGrid->setColumnStretch(column, column < columns ? 1 : 0);
    for (int slot = 0; slot < this->m_engineCards.size(); ++slot)
        this->m_engineGrid->addWidget(this->m_engineCards.at(slot), slot / columns, slot % columns);
}

void GpuDetailWidget::onEngineSelectionChanged(int slot, int comboIndex)
{
    if (slot < 0 || slot >= this->m_selectedEngineBySlot.size())
        return;

    if (slot < 0 || slot >= this->m_engineSelectors.size())
        return;

    QComboBox *combo = this->m_engineSelectors.at(slot);
    if (!combo || comboIndex < 0)
        return;

    this->m_selectedEngineBySlot[slot] = combo->itemData(comboIndex).toInt();
    CFG->GpuEngineSelectorIndices = this->m_selectedEngineBySlot;
    this->bindEngineGraphSource(slot);
    this->onUpdated();
}

void GpuDetailWidget::rebuildEngineSelectors()
{
    if (this->m_gpuIndex < 0 || this->m_gpuIndex >= Metrics::GetGPU()->GpuCount())
        return;

    const GPU::GPUInfo &gpu = Metrics::GetGPU()->FromIndex(this->m_gpuIndex);
    const int engineCount = gpu.Engines.size();
    for (int slot = 0; slot < this->m_engineSelectors.size(); ++slot)
    {
        QComboBox *combo = this->m_engineSelectors.at(slot);
        if (!combo)
            continue;

        combo->blockSignals(true);
        combo->clear();

        for (int i = 0; i < engineCount; ++i)
            combo->addItem(gpu.Engines.at(i)->Label, i);

        int engineIndex = this->m_selectedEngineBySlot[slot];
        if (engineIndex < 0 || engineIndex >= engineCount)
            engineIndex = (engineCount > 0) ? (slot % engineCount) : -1;

        this->m_selectedEngineBySlot[slot] = engineIndex;
        if (engineIndex >= 0)
            combo->setCurrentIndex(combo->findData(engineIndex));

        combo->setEnabled(engineCount > 0);
        combo->blockSignals(false);
    }
}

void GpuDetailWidget::onUpdated()
{
    if (this->m_gpuIndex < 0 || this->m_gpuIndex >= Metrics::GetGPU()->GpuCount())
        return;

    const GPU::GPUInfo &gpu = Metrics::GetGPU()->FromIndex(this->m_gpuIndex);
    const double util = gpu.UtilPct;
    const int tempC = gpu.TemperatureC;
    const bool hasCoreClock = gpu.CoreClockMHz >= 0;
    const bool hasPowerUsage = gpu.PowerUsageW >= 0.0;
    const qint64 dedicatedUsedMiB = gpu.MemUsedMiB;
    const qint64 dedicatedTotalMiB = gpu.MemTotalMiB;

    qint64 sharedTotalMiB = gpu.SharedMemTotalMiB;
    qint64 sharedUsedMiB  = gpu.SharedMemUsedMiB;

    const qint64 gpuUsedMiB = dedicatedUsedMiB + sharedUsedMiB;
    const qint64 gpuTotalMiB = dedicatedTotalMiB + sharedTotalMiB;

    this->ui->utilValueLabel->setText(QString::number(util, 'f', 0) + "%");
    this->ui->tempValueLabel->setText(tempC >= 0 ? tr("%1 °C").arg(tempC) : tr("—"));
    this->ui->gpuMemValueLabel->setText(tr("%1 / %2")
                                        .arg(Misc::FormatMiB(static_cast<quint64>(qMax<qint64>(0, gpuUsedMiB)), 1))
                                        .arg(Misc::FormatMiB(static_cast<quint64>(qMax<qint64>(0, gpuTotalMiB)), 1)));
    this->ui->dedicatedMemValueLabel->setText(tr("%1 / %2")
                                              .arg(Misc::FormatMiB(static_cast<quint64>(qMax<qint64>(0, dedicatedUsedMiB)), 1))
                                              .arg(Misc::FormatMiB(static_cast<quint64>(qMax<qint64>(0, dedicatedTotalMiB)), 1)));
    this->ui->sharedMemValueLabel->setText(tr("%1 / %2")
                                           .arg(Misc::FormatMiB(static_cast<quint64>(qMax<qint64>(0, sharedUsedMiB)), 1))
                                           .arg(Misc::FormatMiB(static_cast<quint64>(qMax<qint64>(0, sharedTotalMiB)), 1)));
    this->ui->driverValueLabel->setText(gpu.DriverVersion);
    this->ui->backendValueLabel->setText(gpu.Backend);
    this->ui->statsPanel->SetEntryVisible(this->ui->coreClockValueLabel, hasCoreClock);
    if (hasCoreClock)
        this->ui->coreClockValueLabel->setText(tr("%1 MHz").arg(gpu.CoreClockMHz));

    this->ui->statsPanel->SetEntryVisible(this->ui->powerUsageValueLabel, hasPowerUsage);
    if (hasPowerUsage)
        this->ui->powerUsageValueLabel->setText(tr("%1 W").arg(QString::number(gpu.PowerUsageW, 'f', 1)));

    for (int slot = 0; slot < this->m_engineGraphs.size(); ++slot)
    {
        const int engineIndex = (slot < this->m_selectedEngineBySlot.size())
                                ? this->m_selectedEngineBySlot.at(slot)
                                : -1;
        GraphWidget *graph = this->m_engineGraphs.at(slot);
        QLabel *value = this->m_engineValueLabels.at(slot);

        if (engineIndex >= 0)
            value->setText(QString::number(gpu.Engines.at(engineIndex)->Pct, 'f', 0) + "%");
        else
            value->setText("0%");

        graph->Tick();
    }

    this->ui->statsPanel->SetMeterFraction(this->ui->dedicatedMemValueLabel,
                                           dedicatedTotalMiB > 0 ? static_cast<double>(dedicatedUsedMiB) / static_cast<double>(dedicatedTotalMiB) : 0.0);
    this->ui->statsPanel->SetMeterFraction(this->ui->sharedMemValueLabel,
                                           sharedTotalMiB > 0 ? static_cast<double>(sharedUsedMiB) / static_cast<double>(sharedTotalMiB) : 0.0);

    this->ui->utilGraphWidget->Tick();

    this->ui->dedicatedMemGraphWidget->SetPercentTooltipAbsolute(static_cast<double>(dedicatedTotalMiB) / 1024.0, tr("GB"), 2);
    this->ui->dedicatedMemGraphMaxLabel->setText(Misc::FormatMiB(static_cast<quint64>(qMax<qint64>(0, dedicatedTotalMiB)), 1));
    this->ui->dedicatedMemGraphWidget->Tick();

    this->ui->sharedMemGraphWidget->SetPercentTooltipAbsolute(static_cast<double>(sharedTotalMiB) / 1024.0, tr("GB"), 2);
    this->ui->sharedMemGraphMaxLabel->setText(Misc::FormatMiB(static_cast<quint64>(qMax<qint64>(0, sharedTotalMiB)), 1));
    this->ui->sharedMemGraphWidget->Tick();

    const double maxCopyRate = gpu.MaxCopyBps;
    this->ui->copyBwGraphWidget->SetMax(maxCopyRate);
    this->ui->copyBwGraphMaxLabel->setText(Misc::FormatBytesPerSecond(maxCopyRate));
    this->ui->copyBwGraphWidget->Tick();
}

void GpuDetailWidget::bindGpuIdentity()
{
    if (this->m_gpuIndex < 0 || this->m_gpuIndex >= Metrics::GetGPU()->GpuCount())
        return;

    const GPU::GPUInfo &gpu = Metrics::GetGPU()->FromIndex(this->m_gpuIndex);
    this->ui->titleLabel->setText(tr("GPU %1").arg(this->m_gpuIndex));
    this->ui->modelLabel->setText(gpu.Name);
}

void GpuDetailWidget::bindEngineGraphSource(int slot)
{
    if (this->m_gpuIndex < 0 || this->m_gpuIndex >= Metrics::GetGPU()->GpuCount())
        return;
    if (slot < 0 || slot >= this->m_engineGraphs.size())
        return;

    GraphWidget *graph = this->m_engineGraphs.at(slot);
    if (!graph)
        return;

    const GPU::GPUInfo &gpu = Metrics::GetGPU()->FromIndex(this->m_gpuIndex);
    const int engineIndex = (slot < this->m_selectedEngineBySlot.size())
                            ? this->m_selectedEngineBySlot.at(slot)
                            : -1;
    if (engineIndex >= 0 && engineIndex < static_cast<int>(gpu.Engines.size()))
    {
        const GPU::GPUEngineInfo &engine = *gpu.Engines.at(engineIndex);
        graph->SetSeriesNames(engine.Label);
        graph->SetDataSource(engine.History, 100.0);
    } else
    {
        graph->SetSeriesNames(tr("Value"));
        graph->SetDataSource(kEmptyHistory, 100.0);
    }
}

void GpuDetailWidget::bindMemoryAndCopySources()
{
    if (this->m_gpuIndex < 0 || this->m_gpuIndex >= Metrics::GetGPU()->GpuCount())
        return;

    const GPU::GPUInfo *gpu = &Metrics::GetGPU()->FromIndex(this->m_gpuIndex);
    this->ui->utilGraphWidget->SetDataSource(gpu->UtilHistory, 100.0);
    this->ui->dedicatedMemGraphWidget->SetDataSource(gpu->MemUsageHistory, 100.0);
    this->ui->copyBwGraphWidget->SetDataSource(gpu->CopyTxHistory, 1024.0);
    this->ui->copyBwGraphWidget->SetOverlayDataSource(gpu->CopyRxHistory);
    this->ui->sharedMemGraphWidget->SetDataSource(gpu->SharedMemHistory, 100.0);
}
