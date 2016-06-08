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


#include "doDBPlugin/doDBPlugins.h"
#include "doDBPlugin/doDBPlugin.h"


doDBPlugins* doDBPlugins::ptr = NULL;

doDBPlugins::doDBPlugins(){
    this->ptr = this;

}

doDBPlugins::~doDBPlugins(){

}



void doDBPlugins::      append( doDBPlugin *dbPlugin ){
    this->pluginList.append( dbPlugin );
}



// prepare stuff
void doDBPlugins::      eventPrepareDashboard( QLayout *dashboardLayout ){

// vars
    doDBPlugin *plugin = NULL;

// iterate
    foreach( plugin, this->pluginList ){

    // run function
        plugin->prepareDashboard( dashboardLayout );

    }

    return;
}


void doDBPlugins::      eventPrepareTree( doDBtree *dbTree ){

// vars
    doDBPlugin *plugin = NULL;

// iterate
    foreach( plugin, this->pluginList ){

    // run function
        plugin->prepareTree( dbTree );

    }

    return;
}


void doDBPlugins::      eventPrepareItemView( QLayout *itemViewLayout ){

// vars
    doDBPlugin *plugin = NULL;

// iterate
    foreach( plugin, this->pluginList ){

    // run function
        plugin->prepareItemView( itemViewLayout );

    }

    return;
}






// all events
void doDBPlugins::      eventTreeItemClicked( QTreeWidgetItem * item, int column ){

// vars
    doDBPlugin *plugin = NULL;

// iterate
    foreach( plugin, this->pluginList ){

    // run function
        if( plugin->dbTreeItemClicked( item, column ) != true ){
            return;
        }

    }

    return;
}


void doDBPlugins::      eventTreeItemExpanded( QTreeWidgetItem * item ){
// vars
    doDBPlugin *plugin = NULL;

// iterate
    foreach( plugin, this->pluginList ){

    // run function
        if( plugin->dbTreeItemExpanded( item ) != true ){
            return;
        }

    }

    return;
}


void doDBPlugins::      eventTreeItemCollapsed( QTreeWidgetItem * item ){
// vars
    doDBPlugin *plugin = NULL;

// iterate
    foreach( plugin, this->pluginList ){

    // run function
        if( plugin->dbTreeItemCollapsed( item ) != true ){
            return;
        }

    }

    return;
}




