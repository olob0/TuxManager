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

#include "sidepanelitem.h"
#include "../configuration.h"
#include "../colorscheme.h"
#include "globals.h"
#include "../ui/uimetrics.h"

#include <QPainter>
#include <QPaintEvent>
#include <QVBoxLayout>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QEnterEvent>
#endif

using namespace Perf;

SidePanelItem::SidePanelItem(const QString &title, QWidget *parent) : QWidget(parent), m_title(title), m_graph(new GraphWidget(this))
{
    this->setCursor(Qt::PointingHandCursor);
    this->setMouseTracking(true);
    this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    // Layout: the graph fills the bottom of the cell; the text rows are painted
    // directly in paintEvent to avoid font layout overhead.
    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setSpacing(0);
    this->m_graph->SetSampleCapacity(TUX_MANAGER_HISTORY_SIZE);
    this->m_graph->SetGridEnabled(CFG->SidePanelGridEnabled);
    this->m_graph->SetHoverLineEnabled(false);
    this->m_graph->SetHoverTooltipEnabled(false);
    lay->addWidget(this->m_graph);
    this->setLayout(lay);
    this->updateMargins();
}

void SidePanelItem::SetGraphSource(const HistoryBuffer &history, double maxVal)
{
    this->m_graph->SetDataSource(history, maxVal);
}

void SidePanelItem::Update(const QString &value, const QString &detail, double maxVal)
{
    this->m_value = value;
    this->m_detail = detail;
    this->m_graph->SetMax(maxVal);
    this->m_graph->Tick();
    this->update();
}

void SidePanelItem::SetSelected(bool selected)
{
    if (this->m_selected == selected)
        return;
    this->m_selected = selected;
    this->update();
}

void SidePanelItem::SetGraphColor(QColor line, QColor fill)
{
    this->m_accent = line;
    this->m_graph->SetColor(line, fill);
    this->update();
}

void SidePanelItem::SetGraphGridEnabled(bool enabled)
{
    this->m_graph->SetGridEnabled(enabled);
}

// ── Geometry ──────────────────────────────────────────────────────────────────

int SidePanelItem::textBlockHeight() const
{
    const QFontMetrics strongFm(UiMetrics::Font(UiMetrics::TextRole::Strong, this->font()));
    const QFontMetrics captionFm(UiMetrics::Font(UiMetrics::TextRole::Caption, this->font()));
    return strongFm.height() + UiMetrics::Space::XXS + captionFm.height() + UiMetrics::Space::S;
}

int SidePanelItem::graphHeight() const
{
    // Scales with the font so the thumbnail keeps its proportions on HiDPI / large fonts.
    const QFontMetrics captionFm(UiMetrics::Font(UiMetrics::TextRole::Caption, this->font()));
    return qMax(56, captionFm.height() * 4);
}

void SidePanelItem::updateMargins()
{
    if (!this->layout())
        return;
    const int pad = UiMetrics::Space::S;
    this->layout()->setContentsMargins(pad, pad + this->textBlockHeight(), pad, pad);
    this->m_graph->setFixedHeight(this->graphHeight());
    this->updateGeometry();
}

QSize SidePanelItem::sizeHint() const
{
    const int pad = UiMetrics::Space::S;
    return QSize(200, pad + this->textBlockHeight() + this->graphHeight() + pad);
}

QSize SidePanelItem::minimumSizeHint() const
{
    return QSize(120, this->sizeHint().height());
}

// ── Paint ─────────────────────────────────────────────────────────────────────

void SidePanelItem::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);   // draw children (the graph widget)

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    const ColorScheme *scheme = ColorScheme::GetCurrent();
    const QRectF r = QRectF(this->rect()).adjusted(0.5, 0.5, -0.5, -0.5);

    // Background: selected items take a tint of their own resource color.
    if (this->m_selected)
    {
        const QColor accent = this->m_accent.isValid() ? this->m_accent : this->palette().color(QPalette::Highlight);
        p.setPen(QPen(accent, 1));
        p.setBrush(UiMetrics::WithAlpha(accent, 40));
        p.drawRoundedRect(r, UiMetrics::Radius::Card, UiMetrics::Radius::Card);
    } else if (this->m_hovered)
    {
        p.setPen(Qt::NoPen);
        p.setBrush(scheme->SidePanelItemHoverBackgroundColor);
        p.drawRoundedRect(r, UiMetrics::Radius::Card, UiMetrics::Radius::Card);
    }

    // Text rows: "title ...... value" then the detail line, inset a bit more than the graph.
    const QFont strongFont = UiMetrics::Font(UiMetrics::TextRole::Strong, this->font());
    const QFont captionFont = UiMetrics::Font(UiMetrics::TextRole::Caption, this->font());
    const QFontMetrics strongFm(strongFont);
    const QFontMetrics captionFm(captionFont);

    const int inset = UiMetrics::Space::S + UiMetrics::Space::XS;
    const int top = UiMetrics::Space::S;
    const int fullW = qMax(0, this->width() - 2 * inset);

    const QString valueText = strongFm.elidedText(this->m_value, Qt::ElideLeft, fullW * 6 / 10);
    const int valueW = strongFm.horizontalAdvance(valueText);
    const int titleMaxW = qMax(0, fullW - (valueW > 0 ? valueW + UiMetrics::Space::S : 0));
    const QString titleText = strongFm.elidedText(this->m_title, Qt::ElideRight, titleMaxW);

    p.setFont(strongFont);
    p.setPen(this->m_selected ? scheme->SidePanelItemSelectedTextColor : scheme->SidePanelItemTextColor);
    p.drawText(QRect(inset, top, titleMaxW, strongFm.height()), Qt::AlignLeft | Qt::AlignVCenter, titleText);

    if (!valueText.isEmpty())
    {
        p.setPen(this->m_accent.isValid() ? this->m_accent : scheme->SidePanelItemTextColor);
        p.drawText(QRect(this->width() - inset - valueW, top, valueW, strongFm.height()),
                   Qt::AlignRight | Qt::AlignVCenter, valueText);
    }

    if (!this->m_detail.isEmpty())
    {
        p.setFont(captionFont);
        p.setPen(scheme->SidePanelItemSubtitleColor);
        const int detailTop = top + strongFm.height() + UiMetrics::Space::XXS;
        p.drawText(QRect(inset, detailTop, fullW, captionFm.height()), Qt::AlignLeft | Qt::AlignVCenter,
                   captionFm.elidedText(this->m_detail, Qt::ElideRight, fullW));
    }
}

// ── Events ────────────────────────────────────────────────────────────────────

void SidePanelItem::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);
    if (event->button() == Qt::LeftButton)
        emit this->clicked();
    else if (event->button() == Qt::RightButton)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        emit this->contextMenuRequested(event->globalPosition().toPoint());
#else
        emit this->contextMenuRequested(event->globalPos());
#endif
    }
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void SidePanelItem::enterEvent(QEnterEvent *event)
#else
void SidePanelItem::enterEvent(QEvent *event)
#endif
{
    QWidget::enterEvent(event);
    this->m_hovered = true;
    this->update();
}

void SidePanelItem::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    this->m_hovered = false;
    this->update();
}

void SidePanelItem::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::FontChange || event->type() == QEvent::StyleChange)
        this->updateMargins();
}
