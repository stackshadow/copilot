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

// service
var coreService = {};
coreService.id = "auth";
coreService.displayName = "Login-Service";
coreService.listenGroup = "co";
coreService.onAuth = authOnAuth;
coreService.onConnect = authOnConnect;
coreService.onDisconnect = authOnDisconnect;
coreService.onMessage = authOnMessage;

// Navigation bar
htmlConnState = document.createElement('a');
htmlConnState.id = "btnAuthConnect";
htmlConnState.innerHTML = "<span class=\"glyphicon glyphicon-log-in\"></span> Login";
htmlConnState.onclick = function(){ authDialogShow(); }
navAppend( htmlConnState );

htmlLoadFile( "output", "html/authDialog.html" );



function authOnConnect(){



}
function authOnDisconnect(){

}
function authOnMessage( topicHostName, topicGroup, topicCommand, payload ){

// login was okay
    if( topicCommand == "loginok" ) authDialogLoginOk();

}
function authOnAuth( service ){

// hide the dialog
    $("#loginDialog").modal('hide');
    
// change the Login-Button to Logout
    var htmlBtnConnect =  document.getElementById( "btnAuthConnect" );
    htmlConnState.innerHTML = "<span class=\"glyphicon glyphicon-log-out\"></span> Logout";
    htmlConnState.onclick = function(){ authDialogLogout(); }

// send ping to connected services ( to get a list of connected hosts )
    copilotPing();

}


function authDialogShow(){
    $("#loginDialog").modal('show');
}

function authDialogLogin(){

    var htmlUsername = document.getElementById( "authUserName" ); htmlUsername = htmlUsername.value;
    var htmlPassword = document.getElementById( "authUserPassword" ); htmlPassword = htmlPassword.value;

    var jsonObject = {};
    jsonObject.user = htmlUsername;
    jsonObject.password = htmlPassword;

    wsMessageSend( null, coreService.listenGroup, "login", JSON.stringify(jsonObject) );

//JSON.stringify(jsonObject)
//JSON.parse(received_msg);

}

function authDialogLoginOk(){



// iterate
    for( serviceName in copilot.services ){

    // get the service
        service = copilot.services[serviceName];

        if( service.onAuth !== null ){
            service.onAuth( service );
        }

    }


}

function authDialogLogout(){
    wsDisconnect();
}




wsServiceRegister( coreService );














