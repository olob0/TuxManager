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

#include "detailpage.h"
#include "graphcard.h"
#include "statspanel.h"
#include "../colorscheme.h"
#include "../ui/uimetrics.h"
#include "../ui/widgetstyle.h"

#include <QBoxLayout>
#include <QLabel>
#include <QResizeEvent>
#include <QScrollBar>

using namespace Perf;

DetailPage::DetailPage(QWidget *parent) : QWidget(parent)
{}

void DetailPage::setupPage(const Parts &parts)
{
    this->m_parts = parts;

    if (QLayout *root = this->layout())
    {
        root->setContentsMargins(UiMetrics::Space::XL, UiMetrics::Space::L, UiMetrics::Space::XL, UiMetrics::Space::L);
        root->setSpacing(UiMetrics::Space::L);
    }

    if (this->m_parts.Body)
    {
        this->m_parts.Body->setSpacing(UiMetrics::Space::L);
        // The graph column always takes the spare room, the statistics only what they need.
        this->m_parts.Body->setStretch(0, 1);
        this->m_parts.Body->setStretch(1, 0);
    }

    this->buildHeader();
}

void DetailPage::buildHeader()
{
    // Drop the .ui header layout (its widgets stay, owned by the page) and rebuild it so all
    // pages share one header: title over subtitle, accessory on the right.
    if (QLayout *old = this->m_parts.Header)
    {
        if (QLayout *root = this->layout())
            root->removeItem(old);
        delete old;
        this->m_parts.Header = nullptr;
    }

    this->m_titleBlock = new QVBoxLayout();
    this->m_titleBlock->setSpacing(UiMetrics::Space::XXS);
    for (QLabel *label : { this->m_parts.Title, this->m_parts.Subtitle })
    {
        if (!label)
            continue;
        // Long device names get clipped instead of dictating the page's minimum width.
        label->setMinimumWidth(1);
        label->setWordWrap(false);
        label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        this->m_titleBlock->addWidget(label);
    }

    this->m_header = new QBoxLayout(QBoxLayout::LeftToRight);
    this->m_header->setSpacing(UiMetrics::Space::L);
    this->m_header->addLayout(this->m_titleBlock, 1);
    if (this->m_parts.HeaderAccessory)
        this->m_header->addWidget(this->m_parts.HeaderAccessory, 0, Qt::AlignRight | Qt::AlignBottom);

    // The header heads the graph column, so a side-by-side statistics card starts level with it.
    QBoxLayout *graphColumn = nullptr;
    if (this->m_parts.Body && this->m_parts.Body->count() > 0)
        graphColumn = qobject_cast<QBoxLayout *>(this->m_parts.Body->itemAt(0)->layout());
    Q_ASSERT_X(graphColumn, "DetailPage", "bodyLayout must start with the graph column layout");
    if (!graphColumn)
        return;
    graphColumn->insertLayout(0, this->m_header);
    // Keep the header at its natural height; spare height belongs to the graphs, not between
    // the title and the subtitle.
    graphColumn->setAlignment(this->m_header, Qt::AlignTop);
    graphColumn->setSpacing(UiMetrics::Space::L);
    this->m_graphColumn = graphColumn;
}

void DetailPage::applyPageStyle(const QColor &accent)
{
    const ColorScheme *scheme = ColorScheme::GetCurrent();

    if (this->m_parts.Title)
        WidgetStyle::ApplyTextStyle(this->m_parts.Title, accent, UiMetrics::TextRole::Title);
    if (this->m_parts.Subtitle)
        WidgetStyle::ApplyTextStyle(this->m_parts.Subtitle, scheme->MutedTextColor, UiMetrics::TextRole::Body);

    for (GraphCard *card : this->findChildren<GraphCard *>())
        card->ApplyStyle();

    if (this->m_parts.Stats)
    {
        this->m_parts.Stats->SetAccentColor(accent);
        this->m_parts.Stats->ApplyStyle();
    }

    this->update();
}

