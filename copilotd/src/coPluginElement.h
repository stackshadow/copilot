/*
Copyright (C) 2017 by Martin Langlotz

This file is part of copilot.

copilot is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, version 3 of this License

copilot is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with copilot.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef coCorePluginElement_H
#define coCorePluginElement_H

/** @defgroup coPluginElement coPluginElement - The Element inside the plugin list

The coPluginElement represent an plugin in the core list. 
Purpose of this class is to ensure that the plugin itselfe can not manipulate the internal infos, like authentification, access right etc.

*/


#include "coPlugin.h"

class coPluginElement {

public:
    enum {
        UNSET = -1,
        NOT_ALLOWED = 0,
        ALLOWED = 1
    };
    
public:
    etString*       listenHostName;         /**< the hostname we listen for or "" if we accept every target */
    etString*       listenGroup;            /**< the group we listen for or "" if we accept every target */
    coPlugin*       plugin;                 /**< The plugin itselfe */

public:
                    coPluginElement();
                    ~coPluginElement();

};




#endif // ldapService_C
