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

#ifndef websocketClient_H
#define websocketClient_H


#include "evillib_depends.h"
#include "memory/etList.h"
#include "string/etString.h"
#include "string/etStringChar.h"


#include "coPlugin.h"
#include <QtWebSockets/QtWebSockets>

class websocketClient : public QObject, public coPlugin {
Q_OBJECT

private:
    QWebSocket*     remoteSocket;
    quint16         remotePort;

// authentication
    bool            authenticated;
    etString*       username;

public:
                    websocketClient( QWebSocket* remoteSocket );
                    ~websocketClient();
    bool            onBroadcastReply( json_t* jsonAnswerArray );

public slots:
    void            onTextMessage( QString message );
    void            onDisconnected();


private:
    void            doAuth( const char* username, const char* password );

// callbacks
public:
    bool            onBroadcastMessage(     const char*     msgHostName, 
                                            const char*     msgGroup, 
                                            const char*     msgCommand, 
                                            const char*     msgPayload, 
                                            json_t*         jsonAnswerObject );

};




#endif




