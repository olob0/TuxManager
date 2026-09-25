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

#ifndef PERF_STATSPANEL_H
#define PERF_STATSPANEL_H

#include <QColor>
#include <QFrame>
#include <QVector>

class QBoxLayout;
class QGridLayout;
class QLabel;
class QVBoxLayout;

namespace Perf
{
    /// Statistics card shown on every Performance detail page.
    ///
    /// Pages keep declaring their caption/value labels in the .ui file (which keeps their
    /// translations) and hand them over in display order:
    ///  - AddStat():   headline numbers, rendered as tiles with a small caption above a large value
    ///  - AddMeter():  "caption ...... value" rows with a usage bar under them (SetMeterFraction())
    ///  - AddDetail(): secondary facts, rendered as "caption ...... value" rows
    ///
    /// The panel owns the layout. In compact mode (panel placed under the graphs) the entries
    /// are spread over more columns so they take less height.
    class StatsPanel : public QFrame
    {
        Q_OBJECT

        public:
            explicit StatsPanel(QWidget *parent = nullptr);

            void AddStat(QLabel *caption, QLabel *value);
            void AddMeter(QLabel *caption, QLabel *value);
            void AddDetail(QLabel *caption, QLabel *value);
            //! Fill of the bar that belongs to the meter showing value, 0.0 - 1.0.
            void SetMeterFraction(QLabel *value, double fraction);
            //! Shows or hides the entry that owns value, together with its caption.
            void SetEntryVisible(QLabel *value, bool visible);
            //! Color of the first headline value, normally the page's resource color.
            void SetAccentColor(const QColor &color);
            //! Compact mode lays every entry out as a tile over columns columns.
            void SetCompact(bool compact, int columns = 4);
            bool IsCompact() const { return this->m_compact; }
            //! Re-applies fonts and colors; call after the color scheme changes.
            void ApplyStyle();

        protected:
            void paintEvent(QPaintEvent *event) override;

        private:
            enum class Kind { Stat, Meter, Detail };

            class MeterBar;

            struct Entry
            {
                QWidget    *Host { nullptr };
                QBoxLayout *Row { nullptr };
                QLabel     *Caption { nullptr };
                QLabel     *Value { nullptr };
                MeterBar   *Bar { nullptr };
                bool        Visible { true };
            };

            struct Section
            {
                Kind            Type { Kind::Stat };
                QGridLayout    *Grid { nullptr };
                QFrame         *Separator { nullptr };
                QVector<Entry>  Entries;
            };

            void addEntry(Kind kind, QLabel *caption, QLabel *value);
            Entry *findEntry(QLabel *value);
            void relayout();
            int columnsFor(Kind kind) const;
            static bool anyVisible(const Section &section);
            static void fillGrid(QGridLayout *grid, const QVector<Entry> &entries, int columns);

            QVBoxLayout      *m_root;
            QVector<Section>  m_sections;
            QColor            m_accent;
            bool              m_compact { false };
            int               m_compactColumns { 4 };
    };
} // namespace Perf

#endif // PERF_STATSPANEL_H
