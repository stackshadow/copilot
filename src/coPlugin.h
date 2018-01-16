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

/**
@defgroup coPlugin coPlugin - A single Plugin
*/

#ifndef doDBDPlugin_H
#define doDBDPlugin_H


#include "string/etString.h"
#include "string/etStringChar.h"

#include "jansson.h"

#include "coMessage.h"
//class doDBDChannel;


class coPlugin {


public:
	typedef enum e_state {
		NO_REPLY = -1,
		REPLY = 0,
		BREAK = 1,
        MESSAGE_FINISHED,       ///< Finished with message, Messagehandler can delete this message
        MESSAGE_UNKNOWN,        ///< Plugin Dont know what to do with this message
	} t_state;


protected:
    etString*           pluginName;
    etString*           pluginInfo;
private:
	etString*			targetNode;
	etString*			targetGroup;

public:


public:
    coPlugin( const char* name, const char* onlyTargetNodeName, const char* listenGroup );
    ~coPlugin();

public:
    const char*         name();
    bool                info( const char* shortInfo );
    const char*         info();
	bool				filterCheck( const char* hostName, const char* group );


// API
	virtual t_state		onBroadcastMessage( coMessage* message ){ return NO_REPLY; }
	virtual bool 		onSetup(){ return true; }
	virtual bool		onExecute(){ return true; }

protected:
	void				setName( const char* name );


};














#endif // doDBDPlugin_H
