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

#include "colorscheme.h"
#include "ui/uimetrics.h"

#include <QApplication>
#include <QCoreApplication>
#include <QPalette>
#include <QVariant>

ColorScheme *ColorScheme::current = nullptr;

namespace
{
    using Category = ColorScheme::Category;

    const ColorScheme::ColorField kColorFields[] =
    {
        { "CpuGraphLineColor", &ColorScheme::CpuGraphLineColor, Category::Cpu, QT_TRANSLATE_NOOP("ColorScheme", "Graph line") },
        { "CpuGraphFillColor", &ColorScheme::CpuGraphFillColor, Category::Cpu, QT_TRANSLATE_NOOP("ColorScheme", "Graph fill") },
        { "CpuGraphSecondaryFillColor", &ColorScheme::CpuGraphSecondaryFillColor, Category::Cpu, QT_TRANSLATE_NOOP("ColorScheme", "Kernel time fill") },
        { "CpuTitleColor", &ColorScheme::CpuTitleColor, Category::Cpu, QT_TRANSLATE_NOOP("ColorScheme", "Title") },
        { "CpuHeaderValueColor", &ColorScheme::CpuHeaderValueColor, Category::Cpu, nullptr },
        { "MemoryGraphLineColor", &ColorScheme::MemoryGraphLineColor, Category::Memory, QT_TRANSLATE_NOOP("ColorScheme", "Graph line") },
        { "MemoryGraphFillColor", &ColorScheme::MemoryGraphFillColor, Category::Memory, QT_TRANSLATE_NOOP("ColorScheme", "Graph fill") },
        { "MemoryTitleColor", &ColorScheme::MemoryTitleColor, Category::Memory, QT_TRANSLATE_NOOP("ColorScheme", "Title") },
        { "MemoryBarUsedColor", &ColorScheme::MemoryBarUsedColor, Category::Memory, QT_TRANSLATE_NOOP("ColorScheme", "Composition: in use") },
        { "MemoryBarCompressedColor", &ColorScheme::MemoryBarCompressedColor, Category::Memory, QT_TRANSLATE_NOOP("ColorScheme", "Composition: compressed") },
        { "MemoryBarDirtyColor", &ColorScheme::MemoryBarDirtyColor, Category::Memory, QT_TRANSLATE_NOOP("ColorScheme", "Composition: dirty") },
        { "MemoryBarCachedColor", &ColorScheme::MemoryBarCachedColor, Category::Memory, QT_TRANSLATE_NOOP("ColorScheme", "Composition: cached") },
        { "MemoryBarFreeColor", &ColorScheme::MemoryBarFreeColor, Category::Memory, QT_TRANSLATE_NOOP("ColorScheme", "Composition: free") },
        { "MemoryBarBorderColor", &ColorScheme::MemoryBarBorderColor, Category::Memory, QT_TRANSLATE_NOOP("ColorScheme", "Composition: border") },
        { "MemoryLegendUsedColor", &ColorScheme::MemoryLegendUsedColor, Category::Memory, QT_TRANSLATE_NOOP("ColorScheme", "Legend: in use") },
        { "MemoryLegendCompressedColor", &ColorScheme::MemoryLegendCompressedColor, Category::Memory, QT_TRANSLATE_NOOP("ColorScheme", "Legend: compressed") },
        { "MemoryLegendDirtyColor", &ColorScheme::MemoryLegendDirtyColor, Category::Memory, QT_TRANSLATE_NOOP("ColorScheme", "Legend: dirty") },
        { "MemoryLegendCachedColor", &ColorScheme::MemoryLegendCachedColor, Category::Memory, QT_TRANSLATE_NOOP("ColorScheme", "Legend: cached") },
        { "MemoryLegendFreeColor", &ColorScheme::MemoryLegendFreeColor, Category::Memory, QT_TRANSLATE_NOOP("ColorScheme", "Legend: free") },
        { "DiskGraphLineColor", &ColorScheme::DiskGraphLineColor, Category::Disk, QT_TRANSLATE_NOOP("ColorScheme", "Active time line") },
        { "DiskGraphFillColor", &ColorScheme::DiskGraphFillColor, Category::Disk, QT_TRANSLATE_NOOP("ColorScheme", "Active time fill") },
        { "DiskTransferGraphLineColor", &ColorScheme::DiskTransferGraphLineColor, Category::Disk, QT_TRANSLATE_NOOP("ColorScheme", "Read line") },
        { "DiskTransferGraphFillColor", &ColorScheme::DiskTransferGraphFillColor, Category::Disk, QT_TRANSLATE_NOOP("ColorScheme", "Read fill") },
        { "DiskTransferGraphSecondaryLineColor", &ColorScheme::DiskTransferGraphSecondaryLineColor, Category::Disk, QT_TRANSLATE_NOOP("ColorScheme", "Write line") },
        { "DiskTransferGraphSecondaryFillColor", &ColorScheme::DiskTransferGraphSecondaryFillColor, Category::Disk, nullptr },
        { "DiskTitleColor", &ColorScheme::DiskTitleColor, Category::Disk, QT_TRANSLATE_NOOP("ColorScheme", "Title") },
        { "DiskHeaderValueColor", &ColorScheme::DiskHeaderValueColor, Category::Disk, nullptr },
        { "NetworkGraphLineColor", &ColorScheme::NetworkGraphLineColor, Category::Network, QT_TRANSLATE_NOOP("ColorScheme", "Receive line") },
        { "NetworkGraphFillColor", &ColorScheme::NetworkGraphFillColor, Category::Network, QT_TRANSLATE_NOOP("ColorScheme", "Receive fill") },
        { "NetworkGraphSecondaryLineColor", &ColorScheme::NetworkGraphSecondaryLineColor, Category::Network, QT_TRANSLATE_NOOP("ColorScheme", "Send line") },
        { "NetworkGraphSecondaryFillColor", &ColorScheme::NetworkGraphSecondaryFillColor, Category::Network, nullptr },
        { "NetworkTitleColor", &ColorScheme::NetworkTitleColor, Category::Network, QT_TRANSLATE_NOOP("ColorScheme", "Title") },
        { "GpuGraphLineColor", &ColorScheme::GpuGraphLineColor, Category::Gpu, QT_TRANSLATE_NOOP("ColorScheme", "Graph line") },
        { "GpuGraphFillColor", &ColorScheme::GpuGraphFillColor, Category::Gpu, QT_TRANSLATE_NOOP("ColorScheme", "Graph fill") },
        { "GpuGraphSecondaryLineColor", &ColorScheme::GpuGraphSecondaryLineColor, Category::Gpu, QT_TRANSLATE_NOOP("ColorScheme", "Copy RX line") },
        { "GpuGraphSecondaryFillColor", &ColorScheme::GpuGraphSecondaryFillColor, Category::Gpu, nullptr },
        { "GpuTitleColor", &ColorScheme::GpuTitleColor, Category::Gpu, QT_TRANSLATE_NOOP("ColorScheme", "Title") },
        { "SwapUsageGraphLineColor", &ColorScheme::SwapUsageGraphLineColor, Category::Swap, QT_TRANSLATE_NOOP("ColorScheme", "Usage line and title") },
        { "SwapUsageGraphFillColor", &ColorScheme::SwapUsageGraphFillColor, Category::Swap, QT_TRANSLATE_NOOP("ColorScheme", "Usage fill") },
        { "SwapActivityGraphLineColor", &ColorScheme::SwapActivityGraphLineColor, Category::Swap, QT_TRANSLATE_NOOP("ColorScheme", "Swap in line") },
        { "SwapActivityGraphFillColor", &ColorScheme::SwapActivityGraphFillColor, Category::Swap, QT_TRANSLATE_NOOP("ColorScheme", "Swap in fill") },
        { "SwapActivityGraphSecondaryLineColor", &ColorScheme::SwapActivityGraphSecondaryLineColor, Category::Swap, QT_TRANSLATE_NOOP("ColorScheme", "Swap out line") },
        { "SwapActivityGraphSecondaryFillColor", &ColorScheme::SwapActivityGraphSecondaryFillColor, Category::Swap, nullptr },
        { "GraphGridColor", &ColorScheme::GraphGridColor, Category::General, QT_TRANSLATE_NOOP("ColorScheme", "Graph grid") },
        { "GraphOverlayTextColor", &ColorScheme::GraphOverlayTextColor, Category::General, QT_TRANSLATE_NOOP("ColorScheme", "Graph overlay text") },
        { "AxisLabelColor", &ColorScheme::AxisLabelColor, Category::General, QT_TRANSLATE_NOOP("ColorScheme", "Axis labels") },
        { "StatLabelColor", &ColorScheme::StatLabelColor, Category::General, QT_TRANSLATE_NOOP("ColorScheme", "Statistic labels") },
        { "MutedTextColor", &ColorScheme::MutedTextColor, Category::General, QT_TRANSLATE_NOOP("ColorScheme", "Secondary text") },
        { "MemoryLegendTextColor", &ColorScheme::MemoryLegendTextColor, Category::General, QT_TRANSLATE_NOOP("ColorScheme", "Legend text") },
        { "SidePanelBackgroundColor", &ColorScheme::SidePanelBackgroundColor, Category::General, QT_TRANSLATE_NOOP("ColorScheme", "Side panel background") },
        { "SidePanelItemHoverBackgroundColor", &ColorScheme::SidePanelItemHoverBackgroundColor, Category::General, QT_TRANSLATE_NOOP("ColorScheme", "Side panel hover") },
        { "SidePanelItemTextColor", &ColorScheme::SidePanelItemTextColor, Category::General, QT_TRANSLATE_NOOP("ColorScheme", "Side panel text") },
        { "SidePanelItemSelectedTextColor", &ColorScheme::SidePanelItemSelectedTextColor, Category::General, QT_TRANSLATE_NOOP("ColorScheme", "Side panel selected text") },
        { "SidePanelItemSubtitleColor", &ColorScheme::SidePanelItemSubtitleColor, Category::General, QT_TRANSLATE_NOOP("ColorScheme", "Side panel subtitle") }
    };

