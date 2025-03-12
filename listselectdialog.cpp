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

#include "listselectdialog.h"
#include "ui_listselectdialog.h"

ListSelectDialog::ListSelectDialog(
    QWidget *parent, QStringList strings, QStringList selected) :
    QDialog(parent),
    ui(new Ui::ListSelectDialog)
{
    ui->setupUi(this);

    // build up the unselected list from the big list minus the selected list
    for(auto i = strings.begin(); i != strings.end(); ++i)
    {
        if(!selected.contains(*i, Qt::CaseInsensitive))
            ui->unselectedColumnsList->addItem(*i);
    }

    for(auto i = selected.begin(); i != selected.end(); ++i)
    {
        ui->selectedColumnsList->addItem(*i);
    }

    connect(ui->moveUpButton, &QToolButton::clicked,
            this, &ListSelectDialog::MoveUp);
    connect(ui->moveDownButton, &QToolButton::clicked,
            this, &ListSelectDialog::MoveDown);
    connect(ui->selectButton, &QToolButton::clicked,
            this, &ListSelectDialog::MoveToSelected);
    connect(ui->unselectButton, &QToolButton::clicked,
            this, &ListSelectDialog::MoveToUnselected);
}

ListSelectDialog::~ListSelectDialog()
{
    delete ui;
}

QStringList ListSelectDialog::SelectedStrings() const
{
    QStringList list;

    for(int row=0; row<ui->selectedColumnsList->count(); ++row)
        list.append(ui->selectedColumnsList->item(row)->text());

    return list;
}

void ListSelectDialog::MoveUp()
{
    auto items = ui->selectedColumnsList->selectedItems();
    if(items.count() != 1) return;

    int row = ui->selectedColumnsList->row(items[0]);
    if(row > 0)
    {
        QListWidgetItem *item = ui->selectedColumnsList->takeItem(row);
        ui->selectedColumnsList->insertItem(row-1, item);
        ui->selectedColumnsList->setCurrentRow(row-1);
    }
}

void ListSelectDialog::MoveDown()
{
    auto items = ui->selectedColumnsList->selectedItems();
    if(items.count() != 1) return;

    int row = ui->selectedColumnsList->row(items[0]);
    if(row < ui->selectedColumnsList->count()-1)
    {
        QListWidgetItem *item = ui->selectedColumnsList->takeItem(row);
        ui->selectedColumnsList->insertItem(row+1, item);
        ui->selectedColumnsList->setCurrentRow(row+1);
    }
}

void ListSelectDialog::MoveToSelected()
{
    auto items = ui->unselectedColumnsList->selectedItems();

    for(auto i: items)
    {
        int row = ui->unselectedColumnsList->row(i);
        QListWidgetItem *item = ui->unselectedColumnsList->takeItem(row);
        ui->selectedColumnsList->addItem(item);
    }
}

void ListSelectDialog::MoveToUnselected()
{
    auto items = ui->selectedColumnsList->selectedItems();

    for(auto i: items)
    {
        int row = ui->selectedColumnsList->row(i);
        QListWidgetItem *item = ui->selectedColumnsList->takeItem(row);
        ui->unselectedColumnsList->addItem(item);
    }
}
