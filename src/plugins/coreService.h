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

#ifndef authService_H
#define authService_H



#include "string/etString.h"
#include "string/etStringChar.h"

#include "evillib-extra_depends.h"
#include "db/etDBObject.h"

//#include "coPlugin.h"


/**
@cond capi
@defgroup capi-co co - Core messages
@ingroup capi

target node | group | command | command example |   | respond command | respond example
----------- | ----- | ------- | --------------- | - |---------------- | ----
 <specific> | co    | ping    | dummy/co/ping   |   | pong            |  dummy/co/pong
 "all"      | co    | ping    | all/co/ping     |   | pong            |  dummy/co/pong

| Send a ping to an specific node to check if the node is present
 * | Send a ping to all nodes, and all nodes will respond to it
# answer
Schema: {myhostname}/co/pong \n
Payload: empty


@defgroup capi-co-versionget {node}/co/versionGet - Get the copilot version of an specific node
@ingroup capi-co

{node} must be an known node, you can not use "all"

# answer
Schema: {node}/co/version \n
Payload: The version as an char-array

# example
client send: mylaptop/co/versionGet \n
client recieve: mylaptop/co/versionGet payload: 0.1



@defgroup capi-co-nodesget {node}/co/nodesGet - Get nodes
@ingroup capi-co

{node} must be an known node, you can not use "all"

# answer
Schema: {node}/co/nodes \n
Payload: an json-object

## json node description ( example )

@code{.json}
{
    "dummyNode": {
        "host": "dummyNode",
        "port": 4567,
        "type": 1
    }
}
@endcode


# example
client send: mylaptop/co/versionGet \n
client recieve: mylaptop/co/versionGet payload: 0.1


@endcond
*/


class coreService
{


	public:
                                coreService();
                                ~coreService();




private:
//#ifndef MQTT_ONLY_LOCAL
	void						appendKnownNodes( const char* nodeName );
//#endif

public:
// handlers

	static int 					onSubscriberMessage( void* objectInstance, const char* id, const char* nodeSource, const char* nodeTarget, const char* group, const char* command, const char* payload, void* userdata );


};












#endif // DODB_H


