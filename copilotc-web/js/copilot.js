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

var copilot = {};
copilot.ws = null;
copilot.wsConnected = false;
copilot.hostnames = {};
copilot.services = [];


/**
@brief Load java-script file and init
*/
function            jsLoadFile( fileName, initFunctionName ){

// already exists ?
    var script =  document.getElementById( "js_" + fileName );
    if( script !== null ){
        return;
    }

    var head = document.getElementsByTagName('head')[0];
    var script = document.createElement('script');
    script.type = 'text/javascript';
    script.src = fileName;
    script.id = "js_" + fileName;

// Then bind the event to the callback function.
// There are several events for cross browser compatibility.
//    script.onreadystatechange = callback;
//    script.onload = callback;

    // Fire the loading
    head.appendChild(script);

// run the script
/*
    if( typeof me.onChange === "function" ){

    }
*/
}
function            htmlLoadFile( id, fileName, finishLoadingFunction = null ){
// container

    htmlContainer = document.createElement('div');
    htmlContainer.id = id;
//    htmlContainer.innerHTML='<object type="text/html" data="' + fileName + '" ></object>';
    document.body.appendChild( htmlContainer );



/* dosn't work because of security
    htmlContainer = document.createElement('link');
    htmlContainer.id = id;
    htmlContainer.href=fileName;
    htmlContainer.rel = "import";
    document.body.appendChild( htmlContainer );
*/

    if( finishLoadingFunction !== null ){
        $( "#" + id ).load( fileName, finishLoadingFunction );
    } else {
        $( "#" + id ).load( fileName );
    }


}

// messages
function            messageCreate(){
// container
    htmlContainer = document.createElement('div');
    htmlContainer.className = "alert alert-info";
    htmlContainer.id = "message";
    document.body.appendChild( htmlContainer );

// icon
    htmlIcon = document.createElement('span');
    htmlIcon.className = "pficon pficon-info";
    htmlIcon.id = "messageIcon";
    htmlContainer.appendChild( htmlIcon );

// message
    htmlMessage = document.createElement('span');
    htmlMessage.id = "messageText";
    htmlMessage.innerHTML = "Ready";
    htmlContainer.appendChild( htmlMessage );

}
function            messageHide(){
    var htmlContainer =  document.getElementById( "message" );
    htmlContainer.style.display = 'none';
}
function            messageInfo( message, timeout = 0 ){

// container
    var htmlContainer =  document.getElementById( "message" );
    htmlContainer.className = "alert alert-info";

// icon
    var htmlIcon =  document.getElementById( "messageIcon" );
    htmlIcon.className = "glyphicon glyphicon-ok";

// message
    var htmlMessage =  document.getElementById( "messageText" );
    htmlMessage.innerHTML = " " + message;

// show and hide after a time
    htmlContainer.style.display = '';

    if( timeout > 0 ){
        setTimeout( messageHide, timeout * 1000 );
    }

}
function            messageAlert( message, timeout = 0 ){

// container
    var htmlContainer =  document.getElementById( "message" );
    htmlContainer.className = "alert alert-danger";

// icon
    var htmlIcon =  document.getElementById( "messageIcon" );
    htmlIcon.className = "glyphicon glyphicon-exclamation-sign";

// message
    var htmlMessage =  document.getElementById( "messageText" );
    htmlMessage.innerHTML = " " + message;

// show and hide after a time
    htmlContainer.style.display = '';

    if( timeout > 0 ){
        setTimeout( messageHide, timeout * 1000 );
    }

}
function            messageLog( section, message ){
    console.log(  section + ": " + message );
}

// the connection state
function            connStateCreate(){

//
    htmlConnState = document.createElement('div');
    htmlConnState.className = "label label-danger";
    htmlConnState.id = "connectState";
    htmlConnState.innerHTML = "...";

    htmlNavRight =  document.getElementById( "navigationRight" );
    htmlNavRight.appendChild( htmlConnState );

// core settings
    htmlNavElement = document.createElement( "div" );
    htmlNavElement.innerHTML = "<a href='#' onclick=\"copilotPing()\">Ping</a>";
    
    settingAppend( htmlNavElement );

}
function            connStateConnected(){
    var htmlConnState =  document.getElementById( "connectState" );
    htmlConnState.className = "label label-info";
    htmlConnState.innerHTML = "Verbunden";
}
function            connStateDisConnected(){
    var htmlConnState =  document.getElementById( "connectState" );
    htmlConnState.className = "label label-danger";
    htmlConnState.innerHTML = "Getrennt";
}


// the links
function            navAppend( htmlElement ){
    var htmlNav =  document.getElementById( "navigationLeft" );

    var htmlNavElement = document.createElement('li');
    htmlNavElement.appendChild( htmlElement );

    htmlNav.appendChild( htmlNavElement );
}
function            settingAppend( htmlElement ){
    var settingsDropDown =  document.getElementById( "settingsDropDown" );

    var htmlNavElement = document.createElement('li');
    htmlNavElement.appendChild( htmlElement );
    settingsDropDown.appendChild( htmlNavElement );
}


var wsTopicBase = "nodes/develop-arch/";


