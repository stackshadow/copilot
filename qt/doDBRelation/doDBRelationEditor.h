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

#ifndef doDBRelationEditor_H
#define doDBRelationEditor_H

#include <QObject>
#include <QWidget>
#include <QMainWindow>
#include <QComboBox>
#include "doDBRelation/doDBRelationEditor.ui.h"

#include "doDBRelation/doDBRelation.h"
#include "doDBConnection/doDBConnection.h"

#include "core/etIDState.h"
#include "evillib-extra_depends.h"
#include "db/etDBObject.h"
#include "db/etDBObjectTable.h"


class doDBRelationEditor : public QWidget
{

Q_OBJECT


public:
                                    doDBRelationEditor( QWidget *parent );
                                    ~doDBRelationEditor();

    void                            connectionCBoxRefresh();


    void                            showRelation( etDBObject *dbObject, doDBRelation *dbRelation );

private:
    void                            fillTableWithColumns( const char *tableName, QTableWidget *tableWidget );
    void                            refreshRelation();


private slots:
    void                            srcTableSelected( int selectedItem );
    void                            relTableSelected( int selectedItem );
    void                            relationAppend();
    void                            relationRemove();
    void                            closeEditor();

signals:
    void                            closed();

private:
    Ui::doDBRelationEditor          ui;
    etDBObject                      *dbObject;
    doDBRelation                    *dbRelation;

};


#endif
