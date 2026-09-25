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

#ifndef COLORSCHEME_H
#define COLORSCHEME_H

#include <QColor>
#include <QList>
#include <QString>
#include <QVector>
#include <QVariantMap>

class ColorScheme
{
    public:
        //! Resource a color belongs to. Every resource category gets one base color from the
        //! palette; General holds the neutral colors that follow the system theme.
        enum class Category
        {
            Cpu,
            Memory,
            Disk,
            Network,
            Gpu,
            Swap,
            General
        };
        //! Number of resource categories (everything before General).
        static constexpr int CategoryCount = 6;

        struct ColorField
        {
            const char *Name;
            QColor ColorScheme::*Member;
            Category Group;
            //! Name shown in the color dialog; nullptr keeps a field out of it (unused or legacy).
            const char *Label;
        };

        //! What the user customizes: a palette of base colors, which of them each category uses,
        //! and single colors pinned to a custom value. Everything else is derived.
        struct Settings
        {
            QList<QColor> Palette;
            //! Palette index per resource category, indexed by Category.
            QVector<int>  Assignments;
            //! Field name -> custom color.
            QVariantMap   Overrides;

            static Settings Defaults();
            //! Clamps assignments into the palette and drops a broken palette for the defaults.
            void Normalize();
            QColor BaseColor(Category category) const;
        };

        static ColorScheme *GetCurrent();
        static ColorScheme DefaultLight();
        static ColorScheme DefaultDark();
        //! Builds the scheme for the current (dark or light) theme from the user's settings.
        //! Without overrides it returns the automatic colors, which the dialog shows as defaults.
        static ColorScheme Resolve(const Settings &settings, bool dark, bool applyOverrides = true);
        static bool DetectDarkMode();
        static void Install(ColorScheme *scheme);
        static const QVector<ColorField> &Fields();
        static QString CategoryName(Category category);
        //! Stable key of a category for the configuration file.
        static QString CategoryKey(Category category);

        ColorScheme();
        QVariantMap ToVariantMap() const;
        void ApplyVariantMap(const QVariantMap &map);

        bool DarkMode { false };

        QColor CpuGraphLineColor;
        QColor CpuGraphFillColor;
        QColor CpuGraphSecondaryFillColor;
        QColor MemoryGraphLineColor;
        QColor MemoryGraphFillColor;
        QColor DiskGraphLineColor;
        QColor DiskGraphFillColor;
        QColor DiskTransferGraphLineColor;
        QColor DiskTransferGraphFillColor;
        QColor DiskTransferGraphSecondaryFillColor;
        QColor DiskTransferGraphSecondaryLineColor;
        QColor NetworkGraphLineColor;
        QColor NetworkGraphFillColor;
        QColor NetworkGraphSecondaryFillColor;
        QColor NetworkGraphSecondaryLineColor;
        QColor GpuGraphLineColor;
        QColor GpuGraphFillColor;
        QColor GpuGraphSecondaryFillColor;
        QColor GpuGraphSecondaryLineColor;
        QColor SwapUsageGraphLineColor;
        QColor SwapUsageGraphFillColor;
        QColor SwapActivityGraphLineColor;
        QColor SwapActivityGraphFillColor;
        QColor SwapActivityGraphSecondaryFillColor;
        QColor SwapActivityGraphSecondaryLineColor;
        QColor GraphGridColor;
        QColor GraphOverlayTextColor;
        QColor SidePanelBackgroundColor;
        QColor SidePanelItemHoverBackgroundColor;
        QColor SidePanelItemSelectedTextColor;
        QColor SidePanelItemTextColor;
        QColor SidePanelItemSubtitleColor;
        QColor CpuTitleColor;
        QColor CpuHeaderValueColor;
        QColor MemoryTitleColor;
        QColor DiskTitleColor;
        QColor DiskHeaderValueColor;
        QColor NetworkTitleColor;
        QColor GpuTitleColor;
        QColor MutedTextColor;
        QColor StatLabelColor;
        QColor AxisLabelColor;
        QColor MemoryLegendTextColor;
        QColor MemoryLegendUsedColor;
        QColor MemoryLegendCompressedColor;
        QColor MemoryLegendDirtyColor;
        QColor MemoryLegendCachedColor;
        QColor MemoryLegendFreeColor;
        QColor MemoryBarUsedColor;
        QColor MemoryBarCompressedColor;
        QColor MemoryBarDirtyColor;
        QColor MemoryBarCachedColor;
        QColor MemoryBarFreeColor;
        QColor MemoryBarBorderColor;

    private:
        //! Chrome colors (side panel, labels) follow the active Qt palette so they match the system theme.
        void applyPaletteNeutrals();
        //! Derives every color of a resource category (lines, fills, titles, ...) from its base color.
        void applyBaseColor(Category category, const QColor &base);

        static ColorScheme *current;
};

#endif // COLORSCHEME_H
