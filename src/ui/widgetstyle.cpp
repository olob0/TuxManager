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

#include "widgetstyle.h"

#include <QWidget>

namespace WidgetStyle
{
    QString TextStyle(const QColor &color, UiMetrics::TextRole role)
    {
        QString style;
        if (color.isValid())
            style += QString("color: %1; ").arg(color.name(QColor::HexArgb));
        style += QString("font-size: %1pt; font-weight: %2;")
                     .arg(QString::number(UiMetrics::PointSize(role), 'f', 1))
                     .arg(UiMetrics::CssWeight(role));
        return style;
    }

    void ApplyTextStyle(QWidget *widget, const QColor &color, UiMetrics::TextRole role)
    {
        if (!widget)
            return;
        widget->setStyleSheet(TextStyle(color, role));
    }
}
