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

#include "colorschemedialog.h"
#include "ui_colorschemedialog.h"
#include "configuration.h"
#include "ui/segmentedcontrol.h"
#include "ui/uihelper.h"
#include "ui/uimetrics.h"
#include "ui/widgetstyle.h"

#include <QAbstractButton>
#include <QColorDialog>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace
{
    using Category = ColorScheme::Category;

    enum Mode
    {
        ModeSimple = 0,
        ModeAdvanced = 1
    };

    //! Most colors a palette can hold; enough for every category plus spares.
    constexpr int kMaxPaletteColors = 16;
    constexpr int kPaletteSwatchSize = 34;
    constexpr int kCategorySwatchSize = 18;
    //! Advanced list columns.
    constexpr int kNameColumn = 0;
    constexpr int kValueColumn = 1;
    constexpr int kStateColumn = 2;
    constexpr int kResetColumn = 3;

    //! Round color button. Checkable swatches (category rows) dim until they are the chosen one.
    class ColorSwatch : public QAbstractButton
    {
        public:
            ColorSwatch(const QColor &color, int diameter, QWidget *parent) : QAbstractButton(parent), m_color(color), m_diameter(diameter)
            {
                this->setCursor(Qt::PointingHandCursor);
                this->setFocusPolicy(Qt::TabFocus);
                this->setToolTip(color.name());
                this->setAccessibleName(color.name());
            }

            QSize sizeHint() const override
            {
                // Room for the selection ring around the circle.
                return QSize(this->m_diameter + 6, this->m_diameter + 6);
            }

        protected:
            void paintEvent(QPaintEvent * /*event*/) override
            {
                QPainter p(this);
                p.setRenderHint(QPainter::Antialiasing, true);
                const QPointF center = QRectF(this->rect()).center();
                const qreal radius = this->m_diameter / 2.0;

                if (this->isChecked() || this->hasFocus())
                {
                    const QColor ring = this->hasFocus() ? this->palette().color(QPalette::Highlight)
                                                         : this->palette().color(QPalette::WindowText);
                    p.setPen(QPen(ring, 1.5));
                    p.setBrush(Qt::NoBrush);
                    p.drawEllipse(center, radius + 2, radius + 2);
                }

                if (this->isCheckable() && !this->isChecked())
                    p.setOpacity(this->underMouse() ? 0.85 : 0.45);
                p.setPen(Qt::NoPen);
                p.setBrush(this->m_color);
                p.drawEllipse(center, radius - (this->isChecked() ? 1 : 0), radius - (this->isChecked() ? 1 : 0));
            }

            void enterEvent(QEnterEvent *event) override
            {
                this->update();
                QAbstractButton::enterEvent(event);
            }

            void leaveEvent(QEvent *event) override
            {
                this->update();
                QAbstractButton::leaveEvent(event);
            }

        private:
            QColor m_color;
            int    m_diameter;
    };

    //! Plain rounded card surface, the same one the Performance pages use.
    class CardFrame : public QWidget
    {
        public:
            using QWidget::QWidget;

        protected:
            void paintEvent(QPaintEvent * /*event*/) override
            {
                QPainter p(this);
                UiMetrics::PaintCard(&p, QRectF(this->rect()), this->palette());
            }
    };

    //! Hairline between the rows of a card.
    class Divider : public QWidget
    {
        public:
            explicit Divider(QWidget *parent) : QWidget(parent)
            {
                this->setFixedHeight(1);
            }

        protected:
            void paintEvent(QPaintEvent * /*event*/) override
            {
                QPainter p(this);
                p.fillRect(this->rect(), UiMetrics::CardBorderColor(this->palette()));
            }
    };

    //! Tiny graph drawn with a category's colors, so a palette change is visible right away.
    class CategoryPreview : public QWidget
    {
        public:
            CategoryPreview(const QVector<qreal> &primary, const QVector<qreal> &secondary, QWidget *parent)
                : QWidget(parent), m_primary(primary), m_secondary(secondary)
            {
                this->setFixedSize(78, 42);
            }

            void SetColors(const QColor &line, const QColor &fill, const QColor &second)
            {
                this->m_line = line;
                this->m_fill = fill;
                this->m_second = second;
                this->update();
            }

        protected:
            void paintEvent(QPaintEvent * /*event*/) override
            {
                QPainter p(this);
                p.setRenderHint(QPainter::Antialiasing, true);
                const QRectF area = QRectF(this->rect()).adjusted(0.5, 0.5, -0.5, -0.5);
                p.fillRect(area, this->palette().color(QPalette::Base));

                const QPainterPath line = this->path(this->m_primary, area);
                QPainterPath fill = line;
                fill.lineTo(area.bottomRight());
                fill.lineTo(area.bottomLeft());
                fill.closeSubpath();
                p.fillPath(fill, this->m_fill);
                p.strokePath(line, QPen(this->m_line, 1.5));
                if (this->m_second.isValid() && !this->m_secondary.isEmpty())
                    p.strokePath(this->path(this->m_secondary, area), QPen(this->m_second, 1.2));

                p.setPen(QPen(this->m_line, 1));
                p.setBrush(Qt::NoBrush);
                p.drawRect(area);
            }

        private:
            QPainterPath path(const QVector<qreal> &values, const QRectF &area) const
            {
                QPainterPath result;
                if (values.size() < 2)
                    return result;
                const qreal step = area.width() / (values.size() - 1);
                for (int i = 0; i < values.size(); ++i)
                {
                    const QPointF point(area.left() + i * step, area.bottom() - 2 - values.at(i) * (area.height() - 4));
                    if (i == 0)
                        result.moveTo(point);
                    else
                        result.lineTo(point);
                }
                return result;
            }

            QVector<qreal> m_primary;
            QVector<qreal> m_secondary;
            QColor m_line;
            QColor m_fill;
            QColor m_second;
    };

    QString categoryDescription(Category category)
    {
        switch (category)
        {
            case Category::Cpu:     return QCoreApplication::translate("ColorSchemeDialog", "Utilization, kernel time and per-core graphs");
            case Category::Memory:  return QCoreApplication::translate("ColorSchemeDialog", "Usage graph and memory composition");
            case Category::Disk:    return QCoreApplication::translate("ColorSchemeDialog", "Active time, read and write");
            case Category::Network: return QCoreApplication::translate("ColorSchemeDialog", "Receive and send");
            case Category::Gpu:     return QCoreApplication::translate("ColorSchemeDialog", "Engines, memory and copy bandwidth");
            case Category::Swap:    return QCoreApplication::translate("ColorSchemeDialog", "Usage, swap in and swap out");
            case Category::General: break;
        }
        return QString();
    }

    //! Sample data for the category previews; the second series is empty for single-line graphs.
    void previewSamples(Category category, QVector<qreal> &primary, QVector<qreal> &secondary)
    {
        switch (category)
        {
            case Category::Cpu:
                primary = { 0.2, 0.3, 0.25, 0.5, 0.45, 0.7, 0.4, 0.35, 0.6, 0.5 };
                break;
            case Category::Memory:
                primary = { 0.5, 0.52, 0.55, 0.56, 0.6, 0.6, 0.62, 0.61, 0.63, 0.64 };
                break;
            case Category::Disk:
                primary = { 0.05, 0.1, 0.6, 0.2, 0.05, 0.1, 0.4, 0.8, 0.2, 0.1 };
                secondary = { 0.02, 0.3, 0.1, 0.05, 0.2, 0.5, 0.1, 0.05, 0.1, 0.3 };
                break;
            case Category::Network:
                primary = { 0.1, 0.2, 0.15, 0.7, 0.3, 0.2, 0.25, 0.5, 0.2, 0.15 };
                secondary = { 0.05, 0.1, 0.08, 0.2, 0.1, 0.12, 0.1, 0.25, 0.1, 0.08 };
                break;
            case Category::Gpu:
                primary = { 0.1, 0.1, 0.3, 0.5, 0.2, 0.1, 0.6, 0.3, 0.1, 0.2 };
                secondary = { 0.05, 0.2, 0.1, 0.1, 0.4, 0.2, 0.1, 0.1, 0.3, 0.1 };
                break;
            case Category::Swap:
                primary = { 0.3, 0.3, 0.32, 0.32, 0.33, 0.35, 0.35, 0.36, 0.36, 0.37 };
                break;
            case Category::General:
                break;
        }
    }

    struct PreviewColors
    {
        QColor Title;
        QColor Line;
        QColor Fill;
        QColor Second;
    };

    PreviewColors previewColors(const ColorScheme &scheme, Category category)
    {
        switch (category)
        {
            case Category::Cpu:     return { scheme.CpuTitleColor, scheme.CpuGraphLineColor, scheme.CpuGraphFillColor, QColor() };
            case Category::Memory:  return { scheme.MemoryTitleColor, scheme.MemoryGraphLineColor, scheme.MemoryGraphFillColor, QColor() };
            case Category::Disk:    return { scheme.DiskTitleColor, scheme.DiskTransferGraphLineColor, scheme.DiskTransferGraphFillColor, scheme.DiskTransferGraphSecondaryLineColor };
            case Category::Network: return { scheme.NetworkTitleColor, scheme.NetworkGraphLineColor, scheme.NetworkGraphFillColor, scheme.NetworkGraphSecondaryLineColor };
            case Category::Gpu:     return { scheme.GpuTitleColor, scheme.GpuGraphLineColor, scheme.GpuGraphFillColor, scheme.GpuGraphSecondaryLineColor };
            case Category::Swap:    return { scheme.SwapUsageGraphLineColor, scheme.SwapUsageGraphLineColor, scheme.SwapUsageGraphFillColor, QColor() };
            case Category::General: break;
        }
        return {};
    }

    //! Swatch icon for the advanced list; translucent colors are shown over a checkerboard.
    QIcon swatchIcon(const QColor &color, const QSize &size)
    {
        QPixmap pixmap(size);
        pixmap.fill(Qt::transparent);
        QPainter p(&pixmap);
        const QRect box = QRect(QPoint(0, 0), size).adjusted(0, 0, -1, -1);
        if (color.alpha() < 255)
        {
            const int cell = qMax(3, size.height() / 3);
            for (int y = 0; y < size.height(); y += cell)
            {
                for (int x = 0; x < size.width(); x += cell)
                    p.fillRect(QRect(x, y, cell, cell), ((x + y) / cell) % 2 ? QColor(0x99, 0x99, 0x99) : QColor(0xdd, 0xdd, 0xdd));
            }
        }
        p.fillRect(box, color);
        p.setPen(QColor(0x80, 0x80, 0x80, 160));
        p.drawRect(box);
        return QIcon(pixmap);
    }

    QString colorText(const QColor &color)
    {
        return color.alpha() < 255 ? color.name(QColor::HexArgb) : color.name();
    }

    int colorDistance(const QColor &a, const QColor &b)
    {
        const int dr = a.red() - b.red();
        const int dg = a.green() - b.green();
        const int db = a.blue() - b.blue();
        return dr * dr + dg * dg + db * db;
    }
}

