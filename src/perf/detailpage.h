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

#ifndef PERF_DETAILPAGE_H
#define PERF_DETAILPAGE_H

#include <QColor>
#include <QScrollArea>
#include <QWidget>

class QBoxLayout;
class QLabel;
class QLayout;
class QVBoxLayout;

namespace Perf
{
    class StatsPanel;

    /// Base class of every Performance detail page (CPU, memory, disk, ...).
    ///
    /// Every page's .ui follows the same skeleton:
    ///   headerLayout  - titleLabel and a secondary line describing the device
    ///   bodyLayout    - QHBoxLayout holding the graph column (first) and the StatsPanel (second)
    /// setupPage() registers those parts once. The base class then owns the header (title,
    /// subtitle and an optional accessory such as a mode switch, placed at the top of the graph
    /// column so the statistics start level with the title), spacing, typography and the
    /// responsive behaviour: statistics sit beside the graphs on wide pages and move between the
    /// header and the graphs when the page gets narrow. Pages are shown through a DetailScrollArea, which drives
    /// UpdateLayoutForWidth().
    class DetailPage : public QWidget
    {
        Q_OBJECT

        public:
            explicit DetailPage(QWidget *parent = nullptr);

            //! Chooses between the side-by-side and the stacked layout for the given width.
            //! Must run before the page is resized, so its new minimum size is already known.
            void UpdateLayoutForWidth(int width);
            bool IsWideLayout() const { return this->m_wide; }

        protected:
            struct Parts
            {
                QLabel     *Title { nullptr };
                QLabel     *Subtitle { nullptr };
                //! Layout of the .ui holding Title and Subtitle; it is replaced by the shared header.
                QLayout    *Header { nullptr };
                QBoxLayout *Body { nullptr };
                StatsPanel *Stats { nullptr };
                //! Optional control shown right of the title (or under it on narrow pages).
                QWidget    *HeaderAccessory { nullptr };
            };

            //! Call once, right after setupUi().
            void setupPage(const Parts &parts);
            //! Applies fonts, colors and spacing to the shared parts and to every GraphCard.
            //! accent is the page's resource color; call again from ApplyColorScheme().
            void applyPageStyle(const QColor &accent);

            //! Hook for pages with responsive parts of their own (e.g. the GPU engine grid).
            //! graphColumnWidth is the width the graph column will get.
            virtual void layoutWidthChanged(int graphColumnWidth);

        private:
            void buildHeader();

            Parts        m_parts;
            QBoxLayout  *m_header { nullptr };
            QBoxLayout  *m_graphColumn { nullptr };
            QVBoxLayout *m_titleBlock { nullptr };
            int          m_layoutWidth { -1 };
            bool         m_wide { true };  // the .ui lays pages out side by side until the first resize
    };

    /// Frameless, vertically scrolling host for a DetailPage. A tall page scrolls instead of
    /// dictating the minimum height of the whole window.
    class DetailScrollArea : public QScrollArea
    {
        Q_OBJECT

        public:
            explicit DetailScrollArea(DetailPage *page, QWidget *parent = nullptr);

            QSize minimumSizeHint() const override;

        protected:
            bool eventFilter(QObject *watched, QEvent *event) override;
            void resizeEvent(QResizeEvent *event) override;

        private:
            DetailPage *m_page;
            //! Minimum width of the page in its stacked layout, the narrowest it can ever get.
            mutable int m_narrowMinWidth { 0 };
    };
} // namespace Perf

#endif // PERF_DETAILPAGE_H
