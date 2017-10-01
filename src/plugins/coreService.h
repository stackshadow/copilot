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

#include "coPlugin.h"


/**
@cond capi
@defgroup capi-co co - Core messages
@ingroup capi


@defgroup capi-co-ping {node}/co/ping - Send a ping
@ingroup capi-co

With this command you can ping a single node or all nodes

# command
Schema: {node}/co/ping
{node} could be a specific node or "all"
Esample message:
 - all/co/ping
 - localhost/co/ping
 - somehost/co/ping

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


class coreService : public coPlugin
{


	public:
                                coreService();
                                ~coreService();




private:
#ifndef MQTT_ONLY_LOCAL
	void						appendKnownNodes( const char* hostName );
#endif

public:
// handlers
	coPlugin::t_state 			onBroadcastMessage( coMessage* message );



};












#endif // DODB_H