    const QColor kBlack(0, 0, 0);
    const QColor kWhite(255, 255, 255);
    const QColor kGray(128, 128, 128);

    //! Moves color towards target by amount (0 keeps color, 1 returns target).
    QColor toward(const QColor &target, const QColor &color, qreal amount)
    {
        return UiMetrics::Mix(target, color, amount);
    }

    //! Same saturation family, hue turned by degrees; used for colors that must stand apart from the base.
    QColor turnHue(const QColor &color, int degrees, qreal saturation, qreal value)
    {
        const int hue = qMax(0, color.hsvHue());
        return QColor::fromHsvF(static_cast<float>(((hue + degrees) % 360) / 360.0),
                                static_cast<float>(saturation), static_cast<float>(value));
    }

    QColor colorFromVariant(const QVariant &value, const QColor &fallback)
    {
        if (!value.isValid())
            return fallback;
        if (value.canConvert<QColor>())
        {
            const QColor color = value.value<QColor>();
            if (color.isValid())
                return color;
        }

        const QString text = value.toString().trimmed();
        if (text.isEmpty())
            return fallback;

        const QColor color(text);
        return color.isValid() ? color : fallback;
    }
}

ColorScheme::Settings ColorScheme::Settings::Defaults()
{
    Settings settings;
    settings.Palette = {
        QColor(0x00, 0xbc, 0xff),   // cyan
        QColor(0x44, 0xa8, 0xff),   // blue
        QColor(0xcc, 0x44, 0xcc),   // purple
        QColor(0xe0, 0x5a, 0xa0),   // pink
        QColor(0xe0, 0x52, 0x4f),   // red
        QColor(0xdb, 0x8b, 0x3a),   // orange
        QColor(0xcc, 0x88, 0x44),   // amber
        QColor(0xd4, 0xb5, 0x37),   // yellow
        QColor(0x66, 0xbb, 0x44),   // green
        QColor(0x2b, 0xb5, 0xa0)    // teal
    };
    // Indexed by Category: CPU cyan, memory purple, disk green, network orange, GPU blue, swap amber.
    settings.Assignments = { 0, 2, 8, 5, 1, 6 };
    return settings;
}

