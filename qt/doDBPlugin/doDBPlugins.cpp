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

doDBPlugins::           doDBPlugins(){
    this->ptr = this;

}

doDBPlugins::           ~doDBPlugins(){

}



void doDBPlugins::      append( doDBPlugin *dbPlugin ){
    this->pluginList.append( dbPlugin );
}




void doDBPlugins::      prepareLayout( QString name, QLayout* layout ){

// vars
    doDBPlugin *plugin = NULL;

// iterate
    foreach( plugin, this->pluginList ){

    // run function
        plugin->prepareLayout( name, layout );

    }

    return;
}










void doDBPlugins::      registerListener( doDBPlugin *plugin, doDBPlugin::messageID type, bool fireOnce ){

    messageListener* tempMessage = new messageListener;

    tempMessage->target = plugin;
    tempMessage->type = type;
    tempMessage->onlyOnce = fireOnce;

// we do NOT check if the listener is already present. This is for performance

// add it to the list
    this->messageList.append( tempMessage );

}


void doDBPlugins::      sendBroadcast( doDBPlugin::messageID type, void *payload ){

    messageListener*     listener;
    foreach( listener, this->messageList ){

        if( listener->type == type ){

            listener->target->recieveMessage( type, payload );

        // remove it from the list if only once is true
            if( listener->onlyOnce == true ){
                this->messageList.removeAll( listener );
            }


        }

    }


}




