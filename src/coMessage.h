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

#ifndef broadCastMessage_H
#define broadCastMessage_H

#include "string/etString.h"
#include "string/etStringChar.h"

#include "jansson.h"


class coMessage {

public:
	etString*		hostName_t;
	etString*		group_t;
	etString*		command_t;
	etString*		payload_t;

	etString*		replyCommand_t;
	etString*		replyPayload_t;

	etString*		temp;

public:
	coMessage();
	~coMessage();

// base
	const char*		hostName( const char* newHostName = NULL ){ return this->setValue(this->hostName_t, newHostName); };
	const char*		group( const char* newGroup = NULL ){ return this->setValue(this->group_t, newGroup); };
	const char*		command( const char* newCommand = NULL ){ return this->setValue(this->command_t, newCommand); };
	const char*		payload( const char* newPayload = NULL ){ return this->setValue(this->payload_t, newPayload); };

	const char*		topic();

// reply
	const char*		replyCommand( const char* newReplyCommand = NULL ){ return this->setValue(this->replyCommand_t, newReplyCommand); };
	const char*		replyPayload( const char* newReplyPayload = NULL ){ return this->setValue(this->replyPayload_t, newReplyPayload); };

	bool			replyExists();
	const char*		replyComandFull();

	bool			clear();
	bool			clearReply();

private:
	const char*		setValue( etString* string, const char* newValue );

};





#endif
