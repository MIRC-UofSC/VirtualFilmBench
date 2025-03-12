//-----------------------------------------------------------------------------
// This file is part of Virtual Film Bench
//
// Copyright (c) 2025 University of South Carolina and Thomas Aschenbach
//
// Project contributors include: Thomas Aschenbach (Colorlab, inc.),
// L. Scott Johnson (USC), Greg Wilsbacher (USC), Pingping Cai (USC),
// and Stella Garcia (USC).
//
// Funding for Virtual Film Bench development was provided through a grant
// from the National Endowment for the Humanities with additional support
// from the National Science Foundation’s Access program.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3 of the License, or (at your
// option) any later version.
//
// Virtual Film Bench is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, see http://gnu.org/licenses/.
//
// For inquiries or permissions, contact
// Greg Wilsbacher (gregw@mailbox.sc.edu)
//-----------------------------------------------------------------------------

#include "decimalelidedelegate.h"

#include <QApplication>
#include <QPainter>

// Fix for Qt's poor decimal eliding algorithm from
// https://stackoverflow.com/questions/64198197
// how-to-prevent-too-aggressive-text-elide-in-qtableview

DecimalElideDelegate::DecimalElideDelegate(QObject *parent)
    : QStyledItemDelegate{parent}
{
    column = -1;
}

QSize DecimalElideDelegate::sizeHint(
    const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    //Get the original view parameters.
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    return size;
}

void DecimalElideDelegate::paint(
    QPainter *painter,
    const QStyleOptionViewItem &option,
    const QModelIndex &index) const
{
    if (!index.isValid())
        return;

    if(index.column() != column)
    {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

#ifdef CONFIDENCE_PROGRESSBAR
    int progress = int(index.data().toDouble() * 100.0);

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    int xpadding = 3;
    int ypadding = (opt.rect.height()-opt.fontMetrics.height())/2 - xpadding;
    if(ypadding < xpadding) ypadding = xpadding;

    painter->save();
    painter->setClipRect(opt.rect);
    opt.rect = opt.rect.adjusted(xpadding, ypadding, -xpadding, -ypadding);

    QStyleOptionProgressBar progressBarOption;
    progressBarOption.rect = opt.rect;
    progressBarOption.minimum = 0;
    progressBarOption.maximum = 100;
    progressBarOption.progress = progress;
    progressBarOption.text =
        opt.fontMetrics.elidedText(opt.text, Qt::ElideRight, opt.rect.width());
    progressBarOption.textVisible = true;
    progressBarOption.textAlignment = Qt::AlignCenter;

    QApplication::style()->drawControl(QStyle::CE_ProgressBar,
                                       &progressBarOption, painter);

    painter->restore();

#else
    /*
     *  basic elide -- no progress bar
     */
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    int padding = 3;

    painter->save();
    painter->setClipRect(opt.rect);
    opt.rect = opt.rect.adjusted(padding, padding, -padding, -padding);
    if(opt.state & QStyle::State_Selected) painter->setPen(Qt::white);
    painter->drawText(
        opt.rect,
        Qt::AlignLeft | Qt::AlignVCenter,
        opt.fontMetrics.elidedText(opt.text, Qt::ElideRight, opt.rect.width()));
    painter->restore();
#endif

}
