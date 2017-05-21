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

#ifndef doCore_H
#define doCore_H

#include <unistd.h>
#include <sys/utsname.h>
#include <string>

#include "evillib_depends.h"
#include "memory/etList.h"
#include "string/etString.h"
#include "string/etStringChar.h"


#include "coMessage.h"
#include "coPluginElement.h"



#define baseFilePath /etc/copilot/services/

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define configFile(a) STR(baseFilePath) a




class coCore {

public:
    struct utsname      hostInfo;
    int                 hostNodeNameLen;

private:
    coPlugin*           authPlugin;             /**< The plugin which authenticate the user-login */
    json_t*             jsonAnswerArray;
    bool                locked;
    etList*             pluginList;
    etList*             pluginListEnd;
    void*               iterator;

// lock
	bool				broadcastBusy;
	coMessage*			broadcastMessage;

public:
                    coCore();
                    ~coCore();
    static coCore*  ptr;

public:

// set / get
    void            setHostName( const char *hostname );

// functions to work with the list
    bool            registerPlugin( coPlugin* plugin, const char *hostName, const char *group );
    bool            removePlugin( coPlugin* plugin );
    void            listPlugins( json_t* pluginNameArray );


// helper functions
    static bool     jsonValue( json_t* jsonObject, const char* key, char* value, int valueMax, const char* defaultValue, bool toJson );
    static bool     jsonValue( json_t* jsonObject, const char* key, std::string* value, const char* defaultValue, bool toJson );

// login / auth / permissions
    static bool     passwordCheck( const char* user, const char* pass );
    static bool     passwordChange( const char* user, const char* oldpw, const char* newpw );

// the main loop
    void            mainLoop();

private:
    bool            pluginsIterate();
    bool            pluginNext( coPluginElement** pluginElement );
    bool            pluginNext( coPlugin** plugin );
    bool            pluginElementGet( coPlugin* plugin, coPluginElement** pluginElement );
    bool            nextAviable();

// some helper stuff
    static bool     setTopic( coPluginElement* pluginElement, json_t* jsonAnswerObject, const char* msgGroup );


public:
    void            broadcast(
                        coPlugin*       source,
                        const char*     msgHostName,
                        const char*     msgGroup,
                        const char*     msgCommand,
                        const char*     msgPayload
                    );






};



#endif