void ColorScheme::Settings::Normalize()
{
    const Settings defaults = Settings::Defaults();
    if (this->Palette.isEmpty())
        this->Palette = defaults.Palette;
    while (this->Assignments.size() < ColorScheme::CategoryCount)
        this->Assignments.append(defaults.Assignments.at(this->Assignments.size()));
    this->Assignments.resize(ColorScheme::CategoryCount);
    for (int &index : this->Assignments)
        index = qBound(0, index, static_cast<int>(this->Palette.size()) - 1);
}

QColor ColorScheme::Settings::BaseColor(Category category) const
{
    const int slot = static_cast<int>(category);
    if (slot < 0 || slot >= this->Assignments.size())
        return QColor();
    const int index = this->Assignments.at(slot);
    if (index < 0 || index >= this->Palette.size())
        return QColor();
    return this->Palette.at(index);
}

ColorScheme *ColorScheme::GetCurrent()
{
    if (!ColorScheme::current)
        ColorScheme::current = new ColorScheme(ColorScheme::DefaultLight());
    return ColorScheme::current;
}

bool ColorScheme::DetectDarkMode()
{
    return QApplication::palette().color(QPalette::Window).lightness() <= 127;
}

const QVector<ColorScheme::ColorField> &ColorScheme::Fields()
{
    static const QVector<ColorScheme::ColorField> fields = []
    {
        QVector<ColorScheme::ColorField> items;
        items.reserve(static_cast<int>(sizeof(kColorFields) / sizeof(kColorFields[0])));
        for (const ColorScheme::ColorField &field : kColorFields)
            items.append(field);
        return items;
    }();
    return fields;
}

