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

#ifndef doDBConnectionEditorC_H
#define doDBConnectionEditorC_H

#include <QObject>
#include <QWidget>
#include <QTableWidget>

#include "doDBConnection/doDBConnection.h"
#include "doDBConnection/doDBConnectionEditor.ui.h"

class doDBConnectionEditor :
public QWidget
{
Q_OBJECT

public:
    enum editMode {
        MODE_NOTHING = 0,
        MODE_SELECT,
        MODE_ADD,
        MODE_CHANGE,
        MODE_REMOVE
    };

public:
    doDBConnectionEditor( QWidget *parent );
    ~doDBConnectionEditor();


private:
    void        loadConnectionsFromConfig();
    void        saveConnectionsToConfig();
    void        tblConnectionRefresh();


private slots:
    void        showSelectedConnection();
    void        connectionAdd();
    void        connectionSave();
    void        connectionDelete();
    void        selectSQLiteFilename();
    void        dbConnect();
    void        dbDisconnect();
    void        closeClicked();


signals:
    void        finished();


private:
    Ui::doDBConnectionEditorUi      ui;
    int                             mode;
    doDBConnection                  *selectedConnection;

};

#endif // DODBCONNECTIONEDITOR_H
