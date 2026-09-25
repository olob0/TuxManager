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

#ifndef UI_UIMETRICS_H
#define UI_UIMETRICS_H

#include <QColor>
#include <QFont>
#include <QFontMetrics>

class QPainter;
class QPalette;
class QRectF;

//! Single source of truth for spacing, corner radii, typography and neutral surface colors.
//! Widgets should not hard-code pixel or point sizes; pick a token from here instead so the
//! whole UI stays consistent and follows the system font and Qt theme.
namespace UiMetrics
{
    //! Spacing scale in pixels.
    namespace Space
    {
        constexpr int XXS = 2;   //!< caption <-> value
        constexpr int XS  = 4;   //!< tight gaps, text inset
        constexpr int S   = 8;   //!< list gaps, item padding
        constexpr int M   = 12;  //!< card padding, cell padding
        constexpr int L   = 16;  //!< gaps between cards
        constexpr int XL  = 24;  //!< page margins
        constexpr int XXL = 32;
    }

    //! Corner radii in pixels.
    namespace Radius
    {
        constexpr int Graph   = 4;
        constexpr int Control = 6;
        constexpr int Card    = 8;
    }

    //! Detail pages put the statistics beside the graphs when they are at least this wide,
    //! otherwise the statistics move under the graphs.
    constexpr int WideLayoutMinWidth = 820;
    constexpr int StatsPanelMinWidth = 260;
    constexpr int StatsPanelMaxWidth = 360;
    //! Stacked (narrow) statistics use four columns from this panel width on, two below it.
    constexpr int CompactStatsFourColumnsMinWidth = 560;
    //! Horizontal page margin for a page of the given width: tight on small windows, where every
    //! pixel counts, and roomier as the page grows (Space::M up to Space::XL).
    int PageMargin(int pageWidth);
    //! Narrowest a detail page gets; below it the Performance sidebar gives up room instead.
    constexpr int DetailPageMinWidth = 380;
    //! Smallest height of a secondary graph (e.g. GPU engines) before its page starts scrolling.
    constexpr int CompactGraphMinHeight = 72;
    //! Height of the secondary graph cards (GPU engines); they do not stretch with the page.
    constexpr int SecondaryGraphHeight = 120;
    //! Width range of the Performance sidebar.
    constexpr int SidePanelMinWidth = 150;
    constexpr int SidePanelMaxWidth = 360;

    //! Typographic roles. Sizes are relative to the application font, so they follow the
    //! user's font settings; Caption is the smallest text the UI ever renders.
    enum class TextRole
    {
        Caption,   //!< 0.9x - graph labels, stat captions, axis labels
        Body,      //!< 1.0x - regular text and secondary values
        Strong,    //!< 1.0x semibold - item names
        Heading,   //!< 1.2x semibold - values inside graph cards
        Value,     //!< 1.45x semibold - headline numbers
        Title      //!< 1.8x bold - page titles
    };

    QFont Font(TextRole role);
    QFont Font(TextRole role, const QFont &base);
    qreal PointSize(TextRole role);
    //! Weight on the CSS scale (400 normal, 600 semibold, 700 bold).
    int CssWeight(TextRole role);

    //! Height of a list/table row: one line of text plus vertical padding.
    int RowHeight(const QFontMetrics &metrics);

    //! Linear blend of two colors; amount 0 returns b, 1 returns a.
    QColor Mix(const QColor &a, const QColor &b, qreal amount);
    //! Returns color with its alpha replaced.
    QColor WithAlpha(QColor color, int alpha);

    //! Neutral surfaces derived from the active palette, so they work with any light or dark theme.
    QColor CardColor(const QPalette &palette);
    QColor CardBorderColor(const QPalette &palette);
    QColor SecondaryTextColor(const QPalette &palette);
    QColor TertiaryTextColor(const QPalette &palette);

    //! Paints a rounded card surface with a hairline border.
    void PaintCard(QPainter *painter, const QRectF &rect, const QPalette &palette);
}

#endif // UI_UIMETRICS_H
