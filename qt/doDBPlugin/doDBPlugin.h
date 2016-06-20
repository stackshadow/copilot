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

#ifndef doDBPlugin_H
#define doDBPlugin_H

#include "qt/doDBTree/doDBTree.h"
#include "doDBEntry/doDBEntry.h"

class doDBPlugin : public QObject {
    Q_OBJECT

    public:
        doDBPlugin();
        virtual ~doDBPlugin();

// prepare stuff
        virtual void prepareToolBar( QLayout *layout ){ return; }
        virtual void prepareTree( doDBtree *dbTree ){ return; }
        virtual void prepareItemView( QLayout *layout ){ return; }

// events
        virtual bool dbTreeItemClicked( doDBEntry* entry ){ return true; }
        virtual bool dbTreeItemExpanded( doDBEntry* entry ){ return true; }
        virtual bool dbTreeItemCollapsed( doDBEntry* entry ){ return true; }

        virtual bool itemChanged( const char * columnName, const char * newColumnValue ){ return true; }



    protected:
    private:
};

#endif // doDBPlugin_H