ColorSchemeDialog::ColorSchemeDialog(QWidget *parent) : QDialog(parent), ui(new Ui::ColorSchemeDialog)
{
    this->ui->setupUi(this);
    this->m_settings = CFG->Colors;
    this->m_settings.Normalize();
    this->m_dark = ColorScheme::DetectDarkMode();

    this->ui->rootLayout->setContentsMargins(UiMetrics::Space::XL, UiMetrics::Space::L, UiMetrics::Space::XL, UiMetrics::Space::L);
    this->ui->rootLayout->setSpacing(UiMetrics::Space::L);

    auto *top = new QHBoxLayout();
    this->m_mode = new SegmentedControl(this);
    this->m_mode->AddSegment(tr("Simple"), tr("Pick a base color for each category"));
    this->m_mode->AddSegment(tr("Advanced"), tr("Customize every single color"));
    this->m_mode->SetCurrentIndex(ModeSimple);
    this->m_resetButton = new QPushButton(this);
    top->addWidget(this->m_mode);
    top->addStretch(1);
    top->addWidget(this->m_resetButton);

    this->m_pages = new QStackedWidget(this);
    this->m_pages->addWidget(this->buildSimplePage());
    this->m_pages->addWidget(this->buildAdvancedPage());

    this->ui->rootLayout->insertLayout(0, top);
    this->ui->rootLayout->insertWidget(1, this->m_pages, 1);

    connect(this->m_mode, &SegmentedControl::activated, this, &ColorSchemeDialog::setMode);
    connect(this->m_resetButton, &QPushButton::clicked, this, [this]()
    {
        if (this->m_pages->currentIndex() == ModeSimple)
            this->m_settings = ColorScheme::Settings::Defaults();
        else
            this->m_settings.Overrides.clear();
        this->refresh();
    });
    connect(this->ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(this->ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    this->setMode(ModeSimple);
    this->refresh();
}

ColorSchemeDialog::~ColorSchemeDialog()
{
    delete this->ui;
}

QWidget *ColorSchemeDialog::buildSimplePage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(UiMetrics::Space::S);

    const QColor muted = UiMetrics::SecondaryTextColor(this->palette());

    // Palette
    auto *paletteHeader = new QHBoxLayout();
    auto *paletteTitle = new QLabel(tr("Palette"), page);
    WidgetStyle::ApplyTextStyle(paletteTitle, QColor(), UiMetrics::TextRole::Strong);
    auto *paletteHint = new QLabel(tr("Pick base colors, matching tones are generated automatically"), page);
    WidgetStyle::ApplyTextStyle(paletteHint, muted, UiMetrics::TextRole::Caption);
    paletteHeader->addWidget(paletteTitle);
    paletteHeader->addStretch(1);
    paletteHeader->addWidget(paletteHint);
    layout->addLayout(paletteHeader);

    auto *paletteCard = new CardFrame(page);
    auto *paletteCardLayout = new QHBoxLayout(paletteCard);
    paletteCardLayout->setContentsMargins(UiMetrics::Space::M, UiMetrics::Space::M, UiMetrics::Space::M, UiMetrics::Space::M);
    paletteCardLayout->setSpacing(UiMetrics::Space::S);
    this->m_paletteLayout = new QHBoxLayout();
    this->m_paletteLayout->setSpacing(UiMetrics::Space::S);
    this->m_addButton = new QPushButton(tr("Add"), paletteCard);
    this->m_addButton->setIcon(QIcon::fromTheme("list-add"));
    this->m_addButton->setToolTip(tr("Add a color to the palette"));
    paletteCardLayout->addLayout(this->m_paletteLayout);
    paletteCardLayout->addStretch(1);
    paletteCardLayout->addWidget(this->m_addButton);
    connect(this->m_addButton, &QPushButton::clicked, this, &ColorSchemeDialog::addPaletteColor);
    layout->addWidget(paletteCard);

    layout->addSpacing(UiMetrics::Space::S);

    // Categories
    auto *categoriesTitle = new QLabel(tr("Categories"), page);
    WidgetStyle::ApplyTextStyle(categoriesTitle, QColor(), UiMetrics::TextRole::Strong);
    layout->addWidget(categoriesTitle);

    auto *categoriesCard = new CardFrame(page);
    auto *categoriesLayout = new QVBoxLayout(categoriesCard);
    // Rows run edge to edge so the dividers between them span the whole card.
    categoriesLayout->setContentsMargins(0, 0, 0, 0);
    categoriesLayout->setSpacing(0);
    for (int i = 0; i < ColorScheme::CategoryCount; ++i)
    {
        const Category category = static_cast<Category>(i);
        QVector<qreal> primary;
        QVector<qreal> secondary;
        previewSamples(category, primary, secondary);

        if (i > 0)
            categoriesLayout->addWidget(new Divider(categoriesCard));

        auto *row = new QHBoxLayout();
        const int rowPadding = UiMetrics::Space::S + UiMetrics::Space::XXS;
        row->setContentsMargins(UiMetrics::Space::M, rowPadding, UiMetrics::Space::M, rowPadding);
        row->setSpacing(UiMetrics::Space::M);
        auto *preview = new CategoryPreview(primary, secondary, categoriesCard);

        auto *text = new QVBoxLayout();
        text->setSpacing(UiMetrics::Space::XXS);
        auto *name = new QLabel(ColorScheme::CategoryName(category), categoriesCard);
        auto *description = new QLabel(categoryDescription(category), categoriesCard);
        WidgetStyle::ApplyTextStyle(description, muted, UiMetrics::TextRole::Caption);
        description->setMinimumWidth(1);
        text->addStretch(1);
        text->addWidget(name);
        text->addWidget(description);
        text->addStretch(1);

        auto *swatches = new QHBoxLayout();
        swatches->setSpacing(UiMetrics::Space::XXS);

        row->addWidget(preview);
        row->addLayout(text, 1);
        row->addLayout(swatches);
        categoriesLayout->addLayout(row);
        this->m_rows.append({ category, preview, name, swatches });
    }
    layout->addWidget(categoriesCard);
    layout->addStretch(1);

    // Many palette colors can make the rows wider than the dialog; scroll instead of clipping.
    auto *scroll = new QScrollArea(this);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setWidget(page);
    return scroll;
}

QWidget *ColorSchemeDialog::buildAdvancedPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(UiMetrics::Space::S);

    auto *top = new QHBoxLayout();
    auto *hint = new QLabel(tr("Click any color to customize it."), page);
    WidgetStyle::ApplyTextStyle(hint, UiMetrics::SecondaryTextColor(this->palette()), UiMetrics::TextRole::Body);
    this->m_filter = new QLineEdit(page);
    this->m_filter->setPlaceholderText(tr("Filter colors"));
    this->m_filter->setClearButtonEnabled(true);
    this->m_filter->addAction(QIcon::fromTheme("edit-find"), QLineEdit::LeadingPosition);
    this->m_filter->setMaximumWidth(260);
    top->addWidget(hint, 1);
    top->addWidget(this->m_filter);
    layout->addLayout(top);

    this->m_tree = new QTreeWidget(page);
    this->m_tree->setColumnCount(4);
    this->m_tree->setHeaderHidden(true);
    this->m_tree->setUniformRowHeights(true);
    this->m_tree->setSelectionMode(QAbstractItemView::NoSelection);
    this->m_tree->setFocusPolicy(Qt::NoFocus);
    this->m_tree->setIconSize(QSize(28, 16));
    UIHelper::ApplyItemViewStyle(this->m_tree);
    this->m_tree->header()->setStretchLastSection(false);
    this->m_tree->header()->setSectionResizeMode(kNameColumn, QHeaderView::Stretch);
    this->m_tree->header()->setSectionResizeMode(kValueColumn, QHeaderView::ResizeToContents);
    this->m_tree->header()->setSectionResizeMode(kStateColumn, QHeaderView::ResizeToContents);
    // Fixed, so the list does not shift sideways when the first reset icon appears.
    this->m_tree->header()->setSectionResizeMode(kResetColumn, QHeaderView::Fixed);
    this->m_tree->header()->resizeSection(kResetColumn, UiMetrics::Space::XXL);
    layout->addWidget(this->m_tree, 1);

    for (int i = 0; i <= ColorScheme::CategoryCount; ++i)
    {
        auto *group = new QTreeWidgetItem(this->m_tree);
        group->setText(kNameColumn, ColorScheme::CategoryName(static_cast<Category>(i)));
        QFont font = group->font(kNameColumn);
        font.setWeight(QFont::DemiBold);
        group->setFont(kNameColumn, font);
        this->m_groupItems.insert(i, group);
    }

    for (const ColorScheme::ColorField &field : ColorScheme::Fields())
    {
        if (!field.Label)
            continue;
        QTreeWidgetItem *group = this->m_groupItems.value(static_cast<int>(field.Group));
        auto *item = new QTreeWidgetItem(group);
        item->setText(kNameColumn, QCoreApplication::translate("ColorScheme", field.Label));
        item->setToolTip(kValueColumn, tr("Click to change"));
        this->m_fieldItems.append({ &field, item });
    }
    this->m_groupItems.value(static_cast<int>(Category::Cpu))->setExpanded(true);

    connect(this->m_tree, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *item, int column)
    {
        for (const FieldItem &entry : std::as_const(this->m_fieldItems))
        {
            if (entry.Item != item)
                continue;
            const QString name = QString::fromLatin1(entry.Field->Name);
            // The reset column only reacts while the color is custom (it shows its icon then).
            if (column == kResetColumn && this->m_settings.Overrides.contains(name))
            {
                this->m_settings.Overrides.remove(name);
                this->refresh();
            } else
            {
                this->editField(entry.Field);
            }
            return;
        }
        // Group rows toggle on a single click, like the rows under them react to one.
        if (item->childCount() > 0)
            item->setExpanded(!item->isExpanded());
    });
    connect(this->m_filter, &QLineEdit::textChanged, this, &ColorSchemeDialog::applyFilter);
    return page;
}

