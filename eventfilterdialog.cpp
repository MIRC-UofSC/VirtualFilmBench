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

#include "eventfilterdialog.h"
#include "mainwindow.h"

#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QRadioButton>

#include "ui_eventfilterdialog.h"

const QVector<EventFilterComp> attrDisplayOrder = {
    EVENT_FILTER_COMP_ATTR_EQUAL,
    EVENT_FILTER_COMP_ATTR_NOT_EQUAL,
    EVENT_FILTER_COMP_ATTR_CONTAINS,
    EVENT_FILTER_COMP_ATTR_DOES_NOT_CONTAIN,
    EVENT_FILTER_COMP_ATTR_STARTS_WITH,
    EVENT_FILTER_COMP_ATTR_DOES_NOT_START_WITH,
    EVENT_FILTER_COMP_ATTR_EXISTS,
    EVENT_FILTER_COMP_ATTR_DOES_NOT_EXIST,
    EVENT_FILTER_COMP_ATTR_EQ,
    EVENT_FILTER_COMP_ATTR_NE,
    EVENT_FILTER_COMP_ATTR_LT,
    EVENT_FILTER_COMP_ATTR_GT,
    EVENT_FILTER_COMP_ATTR_LE,
    EVENT_FILTER_COMP_ATTR_GE
};

const QList<EventFilterComp> posDisplayOrder = {
    EVENT_FILTER_COMP_POS_LEFT,
    EVENT_FILTER_COMP_POS_RIGHT,
    EVENT_FILTER_COMP_POS_SPAN,
    EVENT_FILTER_COMP_POS_NOT_SPAN,
    EVENT_FILTER_COMP_POS_TOP,
    EVENT_FILTER_COMP_POS_BOTTOM
};

EventFilterDialog::EventFilterDialog(
    QWidget *parent,
    const EventFilter *f,
    EventFilterMatchingMode matchMode)
    :
    QDialog(parent),
    ui(new Ui::EventFilterDialog)
{
    ui->setupUi(this);

    QRect sz = this->geometry();
    if(sz.width() < 500)
        this->resize(500, sz.height());

    matchModeGroupBox = new QGroupBox;
    QHBoxLayout *modeLayout = new QHBoxLayout;
    buttonAll =
        new QRadioButton(
        EventFilter::MatchingModeNames()[EVENT_FILTER_MATCH_ALL]);
    buttonAny =
        new QRadioButton(
            EventFilter::MatchingModeNames()[EVENT_FILTER_MATCH_ANY]);

    if(matchMode == EVENT_FILTER_MATCH_ALL)
        buttonAll->setChecked(true);
    else
        buttonAny->setChecked(true);

    modeLayout->addWidget(buttonAll);
    modeLayout->addWidget(buttonAny);
    modeLayout->addStretch(1);
    matchModeGroupBox->setLayout(modeLayout);
    ui->verticalLayout->insertWidget(1,matchModeGroupBox);
    matchModeGroupBox->setVisible(false);
    matchModeGroupBox->setFlat(true);

    numRows = 0;

    // add controls for the current set of conditions
    if(f)
    {
        for(auto cond=f->Conditions().cbegin(), end=f->Conditions().cend();
            cond != end;
            ++cond)
        {
            AddRow(cond->Attribute(), cond->Comparison(), cond->Pattern());
        }

        if(f->MatchMode() == EVENT_FILTER_MATCH_ANY)
        {
            buttonAll->setChecked(false);
            buttonAny->setChecked(true);
        }
    }

    // add one more row for the user to add a condition
    AddRow();
}

EventFilterDialog::~EventFilterDialog()
{
    delete ui;
}

EventFilter EventFilterDialog::Filter() const
{
    EventFilter filter;

    int row, col;

    for(row=0; row<numRows; ++row)
    {
        QLayoutItem *item;

        col = 0;

        item = ui->filterGrid->itemAtPosition(row,col++);
        if(!item)
        {
            qDebug() << "Error: EventFilterDialog attribute item missing";
            return EventFilter();
        }
        QComboBox *attrBox = qobject_cast<QComboBox *>(item->widget());
        if(!attrBox)
        {
            qDebug() << "Error: EventFilterDialog attribute widget missing";
            return EventFilter();
        }

        item = ui->filterGrid->itemAtPosition(row,col++);
        if(!item)
        {
            qDebug() << "Error: EventFilterDialog comparison item missing";
            return EventFilter();
        }
        QComboBox *compBox = qobject_cast<QComboBox *>(item->widget());
        if(!compBox)
        {
            qDebug() << "Error: EventFilterDialog comparison widget missing";
            return EventFilter();
        }

        item = ui->filterGrid->itemAtPosition(row,col++);
        if(!item)
        {
            qDebug() << "Error: EventFilterDialog pattern item missing";
            return EventFilter();
        }
        QLineEdit *pat = qobject_cast<QLineEdit *>(item->widget());
        if(!pat)
        {
            qDebug() << "Error: EventFilterDialog pattern widget missing";
            return EventFilter();
        }

        // skip the lines that have the attr set to the dummy position
        if(attrBox->currentIndex() == 0) continue;

        if(attrBox->currentIndex() == 1)
            filter.AddCondition(
                EventFilterComp(posDisplayOrder[compBox->currentIndex()])
                );
        else
            filter.AddCondition(
                attrBox->currentText(),
                pat->text(),
                EventFilterComp(attrDisplayOrder[compBox->currentIndex()])
                );
    }

    if(buttonAll->isChecked())
        filter.SetMatchMode(EVENT_FILTER_MATCH_ALL);
    else
        filter.SetMatchMode(EVENT_FILTER_MATCH_ANY);

    return filter;
}