function            wsServiceRegister( service ){
    messageLog( "wsService", "try to register '" + service.displayName + "'" );

// check if functions are present or set to null
    if( service.id === null && service.id === undefined ){
        messageLog( "wsService", "ERROR: service dont has an id, dont register it !" );
        return;
    }

    messageLog( "wsService", "service '" + service.displayName + "' registered" );

    copilot.services[service.id] = service;
}

function            wsConnect(){

// already connected ?
    if( copilot.wsConnected == true ) return;
    
    if ("WebSocket" in window){

    // notificate the user
        messageInfo( "Verbinde...");

    // Let us open a web socket
        copilot.wsConnected = false;
        //copilot.ws = new WebSocket("ws://[::]:3000", "copilot" );
        copilot.ws = new WebSocket("ws://127.0.0.1:3000", "copilot" );
        console.log( copilot.ws );
        copilot.ws.onopen = wsOnOpen
        copilot.ws.onmessage = wsOnMessage
        copilot.ws.onclose = wsOnClose
    } else {
    // The browser doesn't support WebSocket
        messageAlert("WebSocket NOT supported by your Browser!");
    }

}
function            wsDisconnect(){
    
    copilot.ws.close();
    copilot.ws = null;
    copilot.wsConnected = false;
}
function            wsMessageSend( topicHostName, topicGroup, topicCommand, payload ){

// connected ?
    if( copilot.wsConnected == false ){
        messageLog( "websocket send", "Could not send, not connected" );
        return;
    }

//
    if( topicHostName === null || topicHostName === undefined ){
        topicHostName = copilot.myhostname
        //topicHostName = "develop-arch";
    }

// command
    var jsonCMD = {};
    jsonCMD["topic"] = "nodes/" + topicHostName + "/" + topicGroup + "/" + topicCommand;
    jsonCMD["payload"] = payload;

// command string
    commandString = JSON.stringify(jsonCMD);
    messageLog( "websocket send", commandString );

    copilot.ws.send( commandString );
}


function            wsOnOpen(){
    copilot.wsConnected = true;
    connStateConnected();
    messageInfo( "Verbunden", 4 );

// call onConnect on every service
    for( serviceName in copilot.services ){
        service = copilot.services[serviceName];
        if( service.onConnect !== null && service.onConnect !== undefined ){
            service.onConnect();
        }
    }

// we request the hostname ( the only function wich works without login )
    copilotGetHostName();

}
function            wsOnClose(){
// websocket is closed.
    connStateDisConnected();
    messageAlert( "Getrennt" );
    copilot.wsConnected = false;

    for( serviceName in copilot.services ){
        service = copilot.services[serviceName];
        if( service.onDisconnect !== null ){
            service.onDisconnect();
        }
    }
}
function            wsOnMessage( evt ){
    var received_msg = evt.data;

    jsonObject = JSON.parse(received_msg);

// command string
    commandString = JSON.stringify(jsonObject);
    messageLog( "websocket recieve", commandString );

// no topic
    if( jsonObject.topic === undefined ){
        return;
    }

// we need the hostname for some plugins
    var jsonTopicArray = jsonObject.topic.split("/");
    var topicHostName = jsonTopicArray[1];
    var topicGroup = jsonTopicArray[2];
    var topicCommand = jsonTopicArray[3];

// remove the base string from the topic to make it easyer
    jsonObject.topic = jsonObject.topic.replace(wsTopicBase,'');

// core commands
    if( jsonObject.topic.endsWith( "msgInfo" ) == true ){
        var message = jsonObject.payload;
        if( message === undefined ) return;
        messageInfo( message, 10 );
    }
    if( jsonObject.topic.endsWith( "msgError" ) == true ){
        var message = jsonObject.payload;
        if( message === undefined ) return;
        messageAlert( message );
    }

    if( topicGroup == "co" && topicCommand == "hostname" ){
        copilot.myhostname = topicHostName;
    }

    if( topicGroup == "co" && topicCommand == "pong" ){
        copilot.hostnames[topicHostName] = "";
    }



// send message to all services
    for( serviceName in copilot.services ){

    // get the service
        service = copilot.services[serviceName];

    // only call plugin if it listen on this group
        if( service.listenGroup === undefined ){
            continue;
        }
        if( topicGroup != service.listenGroup && service.listenGroup != "" ){
            continue;
        }

    // call the function
		if( service.onMessage !== null && service.onMessage !== undefined ){
			service.onMessage( topicHostName, topicGroup, topicCommand, jsonObject.payload );
		}


    }



};


function            copilotGetHostName(){
    wsMessageSend( "localhost", "co", "gethostname", JSON.stringify({}) );
}
function            copilotPing(){
    wsMessageSend( "all", "co", "ping", JSON.stringify({}) );
}



/*
<div class="alert alert-info" id="message">
    <span class="pficon pficon-info" id="messageIcon"></span>
    <span id="messageText">Ready</span>
</div>

	<div class="alert alert-success" role="alert" id="resultContainer" hidden="true">
		<span class="pficon pficon-info" id="resultIcon"></span>
		<span id="resultMessage">Ready</span>
	</div>

*/

messageCreate();
connStateCreate();

// we connect to the websocket
wsConnect();


