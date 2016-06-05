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

#ifndef doDBFile_H
#define doDBFile_H


#include <QTreeWidget>
#include <QLayout>
#include <QPushButton>

#include "doDBTree/doDBTree.h"
#include "jansson.h"
#include "doDBFile/doDBTableFoldersEditor.h"

class doDBFile :
public QObject {
    Q_OBJECT

public:
    doDBFile();
    ~doDBFile();

    void                    doDBTreeInit( doDBtree *dbTree );
    void                    doDBItemViewInit( QLayout *itemView );


// db stuff
private:
    bool                    loadFromDB();
    bool                    saveToDB();
    bool                    appendFolderToTable( const char *tableName, const char *os, const char *folder );
    bool                    getFolderFromTable( const char *tableName, const char **folderName );

// item infos
public:
    QString                 doDBTreeItemFolderNameGet( QTreeWidgetItem *treeItem );
// stuff to do with the dbTree
private:
    QString                 doDBTreeFullPathGet( QTreeWidgetItem *treeItem );
    bool                    doDBTreeSetPathOfExistingItems( QTreeWidgetItem *treeItem, const char *folderName );
    void                    doDBTreeAppendFolder( QTreeWidgetItem *parentItem, const char *folderName );
    void                    doDBTreeAppendFile( QTreeWidgetItem *parentItem, const char *fileName );
private slots:
    void                    doDBTreeExpand( QTreeWidgetItem * item );
    void                    doDBTreeCollapsed( QTreeWidgetItem * item );
    void                    doDBTreeClicked( QTreeWidgetItem * item, int column );

// editor stuff
private slots:
    void                    tableFoldersEditorShow();


private:
    QPushButton             *btnTableFolderEdit;
    doDBTableFoldersEditor  *tableFolderEditor;

    doDBtree                *dbTree;
    int                     dbTreeItemTypeFolder;
    int                     dbTreeItemTypeFile;
    int                     dbTreeItemColumnFolder;

    QString                 connectionID;
    QString                 tableName;
    json_t                  *actualJSON;



};






#endif