void ColorSchemeDialog::setMode(int mode)
{
    this->m_pages->setCurrentIndex(mode);
    this->m_mode->SetCurrentIndex(mode);
    this->m_resetButton->setText(mode == ModeSimple ? tr("Restore defaults") : tr("Reset all to automatic"));
}

void ColorSchemeDialog::refresh()
{
    this->m_settings.Normalize();
    this->refreshPalette();
    this->refreshCategories();
    this->refreshAdvanced();
}

void ColorSchemeDialog::refreshPalette()
{
    while (QLayoutItem *item = this->m_paletteLayout->takeAt(0))
    {
        // Deferred: the swatch being deleted may be the one whose click triggered this refresh.
        if (QWidget *widget = item->widget())
        {
            widget->hide();
            widget->deleteLater();
        }
        delete item;
    }

    for (int i = 0; i < this->m_settings.Palette.size(); ++i)
    {
        auto *swatch = new ColorSwatch(this->m_settings.Palette.at(i), kPaletteSwatchSize, this->m_paletteLayout->parentWidget());
        swatch->setToolTip(tr("%1, click to change or remove").arg(this->m_settings.Palette.at(i).name()));
        connect(swatch, &QAbstractButton::clicked, this, [this, i, swatch]() { this->showPaletteMenu(i, swatch); });
        this->m_paletteLayout->addWidget(swatch);
    }
    this->m_addButton->setEnabled(this->m_settings.Palette.size() < kMaxPaletteColors);
}

