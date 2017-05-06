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

#ifndef qwebsocket_H
#define qwebsocket_H

#include "string/etString.h"
#include "string/etStringChar.h"

#include "evillib-extra_depends.h"
#include "db/etDBObject.h"

#include "coPlugin.h"

#include "plugins/qwebsocketClient.h"
#include <QtWebSockets/QtWebSockets>
#include <QtCore/QList>


class qwebsocket : public QObject {
Q_OBJECT

private:
    //m_pWebSocketServer = new QWebSocketServer(
    QWebSocketServer*           wsServer;
    QList<websocketClient*>     clients;

public:
                                qwebsocket( int wsPort );
                                ~qwebsocket();

public slots:
    void                        onNewConnection();


};

#endif // websocket_H


