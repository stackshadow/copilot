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

#ifndef doDBTableFoldersEditor_H
#define doDBTableFoldersEditor_H

#include <QObject>
#include <QWidget>
#include <QMainWindow>
#include <QComboBox>
#include "doDBFile/doDBTableFoldersEditor.ui.h"


#include "doDBConnection/doDBConnection.h"

#include "core/etIDState.h"
#include "evillib-extra_depends.h"
#include "db/etDBObject.h"
#include "db/etDBObjectTable.h"


class doDBTableFoldersEditor : public QWidget
{

Q_OBJECT

public:
    enum state {
        MODE_NONE,
        MODE_NEW,
        MODE_CHANGE
    };

public:
                                    doDBTableFoldersEditor( QWidget *parent );
                                    ~doDBTableFoldersEditor();

    void                            loadJson( json_t *tableFolders, const char *table );

private:
    void                            refreshTable();

private slots:
    void                            appendFolder();


signals:
    void                            saveJson();

private:
    Ui::doDBTableFoldersEditor      ui;
    QString                         tableName;
    json_t                          *json;

};

#endif // DODBTABLEEDITORWINDOW_H
