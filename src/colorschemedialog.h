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

#ifndef COLORSCHEMEDIALOG_H
#define COLORSCHEMEDIALOG_H

#include "colorscheme.h"

#include <QDialog>
#include <QHash>
#include <QVector>

class QHBoxLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QTreeWidget;
class QTreeWidgetItem;
class SegmentedControl;

QT_BEGIN_NAMESPACE
namespace Ui { class ColorSchemeDialog; }
QT_END_NAMESPACE

/// Color customization. The simple page edits a palette of base colors and assigns one to each
/// resource category; every tone is derived from it. The advanced page lists every single color
/// and lets the user pin any of them to a custom value.
class ColorSchemeDialog : public QDialog
{
    Q_OBJECT

    public:
        explicit ColorSchemeDialog(QWidget *parent = nullptr);
        ~ColorSchemeDialog();

        ColorScheme::Settings GetSettings() const { return this->m_settings; }

    private:
        struct CategoryRow
        {
            ColorScheme::Category Category;
            QWidget     *Preview { nullptr };
            QLabel      *Name { nullptr };
            QHBoxLayout *Swatches { nullptr };
        };

        struct FieldItem
        {
            const ColorScheme::ColorField *Field { nullptr };
            QTreeWidgetItem *Item { nullptr };
        };

        QWidget *buildSimplePage();
        QWidget *buildAdvancedPage();
        void setMode(int mode);
        void refresh();
        void refreshPalette();
        void refreshCategories();
        void refreshAdvanced();
        void showPaletteMenu(int index, QWidget *anchor);
        void changePaletteColor(int index);
        void removePaletteColor(int index);
        void addPaletteColor();
        void editField(const ColorScheme::ColorField *field);
        void applyFilter(const QString &text);

        Ui::ColorSchemeDialog *ui { nullptr };
        ColorScheme::Settings  m_settings;
        bool                   m_dark { false };

        SegmentedControl *m_mode { nullptr };
        QStackedWidget   *m_pages { nullptr };
        QPushButton      *m_resetButton { nullptr };
        QHBoxLayout      *m_paletteLayout { nullptr };
        QPushButton      *m_addButton { nullptr };
        QVector<CategoryRow> m_rows;
        QLineEdit        *m_filter { nullptr };
        QTreeWidget      *m_tree { nullptr };
        QHash<int, QTreeWidgetItem *> m_groupItems;
        QVector<FieldItem> m_fieldItems;
};

#endif // COLORSCHEMEDIALOG_H
