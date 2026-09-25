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

#include "statspanel.h"
#include "../colorscheme.h"
#include "../ui/uimetrics.h"
#include "../ui/widgetstyle.h"

#include <QBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>

using namespace Perf;

namespace
{
    constexpr int kMaxColumns = 4;
    constexpr int kWideStatColumns = 2;
    constexpr int kMeterHeight = 6;
}

//! Thin rounded usage bar under a meter entry.
class StatsPanel::MeterBar : public QWidget
{
    public:
        explicit MeterBar(QWidget *parent) : QWidget(parent)
        {
            this->setFixedHeight(kMeterHeight);
            this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        }

        void SetFraction(double fraction)
        {
            fraction = qBound(0.0, fraction, 1.0);
            if (qFuzzyCompare(fraction + 1.0, this->m_fraction + 1.0))
                return;
            this->m_fraction = fraction;
            this->update();
        }

        void SetColor(const QColor &color)
        {
            this->m_color = color;
            this->update();
        }

    protected:
        void paintEvent(QPaintEvent * /*event*/) override
        {
            QPainter p(this);
            p.setRenderHint(QPainter::Antialiasing, true);
            const QRectF track = QRectF(this->rect());
            const qreal radius = track.height() / 2.0;
            QPainterPath trackPath;
            trackPath.addRoundedRect(track, radius, radius);
            p.fillPath(trackPath, UiMetrics::CardBorderColor(this->palette()));

            if (this->m_fraction <= 0.0)
                return;
            // Keep a visible dot for tiny but non-zero usage.
            const qreal width = qMax(track.height(), track.width() * this->m_fraction);
            QPainterPath fillPath;
            fillPath.addRoundedRect(QRectF(track.left(), track.top(), width, track.height()), radius, radius);
            p.fillPath(fillPath, this->m_color.isValid() ? this->m_color : this->palette().color(QPalette::Highlight));
        }

    private:
        double m_fraction { 0.0 };
        QColor m_color;
};

StatsPanel::StatsPanel(QWidget *parent) : QFrame(parent), m_root(new QVBoxLayout(this))
{
    this->setFrameShape(QFrame::NoFrame);

    this->m_root->setContentsMargins(UiMetrics::Space::L, UiMetrics::Space::L, UiMetrics::Space::L, UiMetrics::Space::L);
    this->m_root->setSpacing(UiMetrics::Space::L);

    for (Kind kind : { Kind::Stat, Kind::Meter, Kind::Detail })
    {
        Section section;
        section.Type = kind;
        section.Grid = new QGridLayout();
        section.Grid->setContentsMargins(0, 0, 0, 0);
        section.Grid->setHorizontalSpacing(UiMetrics::Space::L);
        section.Grid->setVerticalSpacing(kind == Kind::Stat  ? UiMetrics::Space::L
                                         : kind == Kind::Meter ? UiMetrics::Space::M
                                                               : UiMetrics::Space::S);
        if (kind != Kind::Stat)
        {
            section.Separator = new QFrame(this);
            section.Separator->setFixedHeight(1);
            section.Separator->hide();
            this->m_root->addWidget(section.Separator);
        }
        this->m_root->addLayout(section.Grid);
        this->m_sections.append(section);
    }
    this->m_root->addStretch(1);
}

void StatsPanel::addEntry(Kind kind, QLabel *caption, QLabel *value)
{
    if (!caption || !value)
        return;

    Entry entry;
    entry.Host = new QWidget(this);
    auto *column = new QVBoxLayout(entry.Host);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(UiMetrics::Space::XXS);
    entry.Row = new QBoxLayout(QBoxLayout::TopToBottom);
    entry.Row->setContentsMargins(0, 0, 0, 0);
    column->addLayout(entry.Row);
    entry.Caption = caption;
    entry.Value = value;

    caption->setParent(entry.Host);
    value->setParent(entry.Host);
    // No word wrap: wrapped labels make the layout ask for heightForWidth() at the narrowest
    // width, which inflates the page's minimum height. Multi-line values use explicit '\n'.
    caption->setWordWrap(false);
    value->setWordWrap(false);
    entry.Row->addWidget(caption);
    entry.Row->addWidget(value);
    caption->show();
    value->show();

    if (kind == Kind::Meter)
    {
        entry.Bar = new MeterBar(entry.Host);
        column->addSpacing(UiMetrics::Space::XS);
        column->addWidget(entry.Bar);
    }

    this->m_sections[static_cast<int>(kind)].Entries.append(entry);
    this->relayout();
    this->ApplyStyle();
}

void StatsPanel::AddStat(QLabel *caption, QLabel *value)
{
    this->addEntry(Kind::Stat, caption, value);
}

void StatsPanel::AddMeter(QLabel *caption, QLabel *value)
{
    this->addEntry(Kind::Meter, caption, value);
}

