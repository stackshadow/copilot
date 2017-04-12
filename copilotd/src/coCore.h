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


#include "coPluginElement.h"


class coCore {

public:
    struct utsname  hostInfo;
    int             hostNodeNameLen;

private:
    json_t*         jsonAnswerArray;

public:
                    coCore();
                    ~coCore();
    static coCore*  ptr;

public:

// functions to work with the list
    bool            registerPlugin( coPlugin* plugin, const char *hostName, const char *group );
    bool            removePlugin( coPlugin* plugin );
    void            listPlugins( json_t* pluginNameArray );

private:
    void            iterate();
    bool            next( coPluginElement** pluginElement );
    bool            next( coPlugin** plugin );
    bool            nextAviable();

// some helper stuff
    static bool     setTopic( coPluginElement* pluginElement, json_t* jsonAnswerObject );

public:
    void            broadcast( coPlugin *source, const char* msgHostName, const char* msgGroup, const char* msgCommand, json_t* jsonData );


private:
    bool            locked;
    etList*         start;
    etList*         end;
    void*           iterator;


};



#endif

