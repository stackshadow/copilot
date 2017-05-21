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

#ifndef doDBDPlugin_H
#define doDBDPlugin_H


#include "string/etString.h"
#include "string/etStringChar.h"

#include "jansson.h"

#include "coMessage.h"
//class doDBDChannel;


class coPlugin {


protected:
    etString*           pluginName;
    etString*           pluginInfo;

public:


public:
    coPlugin( const char* name );
    ~coPlugin();

public:
    const char*         name();
    bool                info( const char *shortInfo );
    const char*         info();



	virtual bool		onBroadcastMessage( coMessage* message ){ return true; }
	virtual bool        onBroadcastReply( coMessage* message ){ return true; }



};














#endif // doDBDPlugin_H