QString ColorScheme::CategoryName(Category category)
{
    switch (category)
    {
        case Category::Cpu:     return QCoreApplication::translate("ColorScheme", "CPU");
        case Category::Memory:  return QCoreApplication::translate("ColorScheme", "Memory");
        case Category::Disk:    return QCoreApplication::translate("ColorScheme", "Disk");
        case Category::Network: return QCoreApplication::translate("ColorScheme", "Network");
        case Category::Gpu:     return QCoreApplication::translate("ColorScheme", "GPU");
        case Category::Swap:    return QCoreApplication::translate("ColorScheme", "Swap");
        case Category::General: return QCoreApplication::translate("ColorScheme", "General");
    }
    return QString();
}

QString ColorScheme::CategoryKey(Category category)
{
    switch (category)
    {
        case Category::Cpu:     return "cpu";
        case Category::Memory:  return "memory";
        case Category::Disk:    return "disk";
        case Category::Network: return "network";
        case Category::Gpu:     return "gpu";
        case Category::Swap:    return "swap";
        case Category::General: return "general";
    }
    return QString();
}

void ColorScheme::Install(ColorScheme *scheme)
{
    delete ColorScheme::current;
    ColorScheme::current = scheme;
}

ColorScheme::ColorScheme()
{}

void ColorScheme::applyPaletteNeutrals()
{
    const QPalette palette = QApplication::palette();
    this->SidePanelBackgroundColor = palette.color(QPalette::Base);
    this->SidePanelItemHoverBackgroundColor = UiMetrics::WithAlpha(palette.color(QPalette::WindowText), 18);
    this->SidePanelItemSelectedTextColor = palette.color(QPalette::WindowText);
    this->SidePanelItemTextColor = palette.color(QPalette::WindowText);
    this->SidePanelItemSubtitleColor = UiMetrics::SecondaryTextColor(palette);
    this->MutedTextColor = UiMetrics::SecondaryTextColor(palette);
    this->StatLabelColor = UiMetrics::SecondaryTextColor(palette);
    this->AxisLabelColor = UiMetrics::TertiaryTextColor(palette);
    this->MemoryLegendTextColor = UiMetrics::SecondaryTextColor(palette);
}

