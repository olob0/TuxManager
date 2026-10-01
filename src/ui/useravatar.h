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

#ifndef UI_USERAVATAR_H
#define UI_USERAVATAR_H

#include <QIcon>
#include <QString>

#include <sys/types.h>

namespace UserAvatar
{
    //! Round picture of an account, size px wide. Uses the picture set in the system settings
    //! (AccountsService, then ~/.face.icon and ~/.face); accounts without one get their initial
    //! on a color picked from the user's palette, stable per user name.
    QIcon For(uid_t uid, const QString &userName, int size, qreal devicePixelRatio);
}

#endif // UI_USERAVATAR_H
