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

#include "graphcard.h"
#include "graphwidget.h"
#include "../colorscheme.h"
#include "../ui/uimetrics.h"
#include "../ui/widgetstyle.h"

#include <QBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QPainter>

using namespace Perf;

namespace
{
    //! Finds the box layout (at any depth under root) that directly holds widget.
    QBoxLayout *layoutHolding(QLayout *root, QWidget *widget)
    {
        if (!root)
            return nullptr;
        if (root->indexOf(widget) >= 0)
            return qobject_cast<QBoxLayout *>(root);
        for (int i = 0; i < root->count(); ++i)
        {
            if (QBoxLayout *found = layoutHolding(root->itemAt(i)->layout(), widget))
                return found;
        }
        return nullptr;
    }
}

GraphCard::GraphCard(QWidget *parent) : QFrame(parent)
{
    this->setFrameShape(QFrame::NoFrame);
}

void GraphCard::SetHeader(QLabel *title, QLabel *scale)
{
    if (title)
    {
        // Long titles get clipped instead of forcing a minimum width on the whole page.
        title->setMinimumWidth(1);
        this->m_title = title;
        this->m_headerLabels.append(title);
    }
    if (scale)
    {
        scale->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        this->m_headerLabels.append(scale);
    }
    this->ApplyStyle();
}

void GraphCard::SetTimeAxis(QLabel *left, QLabel *right)
{
    if (left)
    {
        // PerformanceWidget rewrites every label tagged like this when the graph window changes.
        left->setProperty("perfTimeAxisLabel", true);
        this->m_axisLabels.append(left);
    }
    if (right)
    {
        right->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        this->m_axisLabels.append(right);
    }
    this->ApplyStyle();
}

void GraphCard::SetLegend(const QList<LegendEntry> &entries)
{
    this->m_legendEntries = entries;
    if (!this->m_legend && this->m_title)
    {
        QBoxLayout *header = layoutHolding(this->layout(), this->m_title);
        if (!header)
            return;
        this->m_legend = new QLabel(this);
        this->m_legend->setTextFormat(Qt::RichText);
        // Title and legend keep their natural width; the legend absorbs the spare room so the
        // scale label stays on the right.
        const int titleIndex = header->indexOf(this->m_title);
        header->insertWidget(titleIndex + 1, this->m_legend, 1);
        header->setStretch(titleIndex, 0);
        header->setSpacing(UiMetrics::Space::M);
    }
    this->updateLegend();
}

void GraphCard::SetTwoLineSeries(GraphWidget *graph, const QString &primary, const QString &secondary,
                                 const QColor &primaryColor)
{
    // The secondary line is pulled towards the text color so it stays distinct from the primary
    // one in both light and dark themes.
    const QColor secondaryColor = UiMetrics::Mix(primaryColor, this->palette().color(QPalette::WindowText), 0.45);
    graph->SetSeriesNames(primary, secondary);
    graph->SetOverlayLineColor(secondaryColor);
    this->SetLegend({ { primary, primaryColor }, { secondary, secondaryColor } });
}

void GraphCard::updateLegend()
{
    if (!this->m_legend)
        return;

    QStringList parts;
    for (const LegendEntry &entry : std::as_const(this->m_legendEntries))
    {
        // U+2501 (heavy horizontal) draws a line swatch, U+25A0 (black square) a filled one.
        parts << QString("<span style=\"color:%1\">%2</span>&nbsp;%3")
                     .arg(entry.Color.name(), entry.Filled ? "&#9632;" : "&#9473;&#9473;", entry.Name.toHtmlEscaped());
    }
    this->m_legend->setVisible(!parts.isEmpty());
    this->m_legend->setText(parts.join("&nbsp;&nbsp;&nbsp;"));
    WidgetStyle::ApplyTextStyle(this->m_legend, ColorScheme::GetCurrent()->StatLabelColor, UiMetrics::TextRole::Caption);
}

void GraphCard::ApplyStyle()
{
    if (QLayout *lay = this->layout())
    {
        lay->setContentsMargins(UiMetrics::Space::M, UiMetrics::Space::M, UiMetrics::Space::M, UiMetrics::Space::M);
        lay->setSpacing(UiMetrics::Space::S);
    }

    const ColorScheme *scheme = ColorScheme::GetCurrent();
    for (const QPointer<QLabel> &label : std::as_const(this->m_headerLabels))
    {
        if (label)
            WidgetStyle::ApplyTextStyle(label, scheme->StatLabelColor, UiMetrics::TextRole::Caption);
    }
    for (const QPointer<QLabel> &label : std::as_const(this->m_axisLabels))
    {
        if (label)
            WidgetStyle::ApplyTextStyle(label, scheme->AxisLabelColor, UiMetrics::TextRole::Caption);
    }
    this->updateLegend();
    this->update();
}

void GraphCard::paintEvent(QPaintEvent * /*event*/)
{
    QPainter p(this);
    UiMetrics::PaintCard(&p, QRectF(this->rect()), this->palette());
}