void ColorScheme::applyBaseColor(Category category, const QColor &base_color)
{
    if (!base_color.isValid())
        return;

    const bool dark = this->DarkMode;
    QColor base = base_color.toRgb();
    base.setAlpha(255);
    const QColor text = dark ? kWhite : kBlack;

    // Lines keep the base color on dark backgrounds and are deepened a little on light ones;
    // fills are the base pushed towards the background.
    const QColor line = dark ? base : toward(kBlack, base, 0.15);
    const QColor fill = dark ? toward(kBlack, base, 0.6) : toward(kWhite, base, 0.6);
    const QColor deepFill = dark ? UiMetrics::WithAlpha(toward(kBlack, base, 0.78), 160)
                                 : UiMetrics::WithAlpha(toward(kWhite, base, 0.35), 130);
    // Transfer style graphs (disk read, swap in) use a lighter line than the page's main graph.
    const QColor lightLine = dark ? toward(kWhite, base, 0.2) : toward(kWhite, line, 0.15);
    // The second series of a two-line graph is pulled towards the text color so it stays distinct.
    const QColor secondLine = toward(text, line, 0.45);
    const QColor secondLightLine = toward(text, lightLine, 0.45);
    const QColor headerValue = toward(text, base, 0.5);

    switch (category)
    {
        case Category::Cpu:
            this->CpuGraphLineColor = line;
            this->CpuGraphFillColor = UiMetrics::WithAlpha(fill, 120);
            this->CpuGraphSecondaryFillColor = deepFill;
            this->CpuTitleColor = line;
            this->CpuHeaderValueColor = headerValue;
            break;

        case Category::Memory:
        {
            const QColor muted = toward(kGray, base, 0.5);
            const QColor dirty = turnHue(base, 100, dark ? 1.0 : 0.7, dark ? 0.73 : 0.82);
            this->MemoryGraphLineColor = line;
            this->MemoryGraphFillColor = UiMetrics::WithAlpha(dark ? toward(kBlack, base, 0.5) : toward(kWhite, base, 0.65), 130);
            this->MemoryTitleColor = line;
            this->MemoryBarDirtyColor = dirty;
            this->MemoryLegendDirtyColor = dirty;
            if (dark)
            {
                this->MemoryBarUsedColor = base;
                this->MemoryBarCompressedColor = toward(kBlack, muted, 0.35);
                this->MemoryBarCachedColor = toward(kBlack, base, 0.6);
                this->MemoryBarFreeColor = toward(kBlack, base, 0.93);
                this->MemoryBarBorderColor = toward(kBlack, muted, 0.15);
                this->MemoryLegendUsedColor = base;
                this->MemoryLegendCompressedColor = muted;
                this->MemoryLegendCachedColor = this->MemoryBarCachedColor;
                this->MemoryLegendFreeColor = toward(kBlack, base, 0.8);
            } else
            {
                this->MemoryBarUsedColor = toward(kWhite, base, 0.35);
                this->MemoryBarCompressedColor = toward(kWhite, muted, 0.35);
                this->MemoryBarCachedColor = toward(kWhite, base, 0.7);
                this->MemoryBarFreeColor = toward(kWhite, base, 0.9);
                this->MemoryBarBorderColor = toward(kWhite, muted, 0.2);
                this->MemoryLegendUsedColor = line;
                this->MemoryLegendCompressedColor = toward(kWhite, muted, 0.1);
                this->MemoryLegendCachedColor = toward(kWhite, muted, 0.3);
                this->MemoryLegendFreeColor = toward(kGray, base, 0.85);
            }
            break;
        }

        case Category::Disk:
            this->DiskGraphLineColor = line;
            this->DiskGraphFillColor = UiMetrics::WithAlpha(fill, 120);
            this->DiskTransferGraphLineColor = lightLine;
            this->DiskTransferGraphFillColor = UiMetrics::WithAlpha(fill, dark ? 100 : 110);
            this->DiskTransferGraphSecondaryFillColor = deepFill;
            this->DiskTransferGraphSecondaryLineColor = secondLightLine;
            this->DiskTitleColor = line;
            this->DiskHeaderValueColor = headerValue;
            break;

        case Category::Network:
            this->NetworkGraphLineColor = line;
            this->NetworkGraphFillColor = UiMetrics::WithAlpha(fill, 110);
            this->NetworkGraphSecondaryFillColor = deepFill;
            this->NetworkGraphSecondaryLineColor = secondLine;
            this->NetworkTitleColor = line;
            break;

        case Category::Gpu:
            this->GpuGraphLineColor = line;
            this->GpuGraphFillColor = UiMetrics::WithAlpha(fill, 110);
            this->GpuGraphSecondaryFillColor = deepFill;
            this->GpuGraphSecondaryLineColor = secondLine;
            this->GpuTitleColor = line;
            break;

        case Category::Swap:
            this->SwapUsageGraphLineColor = line;
            this->SwapUsageGraphFillColor = UiMetrics::WithAlpha(fill, 120);
            this->SwapActivityGraphLineColor = lightLine;
            this->SwapActivityGraphFillColor = UiMetrics::WithAlpha(fill, 100);
            this->SwapActivityGraphSecondaryFillColor = deepFill;
            this->SwapActivityGraphSecondaryLineColor = secondLightLine;
            break;

        case Category::General:
            break;
    }
}