void EventFilterDialog::AddRow(
    QString attribute, EventFilterComp comparison,QString pattern)
{
    MainWindow *mainwindow = MainWindowAncestor(this);
    if(!mainwindow) return;
    QStringList attributes = mainwindow->vbscan.AttributesInUse();

    int row = numRows; // not ui->filterGrid->rowCount(), which is 1 when empty

    int col=0;

    QComboBox *attrBox = new QComboBox(this);
    attrBox->addItem("-");
    attrBox->setToolTip("Set to '-' to remove this filter condition.");

    attrBox->addItem("<Position>");

    if(!attributes.isEmpty()) attrBox->addItems(attributes);

    if(EventFilterCompIsPos(comparison))
    {
        attrBox->setCurrentIndex(1);
    }
    else if(!attribute.isEmpty())
    {
        int p = attrBox->findText(attribute,Qt::MatchExactly);
        if(p<0)
        {
            p = attrBox->count();
            attrBox->addItem(attribute);
        }
        attrBox->setCurrentIndex(p);
    }

    ui->filterGrid->addWidget(attrBox, row, col++, 1, 1);

    QComboBox *compBox = new QComboBox();

    QList<EventFilterComp> displayOrder;
    if(EventFilterCompIsPos(comparison))
        displayOrder = posDisplayOrder;
    else
        displayOrder = attrDisplayOrder;

    int idx=0;
    for(auto i : displayOrder )
    {
        compBox->addItem(EventFilter::ConditionNames()[i]);
        if(i == comparison) compBox->setCurrentIndex(idx);
        idx++;
    }

    ui->filterGrid->addWidget(compBox, row, col++);

    QLineEdit *pat = new QLineEdit();
    if(!pattern.isEmpty()) pat->setText(pattern);
    ui->filterGrid->addWidget(pat, row, col++);

    //for(int r=0; r<=row; ++r)
      //  ui->filterGrid->setRowMinimumHeight(r,height);

    ActivateRow(attrBox);

    connect(attrBox, &QComboBox::currentIndexChanged,
            this, &EventFilterDialog::ActivateThisRow);

    if(numRows >= 2) matchModeGroupBox->setVisible(true);

    numRows++;
}

void EventFilterDialog::ActivateRow(int row, bool activate)
{
    QLayoutItem *item;
    int nCol = ui->filterGrid->columnCount();

    // leave the attribute selector enabled regardless
    // so the loop starts at column 1, not 0
    for(int col=1; col<nCol; ++col)
    {
        if((item = ui->filterGrid->itemAtPosition(row,col)) != nullptr)
            if(item->widget())
                item->widget()->setEnabled(activate);
    }
}

void EventFilterDialog::ActivateRow(QComboBox *attrBox)
{
    int index = ui->filterGrid->indexOf(attrBox);
    Q_ASSERT(index >= 0);

    int row; // the only one of the four we actually need
    int col, rowspan, colspan;
    ui->filterGrid->getItemPosition(index, &row, &col, &rowspan, &colspan);

    this->ActivateRow(row, (attrBox->currentIndex()!=0));

    if(attrBox->currentIndex()!=0)
    {
        if(row == numRows-1) AddRow();
    }
    else
        return;

    QList<EventFilterComp> displayOrder;
    if(attrBox->currentIndex() == 1)
        displayOrder = posDisplayOrder;
    else
        displayOrder = attrDisplayOrder;

    QLayoutItem *item = ui->filterGrid->itemAtPosition(row,1);
    if(!item)
    {
        qDebug() << "Error: EventFilterDialog comparison item missing";
        return;
    }
    QComboBox *compBox = qobject_cast<QComboBox *>(item->widget());
    if(!compBox)
    {
        qDebug() << "Error: EventFilterDialog comparison widget missing";
        return;
    }
    if(compBox->itemText(0).compare(
            EventFilter::ConditionNames()[displayOrder.at(0)]) != 0)
    {
        compBox->clear();
        for(auto i : displayOrder )
            compBox->addItem(EventFilter::ConditionNames()[i]);
        compBox->setCurrentIndex(0);
    }
}

void EventFilterDialog::ActivateThisRow()
{
    QComboBox *attrBox = qobject_cast<QComboBox *>(sender());
    Q_ASSERT(attrBox);

    ActivateRow(attrBox);
}
