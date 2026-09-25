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

#include "uimetrics.h"

#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>

namespace UiMetrics
{
    namespace
    {
        qreal scaleFor(TextRole role)
        {
            switch (role)
            {
                case TextRole::Caption: return 0.9;
                case TextRole::Body:    return 1.0;
                case TextRole::Strong:  return 1.0;
                case TextRole::Heading: return 1.2;
                case TextRole::Value:   return 1.45;
                case TextRole::Title:   return 1.8;
            }
            return 1.0;
        }

        bool isDark(const QPalette &palette)
        {
            return palette.color(QPalette::Window).lightness() <= 127;
        }
    }

    QFont Font(TextRole role)
    {
        return Font(role, QApplication::font());
    }

    QFont Font(TextRole role, const QFont &base)
    {
        QFont font = base;
        if (base.pointSizeF() > 0)
            font.setPointSizeF(base.pointSizeF() * scaleFor(role));
        else if (base.pixelSize() > 0)
            font.setPixelSize(qRound(base.pixelSize() * scaleFor(role)));

        switch (role)
        {
            case TextRole::Caption:
            case TextRole::Body:
                font.setWeight(QFont::Normal);
                break;
            case TextRole::Strong:
            case TextRole::Heading:
            case TextRole::Value:
                font.setWeight(QFont::DemiBold);
                break;
            case TextRole::Title:
                font.setWeight(QFont::Bold);
                break;
        }
        return font;
    }

    qreal PointSize(TextRole role)
    {
        const qreal base = QApplication::font().pointSizeF();
        return (base > 0 ? base : 10.0) * scaleFor(role);
    }

    int CssWeight(TextRole role)
    {
        switch (role)
        {
            case TextRole::Caption:
            case TextRole::Body:
                return 400;
            case TextRole::Strong:
            case TextRole::Heading:
            case TextRole::Value:
                return 600;
            case TextRole::Title:
                return 700;
        }
        return 400;
    }

    int RowHeight(const QFontMetrics &metrics)
    {
        return metrics.height() + Space::M;
    }

    QColor Mix(const QColor &a, const QColor &b, qreal amount)
    {
        const qreal t = qBound(0.0, amount, 1.0);
        return QColor::fromRgbF(static_cast<float>(a.redF() * t + b.redF() * (1.0 - t)),
                                static_cast<float>(a.greenF() * t + b.greenF() * (1.0 - t)),
                                static_cast<float>(a.blueF() * t + b.blueF() * (1.0 - t)),
                                static_cast<float>(a.alphaF() * t + b.alphaF() * (1.0 - t)));
    }

    QColor WithAlpha(QColor color, int alpha)
    {
        color.setAlpha(qBound(0, alpha, 255));
        return color;
    }

    QColor CardColor(const QPalette &palette)
    {
        // A few percent of text color over the window lifts the card in dark themes and
        // sinks it slightly in light ones, without introducing any hue of its own.
        return Mix(palette.color(QPalette::WindowText), palette.color(QPalette::Window), isDark(palette) ? 0.045 : 0.03);
    }

    QColor CardBorderColor(const QPalette &palette)
    {
        return Mix(palette.color(QPalette::WindowText), palette.color(QPalette::Window), 0.12);
    }

    QColor SecondaryTextColor(const QPalette &palette)
    {
        return Mix(palette.color(QPalette::WindowText), palette.color(QPalette::Window), 0.68);
    }

    QColor TertiaryTextColor(const QPalette &palette)
    {
        return Mix(palette.color(QPalette::WindowText), palette.color(QPalette::Window), 0.55);
    }

    void PaintCard(QPainter *painter, const QRectF &rect, const QPalette &palette)
    {
        if (!painter)
            return;

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        QPainterPath path;
        path.addRoundedRect(rect.adjusted(0.5, 0.5, -0.5, -0.5), Radius::Card, Radius::Card);
        painter->fillPath(path, CardColor(palette));
        painter->setPen(QPen(CardBorderColor(palette), 1));
        painter->drawPath(path);
        painter->restore();
    }
}
