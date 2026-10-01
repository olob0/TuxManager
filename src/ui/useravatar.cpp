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

#include "useravatar.h"
#include "../configuration.h"
#include "uimetrics.h"

#include <QApplication>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPixmap>

#include <pwd.h>
#include <vector>

namespace
{
    QString homeDirectory(uid_t uid)
    {
        struct passwd pwd {};
        struct passwd *result = nullptr;
        std::vector<char> buffer(16384);
        if (::getpwuid_r(uid, &pwd, buffer.data(), buffer.size(), &result) != 0 || !result || !result->pw_dir)
            return QString();
        return QString::fromLocal8Bit(result->pw_dir);
    }

    //! Picture the user chose in the system settings, or a null image. Other users' home
    //! directories are usually not readable, so for them only AccountsService can answer.
    QImage loadPicture(uid_t uid, const QString &userName)
    {
        QStringList candidates;
        if (!userName.isEmpty())
            candidates << QStringLiteral("/var/lib/AccountsService/icons/") + userName;
        const QString home = homeDirectory(uid);
        if (!home.isEmpty())
            candidates << home + QStringLiteral("/.face.icon") << home + QStringLiteral("/.face");

        for (const QString &path : std::as_const(candidates))
        {
            const QFileInfo info(path);
            if (!info.isFile() || !info.isReadable())
                continue;
            QImageReader reader(path);
            reader.setAutoTransform(true);
            const QImage image = reader.read();
            if (!image.isNull())
                return image;
        }
        return QImage();
    }

    //! First character of the name (a whole surrogate pair if needed), upper case.
    QString initial(const QString &userName)
    {
        if (userName.isEmpty())
            return QStringLiteral("?");
        const int length = userName.at(0).isHighSurrogate() && userName.size() > 1 ? 2 : 1;
        return userName.left(length).toUpper();
    }

    QColor backgroundFor(const QString &userName)
    {
        const QList<QColor> &palette = CFG->Colors.Palette;
        if (palette.isEmpty())
            return QApplication::palette().color(QPalette::Highlight);
        return palette.at(static_cast<int>(qHash(userName) % static_cast<size_t>(palette.size())));
    }
} // namespace

QIcon UserAvatar::For(uid_t uid, const QString &userName, int size, qreal devicePixelRatio)
{
    const int pixels = qMax(1, qRound(size * devicePixelRatio));
    QPixmap pixmap(pixels, pixels);
    pixmap.setDevicePixelRatio(devicePixelRatio);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    const QRectF circle(0, 0, size, size);
    QPainterPath clip;
    clip.addEllipse(circle);

    const QImage picture = loadPicture(uid, userName);
    if (!picture.isNull())
    {
        // Center crop to a square, then scale once to the final pixel size
        const int edge = qMin(picture.width(), picture.height());
        const QImage square = picture.copy((picture.width() - edge) / 2, (picture.height() - edge) / 2, edge, edge)
                                     .scaled(pixels, pixels, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        painter.setClipPath(clip);
        painter.drawImage(circle, square);
    } else
    {
        const QColor background = backgroundFor(userName);
        painter.setPen(Qt::NoPen);
        painter.setBrush(background);
        painter.drawPath(clip);

        QFont font = UiMetrics::Font(UiMetrics::TextRole::Strong);
        font.setPixelSize(qMax(1, qRound(size * 0.5)));
        painter.setFont(font);
        // Light text on dark colors and the other way around, whatever the theme
        const qreal luma = 0.299 * background.redF() + 0.587 * background.greenF() + 0.114 * background.blueF();
        painter.setPen(luma > 0.6 ? QColor(0, 0, 0, 200) : QColor(255, 255, 255));
        painter.drawText(circle, Qt::AlignCenter, initial(userName));
    }
    painter.end();

    return QIcon(pixmap);
}
