/*  Copyright (C) 2016 by Martin Langlotz alias stackshadow

    This file is part of doDB.

    doDB is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    doDB is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with doDB.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef DODBMAINWINDOW_H
#define DODBMAINWINDOW_H

#include <QMainWindow>
#include <QHBoxLayout>

#include "doDBMainWindowUi.ui.h"
#include "doDBConnection/doDBConnection.h"
#include "doDBTree/doDBTree.h"
#include "doDBEntryEditor/doDBEntryEditor.h"

class doDBMainWindow :
QWidget
{
    Q_OBJECT

public:
    doDBMainWindow( QWidget *parent );
    ~doDBMainWindow();


private:
    void            treeUpdate();

private slots:
    void            connectionEditorShow();
    void            connectionEditorHide();
    void            onBtnTableEditClick();
    void            debugMessagesTrigger();
    void            treeElementClicked( QTreeWidgetItem * item, int column );
    void            entryEditorItemSaveNew( etDBObject *dbObject, const char *tableName );
    void            entryEditorItemChanged( etDBObject *dbObject, const char *tableName );

private:
    Ui::doDBMainWindow          ui;

    doDBtree                    *dataTree;
    QLayout                     *itemViewLayout;
    doDBEntryEditor             *entryEditor;
    QString                     entryEditorConnID;
    QString                     entryEditorLockID;


};

#endif // DODBMAINWINDOW_H