ColorScheme ColorScheme::DefaultDark()
{
    ColorScheme scheme;
    scheme.DarkMode = true;
    scheme.applyPaletteNeutrals();
    scheme.GraphGridColor = QColor(0x88, 0x88, 0x99, 70);              // grey-blue, faint
    scheme.GraphOverlayTextColor = QColor(245, 245, 245, 220);         // near white, semi-transparent

    const Settings defaults = Settings::Defaults();
    for (int i = 0; i < ColorScheme::CategoryCount; ++i)
        scheme.applyBaseColor(static_cast<Category>(i), defaults.BaseColor(static_cast<Category>(i)));
    return scheme;
}

ColorScheme ColorScheme::DefaultLight()
{
    ColorScheme scheme;
    scheme.DarkMode = false;
    scheme.applyPaletteNeutrals();
    scheme.GraphGridColor = QColor(0x80, 0x80, 0x80, 48);              // grey, faint
    scheme.GraphOverlayTextColor = QColor(35, 35, 35, 220);            // near black, semi-transparent

    const Settings defaults = Settings::Defaults();
    for (int i = 0; i < ColorScheme::CategoryCount; ++i)
        scheme.applyBaseColor(static_cast<Category>(i), defaults.BaseColor(static_cast<Category>(i)));
    return scheme;
}

ColorScheme ColorScheme::Resolve(const Settings &settings, bool dark, bool applyOverrides)
{
    Settings normalized = settings;
    normalized.Normalize();

    ColorScheme scheme = dark ? ColorScheme::DefaultDark() : ColorScheme::DefaultLight();
    for (int i = 0; i < ColorScheme::CategoryCount; ++i)
        scheme.applyBaseColor(static_cast<Category>(i), normalized.BaseColor(static_cast<Category>(i)));
    if (applyOverrides)
        scheme.ApplyVariantMap(normalized.Overrides);
    return scheme;
}

QVariantMap ColorScheme::ToVariantMap() const
{
    QVariantMap map;
    map.insert("DarkMode", this->DarkMode);
    for (const ColorScheme::ColorField &field : ColorScheme::Fields())
        map.insert(field.Name, (this->*field.Member).name(QColor::HexArgb));
    return map;
}

void ColorScheme::ApplyVariantMap(const QVariantMap &map)
{
    this->DarkMode = map.value("DarkMode", this->DarkMode).toBool();
    for (const ColorScheme::ColorField &field : ColorScheme::Fields())
        this->*field.Member = colorFromVariant(map.value(field.Name), this->*field.Member);
}
