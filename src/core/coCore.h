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

#include <getopt.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <string>
#include <pthread.h>
#include "semaphore.h"

#include "evillib_depends.h"
#include "memory/etList.h"
#include "string/etString.h"
#include "string/etStringChar.h"

#include "coConfig.h"
#include "coMessage.h"
//#include "coPluginElement.h"
//#include "coPlugin.h"
//#include "coPluginList.h"





class coCore {


// public values
    private:
        pthread_t           threadLock;

        etString*           thisNodeName = NULL;
        etString*           configBasePath = NULL;

        etString*           hostName = NULL;
        int                 hostNodeNameLen;

    // plugins
        json_t*             jsonAnswerArray;



    public:
                            coCore();
                            ~coCore();
        static coCore*      ptr;

        struct option*      options = NULL;
        char**              optionDescriptions = NULL;
        unsigned int        optionsLen = 0;
        static bool         setupMode;
        static coMessage*   message;


    public:
        //coConfig*     config;
        //coPluginList*     plugins;

    // options
        static bool         addOption( const char* optionLong, const char* optionShort, const char* description, int has_arg );
        static bool         isOption( const char* optionLong, unsigned int optNum );
        static void         dumpOptions();
        int                 parseOpt( const char* option, const char* value );

    // set / get
        const char*         configPath( const char* path = NULL );
        const char*         nodeName( const char* newNodeName = NULL );
        void                setHostName( const char* hostname );
        bool                hostNameGet( const char** hostName, int* hostNameChars );
        const char*         hostNameGet();
        bool                isHostName( const char* hostNameToCheck );
        bool                isNodeName( const char* nodeNameToCheck );



    // helper functions
        static bool         strIsExact( const char* str1, const char* str2, int str2Len );
        static int          jsonValue( json_t* jsonObject, const char* key, char* value, int valueMax, const char* defaultValue, bool toJson );
        static int          jsonValue( json_t* jsonObject, const char* key, std::string* value, const char* defaultValue, bool toJson );

    // login / auth / permissions
        static bool         passwordCheck( const char* user, const char* pass );
        static bool         passwordChange( const char* user, const char* oldpw, const char* newpw );

    // the main loop
        void                mainLoop();



    private:


    // some helper stuff
        //static bool     	setTopic( coPlugin* pluginElement, json_t* jsonAnswerObject, const char* msgGroup );





};



#endif