void StatsPanel::AddDetail(QLabel *caption, QLabel *value)
{
    this->addEntry(Kind::Detail, caption, value);
}

StatsPanel::Entry *StatsPanel::findEntry(QLabel *value)
{
    for (Section &section : this->m_sections)
    {
        for (Entry &entry : section.Entries)
        {
            if (entry.Value == value)
                return &entry;
        }
    }
    return nullptr;
}

void StatsPanel::SetMeterFraction(QLabel *value, double fraction)
{
    Entry *entry = this->findEntry(value);
    if (entry && entry->Bar)
        entry->Bar->SetFraction(fraction);
}

void StatsPanel::SetEntryVisible(QLabel *value, bool visible)
{
    Entry *entry = this->findEntry(value);
    if (!entry || entry->Visible == visible)
        return;
    entry->Visible = visible;
    this->relayout();
}

void StatsPanel::SetAccentColor(const QColor &color)
{
    this->m_accent = color;
    this->ApplyStyle();
}

void StatsPanel::SetCompact(bool compact, int columns)
{
    columns = qBound(1, columns, kMaxColumns);
    if (this->m_compact == compact && this->m_compactColumns == columns)
        return;
    this->m_compact = compact;
    this->m_compactColumns = columns;
    this->relayout();
}

void StatsPanel::ApplyStyle()
{
    const ColorScheme *scheme = ColorScheme::GetCurrent();
    for (const Section &section : std::as_const(this->m_sections))
    {
        for (int i = 0; i < section.Entries.size(); ++i)
        {
            const Entry &entry = section.Entries.at(i);
            switch (section.Type)
            {
                case Kind::Stat:
                    WidgetStyle::ApplyTextStyle(entry.Caption, scheme->StatLabelColor, UiMetrics::TextRole::Caption);
                    WidgetStyle::ApplyTextStyle(entry.Value, i == 0 ? this->m_accent : QColor(), UiMetrics::TextRole::Value);
                    break;
                case Kind::Meter:
                case Kind::Detail:
                    WidgetStyle::ApplyTextStyle(entry.Caption, scheme->StatLabelColor, UiMetrics::TextRole::Body);
                    WidgetStyle::ApplyTextStyle(entry.Value, QColor(), UiMetrics::TextRole::Body);
                    break;
            }
            if (entry.Bar)
                entry.Bar->SetColor(this->m_accent);
        }

        if (section.Separator)
        {
            QPalette pal = section.Separator->palette();
            pal.setColor(QPalette::Window, UiMetrics::CardBorderColor(this->palette()));
            section.Separator->setPalette(pal);
            section.Separator->setAutoFillBackground(true);
        }
    }
    this->update();
}

int StatsPanel::columnsFor(Kind kind) const
{
    if (this->m_compact)
        return (kind == Kind::Stat) ? this->m_compactColumns : qMax(1, this->m_compactColumns / 2);
    return (kind == Kind::Stat) ? kWideStatColumns : 1;
}

bool StatsPanel::anyVisible(const Section &section)
{
    for (const Entry &entry : section.Entries)
    {
        if (entry.Visible)
            return true;
    }
    return false;
}

void StatsPanel::relayout()
{
    bool previousVisible = false;
    for (Section &section : this->m_sections)
    {
        // Meters and details read as "caption .... value" rows; stats are caption-over-value tiles.
        const bool rows = (section.Type != Kind::Stat);
        for (Entry &entry : section.Entries)
        {
            entry.Row->setDirection(rows ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom);
            entry.Row->setSpacing(rows ? UiMetrics::Space::M : UiMetrics::Space::XXS);
            entry.Value->setAlignment(rows ? (Qt::AlignRight | Qt::AlignVCenter) : (Qt::AlignLeft | Qt::AlignVCenter));
            entry.Row->setStretch(0, 0);
            entry.Row->setStretch(1, rows ? 1 : 0);
        }
        fillGrid(section.Grid, section.Entries, this->columnsFor(section.Type));

        const bool visible = anyVisible(section);
        if (section.Separator)
            section.Separator->setVisible(visible && previousVisible);
        previousVisible = previousVisible || visible;
    }
}

void StatsPanel::fillGrid(QGridLayout *grid, const QVector<Entry> &entries, int columns)
{
    while (grid->count() > 0)
        delete grid->takeAt(0);

    for (int column = 0; column < kMaxColumns; ++column)
        grid->setColumnStretch(column, column < columns ? 1 : 0);

    int slot = 0;
    for (const Entry &entry : entries)
    {
        entry.Host->setVisible(entry.Visible);
        if (!entry.Visible)
            continue;
        grid->addWidget(entry.Host, slot / columns, slot % columns, Qt::AlignTop);
        ++slot;
    }
}

void StatsPanel::paintEvent(QPaintEvent * /*event*/)
{
    QPainter p(this);
    UiMetrics::PaintCard(&p, QRectF(this->rect()), this->palette());
}