void DetailPage::UpdateLayoutForWidth(int width)
{
    QBoxLayout *body = this->m_parts.Body;
    StatsPanel *stats = this->m_parts.Stats;
    if (!body || !stats || width <= 0 || width == this->m_layoutWidth)
        return;
    this->m_layoutWidth = width;

    const int margin = UiMetrics::PageMargin(width);
    if (QLayout *root = this->layout())
        root->setContentsMargins(margin, UiMetrics::Space::L, margin, UiMetrics::Space::L);

    const int contentWidth = width - 2 * margin;
    int graphColumnWidth = contentWidth;
    const bool wide = width >= UiMetrics::WideLayoutMinWidth;
    this->m_wide = wide;
    if (wide)
    {
        const int statsWidth = qBound(UiMetrics::StatsPanelMinWidth, width * 28 / 100, UiMetrics::StatsPanelMaxWidth);
        if (this->m_graphColumn && this->m_graphColumn->indexOf(stats) >= 0)
        {
            this->m_graphColumn->removeWidget(stats);
            body->addWidget(stats);
        }
        // Beside the graphs the card runs the full height of the graph column, so the column
        // ends level with the graphs instead of stopping halfway; its content stays at the top.
        body->setAlignment(stats, Qt::Alignment());
        stats->setFixedWidth(statsWidth);
        stats->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        graphColumnWidth -= statsWidth + UiMetrics::Space::L;
    } else
    {
        // Stacked, the statistics come right under the header, above the graphs.
        if (this->m_graphColumn && body->indexOf(stats) >= 0)
        {
            body->removeWidget(stats);
            this->m_graphColumn->insertWidget(1, stats);
        }
        stats->setMinimumWidth(0);
        stats->setMaximumWidth(QWIDGETSIZE_MAX);
        stats->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    }
    stats->SetCompact(!wide, contentWidth >= UiMetrics::CompactStatsFourColumnsMinWidth ? 4 : 2);

    // The accessory moves under the subtitle once it would squeeze the title.
    if (QWidget *accessory = this->m_parts.HeaderAccessory)
    {
        int titleWidth = 0;
        for (QLabel *label : { this->m_parts.Title, this->m_parts.Subtitle })
        {
            if (label)
                titleWidth = qMax(titleWidth, label->sizeHint().width());
        }
        const bool inline_ = graphColumnWidth >= titleWidth + UiMetrics::Space::L + accessory->sizeHint().width();
        this->m_header->setDirection(inline_ ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom);
        this->m_header->setSpacing(inline_ ? UiMetrics::Space::L : UiMetrics::Space::S);
        this->m_header->setStretch(0, inline_ ? 1 : 0);
        this->m_header->setAlignment(accessory, inline_ ? (Qt::AlignRight | Qt::AlignBottom) : (Qt::AlignLeft | Qt::AlignTop));
    }

    this->layoutWidthChanged(qMax(0, graphColumnWidth));
}

void DetailPage::layoutWidthChanged(int /*graphColumnWidth*/)
{}

DetailScrollArea::DetailScrollArea(DetailPage *page, QWidget *parent) : QScrollArea(parent), m_page(page)
{
    this->setWidget(page);
    this->setWidgetResizable(true);
    this->setFrameShape(QFrame::NoFrame);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // Content changes (longer values, relayouts) can change how narrow the page may get.
    page->installEventFilter(this);
}

QSize DetailScrollArea::minimumSizeHint() const
{
    // Never narrower than the page's stacked layout needs, so the splitter shrinks the sidebar
    // instead of clipping the page. While the page is in its wide layout its minimum describes
    // that layout, so the last known stacked minimum is used instead.
    if (!this->m_page->IsWideLayout())
        this->m_narrowMinWidth = this->m_page->minimumSizeHint().width();
    const int chrome = 2 * this->frameWidth() + this->verticalScrollBar()->sizeHint().width();
    const int width = qMax(UiMetrics::DetailPageMinWidth, this->m_narrowMinWidth + chrome);
    return QSize(width, QScrollArea::minimumSizeHint().height());
}

bool DetailScrollArea::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == this->m_page && event->type() == QEvent::LayoutRequest)
        this->updateGeometry();
    return QScrollArea::eventFilter(watched, event);
}

void DetailScrollArea::resizeEvent(QResizeEvent *event)
{
    // Re-layout the page for the width it is about to get before QScrollArea sizes it,
    // otherwise the page would be sized from its previous (stale) minimum size.
    int width = event->size().width() - 2 * this->frameWidth();
    if (this->verticalScrollBar()->isVisible())
        width -= this->verticalScrollBar()->width();
    this->m_page->UpdateLayoutForWidth(width);
    QScrollArea::resizeEvent(event);
}
