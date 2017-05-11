
// service
var copliotdService = {};
copliotdService.id = "copliotd";
copliotdService.displayName = "Copliot";
copliotdService.listenGroup = "";
copliotdService.onAuth = copliotdOnAuth;
copliotdService.onConnect = null;
copliotdService.onDisconnect = null;
copliotdService.onMessage = null;
// local



function copliotdOnAuth(){

    htmlNavElement = document.getElementById( "copliotdBtnMenu" );

    if( htmlNavElement === null || htmlNavElement === undefined ){
        // Navigation bar
        htmlNavElement = document.createElement('a');
        htmlNavElement.id = "copliotdBtnMenu";
        htmlNavElement.innerHTML = "<span class=\"glyphicon glyphicon-fire\"></span> Copliot";
        htmlNavElement.onclick = function(){ copilotdShow(); }
        navAppend( htmlNavElement );
    }

	return;
}


function copilotdShow(){
    var service = copilot.services["copliotd"];


    htmlCopilotd = document.getElementById( "copilotd" );
    if( htmlCopilotd === null || htmlCopilotd === undefined ){
    // load
        htmlLoadFile( "output", "html/copilotd.html", function(){
            //jsLoadFile( "js/services/nft.js" );

		// fill table
			copilotdHostTableLoad();
			
		// request infos about mqtt-server
			wsMessageSend( null, "mqtt", "getinfos", "" );
			
			

		// set message
			service.onMessage = copilotdOnMessage;

        });
    }

	return;
}



function copilotdPing(){
    var service = copilot.services["copliotd"];
	copilotdHostTableClear();
	copilotPing();
	
	return;
}


function copilotdHostTableClear(){
	htmlTableBody =  document.getElementById( "copilotd_hosts_values" );
	htmlTableBody.innerHTML = "";
	
	return;
}


function copilotdHostTableAppend( htmlTableBody, hostname, version ){

// row
	newRow = document.createElement('tr');
	//newRow.id = "nftRule_" + chainName + "_" + ruleIndex;
	htmlTableBody.appendChild( newRow );

// name
	newHostname = document.createElement('td');
	newHostname.innerHTML  = hostname;
	newRow.appendChild( newHostname );

// version
	newVersion = document.createElement('td');
	newVersion.id = "nodes/" + hostname + "/co/pong";
	newRow.appendChild( newVersion );

// column
	newAction = document.createElement('td');
	newRow.appendChild( newAction );

	return;
}


function copilotdHostTableLoad(){
    var service = copilot.services["copliotd"];


	htmlTableBody =  document.getElementById( "copilotd_hosts_values" );
	htmlTableBody.innerHTML = "";

	for( hostName in copilot.hostnames ){
		displayName = hostName;
		copilotdHostTableAppend( htmlTableBody, displayName, "" );
	}

	return;
}




function copilotdOnMessage( topicHostName, topicGroup, topicCommand, payload ){

	if( topicCommand == "pong" ){
		htmlTableBody =  document.getElementById( "copilotd_hosts_values" );
		copilotdHostTableAppend( htmlTableBody, topicHostName, "" );
		
		return;
	}
	
	if( topicCommand == "clientCount" ){
		htmlTableBody =  document.getElementById( "mqttClientCount" );
		htmlTableBody.innerHTML = payload;
		return;
	}
	
	if( topicCommand == "infos" ){
		// payload is a json-object
		//jsonPayload = JSON.parse( payload );

		htmlConnected =  document.getElementById( "mqttClientState" );
		if( payload.connected == 1 ){
			htmlConnected.className = "label label-success";
			htmlConnected.innerHTML = "Connected";
		} else {
			htmlConnected.className = "label label-danger";
			htmlConnected.innerHTML = "Disconnected";
		}
		
		htmlClientCount =  document.getElementById( "mqttClientCount" );
		htmlClientCount.innerHTML = payload.clients;
		

	}

	return;
}


wsServiceRegister( copliotdService );
