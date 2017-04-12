

newService = copilot.services["ldapusers"];
newService.onConnect = ldapOnConnect;
newService.onDisconnect = ldapOnDisconnect;
newService.onMessage = ldapOnMessage;

function ldapOnConnect(){

}
function ldapOnDisconnect(){

}
function ldapOnMessage( topic, payload ){

// login was okay
    if( topic == newService.topicListen + "loginok" ) ldapConnected();
    if( topic == newService.topicListen + "data" ) ldapConnectionEditShow( payload );
    if( topic == newService.topicListen + "saved" ) ldapConnectionEditSaved();

}




function ldapConnectionEdit(){
// create json
    var jsonObject = {};
    jsonObject.id = "default";

// send it out
    wsMessageSend( newService.topicListen + "get", JSON.stringify(jsonObject) );

// simulation
    var jsonObject = {};
    jsonObject.id = "default";
    jsonObject.user = "testuser";
    jsonObject.pw = "testpass";
    ldapOnMessage( newService.topicListen + "data", jsonObject );
}

function ldapConnectionEditShow( jsonObject ){

    var htmlUsername = document.getElementById( "ldapUserName" ); htmlUsername.value = jsonObject.user;
    var htmlPassword = document.getElementById( "ldapPassword" ); htmlPassword.value = jsonObject.pw;
    $("#ldapConnectDialog").modal('show');

}

function ldapConnectionEditSave(){

    var htmlUsername = document.getElementById( "ldapUserName" ); htmlUsername = htmlUsername.value;
    var htmlPassword = document.getElementById( "ldapPassword" ); htmlPassword = htmlPassword.value;

    var jsonObject = {};
    jsonObject.id = "default";
    jsonObject.user = htmlUsername;
    jsonObject.pw = htmlPassword;

    ldapOnMessage( newService.topicListen + "save", jsonObject );
}

function ldapConnectionEditSaved(){
    $("#ldapConnectDialog").modal('hide');
}




function ldapConnect(){

// create json
    var jsonObject = {};
    jsonObject.id = "default";

// send it out
    wsMessageSend( newService.topicListen + "conn/login", JSON.stringify(jsonObject) );


    return;


// get elements from form
    var htmlUsername = document.getElementById( "authUserName" ); htmlUsername = htmlUsername.value;
    var htmlPassword = document.getElementById( "authUserPassword" ); htmlPassword = htmlPassword.value;

// create json
    var jsonObject = {};
    jsonObject.user = htmlUsername;
    jsonObject.password = htmlPassword;

// send it out
    wsMessageSend( newService.topicListen + "login", JSON.stringify(jsonObject) );
}
function ldapConnected(){
// change the Login-Button to Logout
    var htmlBtnConnect =  document.getElementById( "ldapConnectBtn" );
    htmlConnState.innerHTML = "<span class=\"glyphicon glyphicon-log-out\"></span> Trennen";
    htmlConnState.onclick = function(){ ldapDisConnect(); }
}
function ldapDisConnect(){

}







//    glyphicon glyphicon-log-out