void ColorSchemeDialog::refreshCategories()
{
    const ColorScheme scheme = ColorScheme::Resolve(this->m_settings, this->m_dark);
    for (const CategoryRow &row : std::as_const(this->m_rows))
    {
        const PreviewColors colors = previewColors(scheme, row.Category);
        static_cast<CategoryPreview *>(row.Preview)->SetColors(colors.Line, colors.Fill, colors.Second);
        WidgetStyle::ApplyTextStyle(row.Name, colors.Title, UiMetrics::TextRole::Strong);

        while (QLayoutItem *item = row.Swatches->takeAt(0))
        {
            if (QWidget *widget = item->widget())
            {
                widget->hide();
                widget->deleteLater();
            }
            delete item;
        }
        const int slot = static_cast<int>(row.Category);
        for (int i = 0; i < this->m_settings.Palette.size(); ++i)
        {
            auto *swatch = new ColorSwatch(this->m_settings.Palette.at(i), kCategorySwatchSize, row.Preview->parentWidget());
            swatch->setCheckable(true);
            swatch->setChecked(this->m_settings.Assignments.at(slot) == i);
            swatch->setAccessibleName(tr("%1: %2").arg(ColorScheme::CategoryName(row.Category), this->m_settings.Palette.at(i).name()));
            connect(swatch, &QAbstractButton::clicked, this, [this, slot, i]()
            {
                this->m_settings.Assignments[slot] = i;
                this->refresh();
            });
            row.Swatches->addWidget(swatch);
        }
    }
}

