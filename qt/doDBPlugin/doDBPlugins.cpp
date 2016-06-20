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
void doDBPlugins::      eventPrepareToolBar( QLayout *layout ){

// vars
    doDBPlugin *plugin = NULL;

// iterate
    foreach( plugin, this->pluginList ){

    // run function
        plugin->prepareToolBar( layout );

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


void doDBPlugins::      eventPrepareItemView( QLayout *layout ){

// vars
    doDBPlugin *plugin = NULL;

// iterate
    foreach( plugin, this->pluginList ){

    // run function
        plugin->prepareItemView( layout );

    }

    return;
}






// all events
void doDBPlugins::      eventTreeItemClicked( doDBEntry* entry ){
// only if item is enabled
    if( entry->treeWidgetItemEnabled() == false ) return;

// vars
    doDBPlugin *plugin = NULL;

// iterate
    foreach( plugin, this->pluginList ){

    // run function
        if( plugin->dbTreeItemClicked( entry ) != true ){
            return;
        }

    }

    return;
}


void doDBPlugins::      eventTreeItemExpanded( doDBEntry* entry ){
// only if item is enabled
    if( entry->treeWidgetItemEnabled() == false ) return;

// vars
    doDBPlugin *plugin = NULL;

// iterate
    foreach( plugin, this->pluginList ){

    // run function
        if( plugin->dbTreeItemExpanded( entry ) != true ){
            return;
        }

    }

    return;
}


void doDBPlugins::      eventTreeItemCollapsed( doDBEntry* entry ){
// only if item is enabled
    if( entry->treeWidgetItemEnabled() == false ) return;

// vars
    doDBPlugin *plugin = NULL;

// iterate
    foreach( plugin, this->pluginList ){

    // run function
        if( plugin->dbTreeItemCollapsed( entry ) != true ){
            return;
        }

    }

    return;
}


void doDBPlugins::      eventItemChanged( const char * columnName, const char * newColumnValue ){

// vars
    doDBPlugin *plugin = NULL;

// iterate
    foreach( plugin, this->pluginList ){

    // run function
        if( plugin->itemChanged( columnName, newColumnValue ) != true ){
            return;
        }

    }

    return;
}




