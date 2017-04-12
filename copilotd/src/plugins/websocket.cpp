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

#ifndef websocket_C
#define websocket_C

#include "coCore.h"
#include "plugins/websocket.h"



websocket::                     websocket( int wsPort ) : QObject() {

    this->wsServer = new QWebSocketServer( QString("localhost"), QWebSocketServer::NonSecureMode, NULL );
     if( this->wsServer->listen( QHostAddress::Any, wsPort ) ){
        qDebug() << "Chat Server listening on port" << wsPort;

        QObject::connect(   this->wsServer, &QWebSocketServer::newConnection,
                            this, &websocket::onNewConnection );


/*
connect(pSocket, &QWebSocket::textMessageReceived, this, &EchoServer::processTextMessage);
connect(pSocket, &QWebSocket::binaryMessageReceived, this, &EchoServer::processBinaryMessage);
connect(pSocket, &QWebSocket::disconnected, this, &EchoServer::socketDisconnected);
*/


    }

}

websocket::                     ~websocket(){
// vars
    websocketClient *wsClient = NULL;

// iterate clients
    for( int i = 0; i < this->clients.size(); ++i ){
        wsClient = this->clients.at(i);
        delete wsClient;
    }

    delete this->wsServer;
}




void websocket::                onNewConnection(){

    QWebSocket *pSocket = this->wsServer->nextPendingConnection();


// create a new client
    websocketClient *wsClient = new websocketClient( pSocket );
    this->clients.append( wsClient );

}



#endif // websocket_C
