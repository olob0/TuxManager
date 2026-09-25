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

#include "segmentedcontrol.h"
#include "uimetrics.h"

#include <QHelpEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QToolTip>

namespace
{
    //! Gap between the outer frame and the segments.
    constexpr int kInset = 3;
}

SegmentedControl::SegmentedControl(QWidget *parent) : QWidget(parent)
{
    this->setFocusPolicy(Qt::TabFocus);
    this->setMouseTracking(true);
    this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

int SegmentedControl::AddSegment(const QString &text, const QString &toolTip)
{
    this->m_segments.append({ text, toolTip });
    if (this->m_current < 0)
        this->m_current = 0;
    this->updateGeometry();
    this->update();
    return static_cast<int>(this->m_segments.size()) - 1;
}

void SegmentedControl::SetCurrentIndex(int index)
{
    if (index < 0 || index >= this->m_segments.size() || index == this->m_current)
        return;
    this->m_current = index;
    this->update();
}

int SegmentedControl::segmentWidth(int index) const
{
    return this->fontMetrics().horizontalAdvance(this->m_segments.at(index).Text) + 2 * UiMetrics::Space::M;
}

int SegmentedControl::segmentHeight() const
{
    return this->fontMetrics().height() + UiMetrics::Space::S + UiMetrics::Space::XS;
}

QSize SegmentedControl::sizeHint() const
{
    int width = 2 * kInset;
    for (int i = 0; i < this->m_segments.size(); ++i)
        width += this->segmentWidth(i) + (i > 0 ? UiMetrics::Space::XXS : 0);
    return QSize(width, this->segmentHeight() + 2 * kInset);
}

QSize SegmentedControl::minimumSizeHint() const
{
    return this->sizeHint();
}

QRect SegmentedControl::segmentRect(int index) const
{
    int x = kInset;
    for (int i = 0; i < index; ++i)
        x += this->segmentWidth(i) + UiMetrics::Space::XXS;
    return QRect(x, kInset, this->segmentWidth(index), this->height() - 2 * kInset);
}

int SegmentedControl::segmentAt(const QPoint &pos) const
{
    for (int i = 0; i < this->m_segments.size(); ++i)
    {
        if (this->segmentRect(i).contains(pos))
            return i;
    }
    return -1;
}

void SegmentedControl::activate(int index)
{
    if (index < 0 || index >= this->m_segments.size() || index == this->m_current)
        return;
    this->m_current = index;
    this->update();
    emit this->activated(index);
}

bool SegmentedControl::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip)
    {
        auto *help = static_cast<QHelpEvent *>(event);
        const int index = this->segmentAt(help->pos());
        if (index >= 0 && !this->m_segments.at(index).ToolTip.isEmpty())
            QToolTip::showText(help->globalPos(), this->m_segments.at(index).ToolTip, this, this->segmentRect(index));
        else
            QToolTip::hideText();
        return true;
    }
    return QWidget::event(event);
}

void SegmentedControl::paintEvent(QPaintEvent * /*event*/)
{
    const QPalette &pal = this->palette();
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Recessed track holding the segments
    QPainterPath track;
    track.addRoundedRect(QRectF(this->rect()).adjusted(0.5, 0.5, -0.5, -0.5), UiMetrics::Radius::Card, UiMetrics::Radius::Card);
    p.fillPath(track, pal.color(QPalette::Base));
    p.setPen(QPen(UiMetrics::CardBorderColor(pal), 1));
    p.drawPath(track);

    const QColor highlight = pal.color(QPalette::Highlight);
    for (int i = 0; i < this->m_segments.size(); ++i)
    {
        const QRectF rect = QRectF(this->segmentRect(i)).adjusted(0.5, 0.5, -0.5, -0.5);
        const bool selected = (i == this->m_current);
        if (selected || i == this->m_hovered)
        {
            QPainterPath path;
            path.addRoundedRect(rect, UiMetrics::Radius::Control, UiMetrics::Radius::Control);
            if (selected)
            {
                p.fillPath(path, UiMetrics::Mix(highlight, pal.color(QPalette::Window), 0.3));
                p.setPen(QPen(UiMetrics::WithAlpha(highlight, 120), 1));
                p.drawPath(path);
            } else
            {
                p.fillPath(path, UiMetrics::WithAlpha(pal.color(QPalette::WindowText), 18));
            }
        }
        if (selected && this->hasFocus())
        {
            p.setPen(QPen(highlight, 1.5));
            p.drawRoundedRect(rect, UiMetrics::Radius::Control, UiMetrics::Radius::Control);
        }

        p.setPen(selected ? pal.color(QPalette::WindowText) : UiMetrics::SecondaryTextColor(pal));
        p.drawText(rect, Qt::AlignCenter, this->m_segments.at(i).Text);
    }
}

void SegmentedControl::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        this->activate(this->segmentAt(event->position().toPoint()));
    QWidget::mousePressEvent(event);
}

void SegmentedControl::mouseMoveEvent(QMouseEvent *event)
{
    const int hovered = this->segmentAt(event->position().toPoint());
    if (hovered != this->m_hovered)
    {
        this->m_hovered = hovered;
        this->update();
    }
    QWidget::mouseMoveEvent(event);
}

void SegmentedControl::leaveEvent(QEvent *event)
{
    this->m_hovered = -1;
    this->update();
    QWidget::leaveEvent(event);
}

void SegmentedControl::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
        case Qt::Key_Left:
            this->activate(this->m_current - 1);
            return;
        case Qt::Key_Right:
            this->activate(this->m_current + 1);
            return;
        default:
            QWidget::keyPressEvent(event);
    }
}

void SegmentedControl::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::FontChange || event->type() == QEvent::StyleChange)
        this->updateGeometry();
    QWidget::changeEvent(event);
}
