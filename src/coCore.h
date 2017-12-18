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
#include <pthread.h>
#include "semaphore.h"

#include "evillib_depends.h"
#include "memory/etList.h"
#include "string/etString.h"
#include "string/etStringChar.h"

#include "coCoreConfig.h"
#include "coMessage.h"
//#include "coPluginElement.h"
#include "coPlugin.h"
#include "coPluginList.h"





class coCore {


// public values
	private:
		pthread_t			threadLock;

        etString*           myNodeName;

		etString*			hostName;
		int                 hostNodeNameLen;

	// plugins
		json_t*             jsonAnswerArray;


	public:
							coCore();
							~coCore();
		static coCore*  	ptr;
		static bool			setupMode;
		static coMessage*	message;


	public:
		coCoreConfig*		config;
		coPluginList*		plugins;

	// set / get
        const char*         nodeName();
		void            	setHostName( const char* hostname );
		bool				hostNameGet( const char** hostName, int* hostNameChars );
		const char*			hostNameGet();
		bool				isHostName( const char* hostNameToCheck );


	// helper functions
		static bool  		jsonValue( json_t* jsonObject, const char* key, char* value, int valueMax, const char* defaultValue, bool toJson );
		static bool  		jsonValue( json_t* jsonObject, const char* key, std::string* value, const char* defaultValue, bool toJson );

	// login / auth / permissions
		static bool  		passwordCheck( const char* user, const char* pass );
		static bool			passwordChange( const char* user, const char* oldpw, const char* newpw );

	// the main loop
		void				mainLoop();

	private:


	// some helper stuff
		//static bool     	setTopic( coPlugin* pluginElement, json_t* jsonAnswerObject, const char* msgGroup );





};



#endif

