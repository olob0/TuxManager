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

#ifndef PERF_GRAPHCARD_H
#define PERF_GRAPHCARD_H

#include <QColor>
#include <QFrame>
#include <QList>
#include <QPointer>
#include <QString>

class QLabel;

namespace Perf
{
    class GraphWidget;

    /// Rounded card that hosts one graph (or graph-like widget) of a detail page together with
    /// its header row (title on the left, scale on the right) and its time axis.
    ///
    /// The card only paints the surface and standardizes padding and label styling; its content
    /// layout comes from the .ui file or from code. Register the labels with SetHeader() and
    /// SetTimeAxis() so they get caption styling and the time axis follows the graph window.
    class GraphCard : public QFrame
    {
        Q_OBJECT

        public:
            struct LegendEntry
            {
                QString Name;
                QColor  Color;
                //! Swatch for a filled area (e.g. kernel time) instead of a line.
                bool    Filled { false };
            };

            explicit GraphCard(QWidget *parent = nullptr);

            void SetHeader(QLabel *title, QLabel *scale = nullptr);
            void SetTimeAxis(QLabel *left, QLabel *right);
            //! Shows a swatch and name next to the title for each series drawn in the graph.
            //! An empty list hides the legend.
            void SetLegend(const QList<LegendEntry> &entries);
            //! For graphs with two independent series (send / receive, read / write, ...): draws the
            //! secondary series as its own line in a color derived from primaryColor, names both
            //! series for the hover tooltip and shows them in the legend. Call again after the
            //! color scheme changes.
            void SetTwoLineSeries(GraphWidget *graph, const QString &primary, const QString &secondary,
                                  const QColor &primaryColor);
            //! Re-applies padding, fonts and colors; call after the color scheme changes.
            void ApplyStyle();

        protected:
            void paintEvent(QPaintEvent *event) override;

        private:
            void updateLegend();

            QPointer<QLabel>        m_title;
            QPointer<QLabel>        m_legend;
            QList<LegendEntry>      m_legendEntries;
            QList<QPointer<QLabel>> m_headerLabels;
            QList<QPointer<QLabel>> m_axisLabels;
    };
} // namespace Perf

#endif // PERF_GRAPHCARD_H
