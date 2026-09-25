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

#ifndef UI_UIHELPER_H
#define UI_UIHELPER_H

#include <QList>
#include <QString>
#include <QVariant>
#include <QHash>

class QAbstractItemView;
class QBoxLayout;
class QHeaderView;
class QLineEdit;
class QTableView;
class QModelIndex;
class QMenu;
class QAction;
class QTimer;
class QWidget;
class QLabel;

namespace UIHelper
{
    struct TableSelectionSnapshot
    {
        QList<QVariant> SelectedKeys;
        QVariant CurrentKey;
        int ScrollPos { 0 };
    };

    QString GetVisibleRowText(const QTableView *view, int row);
    QString GetVisibleRowsText(const QTableView *view, const QList<int> &rows);

    TableSelectionSnapshot CaptureTableSelection(const QTableView *view,
                                                 int keyColumn,
                                                 const std::function<QVariant(const QModelIndex &proxyKeyIndex)> &proxyKeyResolver);

    void RestoreTableSelection(QTableView *view,
                               int keyColumn,
                               int sourceRowCount,
                               const std::function<QModelIndex(int sourceRow)> &sourceIndexForRow,
                               const std::function<QModelIndex(const QModelIndex &sourceIndex)> &sourceToProxy,
                               const std::function<QVariant(const QModelIndex &sourceKeyIndex)> &sourceKeyResolver,
                               const TableSelectionSnapshot &snapshot);

    void AddGlobalContextMenuItems(QMenu *menu, QWidget *parent = nullptr);
    //! Adds context actions relevant to graphs to a context menu.
    void AddGraphContextMenuItems(QMenu *menu, QWidget *graphArea);
    //! Adds context menu relevant to graphs to a widget.
    void EnableGraphContextMenu(QWidget *widget);
    //! Adds a "Graph time" contex menu to an existing menu that gives options to change the timespan of a graph.
    void AddGraphWindowContextMenu(QMenu *menu);
    //! Adds a "Refresh interval" contex menu to an existing menu that gives options to change the refresh rate.
    void AddRefreshIntervalContextMenu(QMenu *menu, QTimer *timer = nullptr, bool timerOwnerActive = false);
    //! Adds a context menu with a Copy action that copies the label text to the clipboard.
    void EnableCopyLabelContextMenu(QLabel *label);
    //! Makes the widget ignore the mouse wheel and pass it on to its parent, so scrolling over a
    //! combo box or spin box scrolls the page instead of silently changing the value.
    void DisableWheelInput(QWidget *widget);
    //! Gives a list/table/tree the shared look: cell padding, row height from the font, no grid.
    //! Use it for every item view so all tabs read the same way.
    void ApplyItemViewStyle(QAbstractItemView *view);
    //! Shared frame of the list tabs (Processes, Services, Users): edge-to-edge list, padded
    //! toolbar, search field with a find icon and a quiet status line. Any part may be nullptr.
    void ApplyTabChrome(QWidget *tab, QBoxLayout *toolbar, QLineEdit *search, QLabel *status);
    //! Sizes columns from a width in characters (plus cell padding), so they scale with the font
    //! and are never narrower than their header title. With growOnly, columns keep their current
    //! (e.g. restored) width unless it is too narrow.
    void SizeColumnsInChars(QHeaderView *header, const QHash<int, int> &charsByColumn, bool growOnly = false);
    //! Adds a Copy graph action to an existing menu that copies the widget snapshot to the clipboard.
    QAction *AddCopyWidgetAction(QMenu *menu, QWidget *widget, const QString &text = QString());
}

#endif // UI_UIHELPER_H