void ColorSchemeDialog::refreshAdvanced()
{
    const ColorScheme automatic = ColorScheme::Resolve(this->m_settings, this->m_dark, false);
    const ColorScheme resolved = ColorScheme::Resolve(this->m_settings, this->m_dark);
    const QColor muted = UiMetrics::SecondaryTextColor(this->palette());
    const QColor text = this->palette().color(QPalette::Text);
    const QSize iconSize = this->m_tree->iconSize();
    const QIcon resetIcon = QIcon::fromTheme("edit-undo");

    QHash<int, int> totals;
    QHash<int, int> customs;
    for (const FieldItem &entry : std::as_const(this->m_fieldItems))
    {
        const QString name = QString::fromLatin1(entry.Field->Name);
        const bool custom = this->m_settings.Overrides.contains(name) && resolved.*(entry.Field->Member) != automatic.*(entry.Field->Member);
        const QColor color = resolved.*(entry.Field->Member);
        const int group = static_cast<int>(entry.Field->Group);

        entry.Item->setIcon(kValueColumn, swatchIcon(color, iconSize));
        entry.Item->setText(kValueColumn, colorText(color));
        entry.Item->setForeground(kValueColumn, custom ? text : muted);
        if (custom)
            entry.Item->setText(kStateColumn, tr("Custom"));
        else
            entry.Item->setText(kStateColumn, entry.Field->Group == Category::General ? tr("System") : tr("Auto"));
        entry.Item->setForeground(kStateColumn, custom ? text : muted);
        entry.Item->setIcon(kResetColumn, custom ? resetIcon : QIcon());
        entry.Item->setText(kResetColumn, custom && resetIcon.isNull() ? QStringLiteral("↺") : QString());
        entry.Item->setToolTip(kResetColumn, custom ? tr("Return to the automatic color") : QString());

        totals[group] += 1;
        if (custom)
            customs[group] += 1;
    }

    for (auto it = this->m_groupItems.constBegin(); it != this->m_groupItems.constEnd(); ++it)
    {
        const Category category = static_cast<Category>(it.key());
        QTreeWidgetItem *group = it.value();
        const int total = totals.value(it.key());
        const int custom = customs.value(it.key());
        if (category == Category::General)
            group->setIcon(kNameColumn, QIcon());
        else
            group->setIcon(kNameColumn, swatchIcon(this->m_settings.BaseColor(category), QSize(12, 12)));
        if (custom > 0)
            group->setText(kStateColumn, tr("%1 of %2 custom").arg(custom).arg(total));
        else
            group->setText(kStateColumn, total == 1 ? tr("1 color") : tr("%1 colors").arg(total));
        group->setForeground(kStateColumn, muted);
    }
}

