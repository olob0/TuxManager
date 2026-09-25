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

#ifndef UI_SEGMENTEDCONTROL_H
#define UI_SEGMENTEDCONTROL_H

#include <QString>
#include <QVector>
#include <QWidget>

//! Compact row of mutually exclusive options ("Overall | Per core", "List | Tree").
//! Painted from the active palette, so it follows the Qt theme like the rest of the UI.
class SegmentedControl : public QWidget
{
    Q_OBJECT

    public:
        explicit SegmentedControl(QWidget *parent = nullptr);

        //! Appends a segment and returns its index. toolTip may carry a longer description.
        int AddSegment(const QString &text, const QString &toolTip = QString());
        //! Selects a segment without emitting activated().
        void SetCurrentIndex(int index);
        int CurrentIndex() const { return this->m_current; }

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    signals:
        //! Emitted when the user picks a segment with the mouse or keyboard.
        void activated(int index);

    protected:
        bool event(QEvent *event) override;
        void paintEvent(QPaintEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;
        void mouseMoveEvent(QMouseEvent *event) override;
        void leaveEvent(QEvent *event) override;
        void keyPressEvent(QKeyEvent *event) override;
        void changeEvent(QEvent *event) override;

    private:
        struct Segment
        {
            QString Text;
            QString ToolTip;
        };

        int segmentWidth(int index) const;
        int segmentHeight() const;
        QRect segmentRect(int index) const;
        int segmentAt(const QPoint &pos) const;
        void activate(int index);

        QVector<Segment> m_segments;
        int              m_current { -1 };
        int              m_hovered { -1 };
};

#endif // UI_SEGMENTEDCONTROL_H