void ColorSchemeDialog::showPaletteMenu(int index, QWidget *anchor)
{
    QMenu menu(this);
    QAction *change = menu.addAction(QIcon::fromTheme("color-picker"), tr("Change color..."));
    QAction *remove = menu.addAction(QIcon::fromTheme("list-remove"), tr("Remove"));
    remove->setEnabled(this->m_settings.Palette.size() > 1);
    QAction *picked = menu.exec(anchor->mapToGlobal(QPoint(0, anchor->height())));
    if (picked == change)
        this->changePaletteColor(index);
    else if (picked == remove)
        this->removePaletteColor(index);
}

void ColorSchemeDialog::changePaletteColor(int index)
{
    const QColor color = QColorDialog::getColor(this->m_settings.Palette.at(index), this, tr("Palette color"));
    if (!color.isValid())
        return;
    this->m_settings.Palette[index] = color;
    this->refresh();
}

void ColorSchemeDialog::removePaletteColor(int index)
{
    if (this->m_settings.Palette.size() <= 1)
        return;

    // Categories using the color move to the closest one that stays.
    const QColor removed = this->m_settings.Palette.at(index);
    int closest = -1;
    for (int i = 0; i < this->m_settings.Palette.size(); ++i)
    {
        if (i != index && (closest < 0 || colorDistance(this->m_settings.Palette.at(i), removed) < colorDistance(this->m_settings.Palette.at(closest), removed)))
            closest = i;
    }

    QStringList users;
    for (int slot = 0; slot < ColorScheme::CategoryCount; ++slot)
    {
        if (this->m_settings.Assignments.at(slot) == index)
            users.append(ColorScheme::CategoryName(static_cast<Category>(slot)));
    }
    if (!users.isEmpty())
    {
        const QMessageBox::StandardButton answer = QMessageBox::question(this, tr("Remove color"),
            users.size() == 1 ? tr("%1 uses this color and will switch to the closest remaining one.").arg(users.constFirst())
                              : tr("%1 use this color and will switch to the closest remaining one.").arg(users.join(", ")));
        if (answer != QMessageBox::Yes)
            return;
    }

    this->m_settings.Palette.removeAt(index);
    for (int &assigned : this->m_settings.Assignments)
    {
        if (assigned == index)
            assigned = closest > index ? closest - 1 : closest;
        else if (assigned > index)
            --assigned;
    }
    this->refresh();
}

void ColorSchemeDialog::addPaletteColor()
{
    const QColor initial = this->m_settings.Palette.isEmpty() ? QColor(Qt::gray) : this->m_settings.Palette.constLast();
    const QColor color = QColorDialog::getColor(initial, this, tr("New palette color"));
    if (!color.isValid())
        return;
    this->m_settings.Palette.append(color);
    this->refresh();
}

void ColorSchemeDialog::editField(const ColorScheme::ColorField *field)
{
    const ColorScheme resolved = ColorScheme::Resolve(this->m_settings, this->m_dark);
    const QColor color = QColorDialog::getColor(resolved.*(field->Member), this,
                                                QCoreApplication::translate("ColorScheme", field->Label),
                                                QColorDialog::ShowAlphaChannel);
    if (!color.isValid())
        return;
    this->m_settings.Overrides.insert(QString::fromLatin1(field->Name), color.name(QColor::HexArgb));
    this->refresh();
}

void ColorSchemeDialog::applyFilter(const QString &text)
{
    const QString needle = text.trimmed();
    for (QTreeWidgetItem *group : std::as_const(this->m_groupItems))
    {
        const bool groupMatches = group->text(kNameColumn).contains(needle, Qt::CaseInsensitive);
        int visible = 0;
        for (int i = 0; i < group->childCount(); ++i)
        {
            QTreeWidgetItem *child = group->child(i);
            const bool show = needle.isEmpty() || groupMatches || child->text(kNameColumn).contains(needle, Qt::CaseInsensitive);
            child->setHidden(!show);
            if (show)
                ++visible;
        }
        group->setHidden(visible == 0);
        if (!needle.isEmpty())
            group->setExpanded(visible > 0);
    }
}
