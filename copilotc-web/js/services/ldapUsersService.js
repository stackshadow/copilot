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
var newService = {};
newService.id = "ldapusers";
newService.displayName = "LDAP User Management";
newService.topicListen = "co-ldap/";
newService.onAuth = ldapOnAuth;
newService.onConnect = null;
newService.onDisconnect = null;
newService.onMessage = null;

function ldapOnAuth(){

    htmlNavElement = document.getElementById( "ldapBtnConnect" );

    if( htmlNavElement === null || htmlNavElement === undefined ){
        // Navigation bar
        htmlNavElement = document.createElement('a');
        htmlNavElement.id = "ldapBtnConnect";
        htmlNavElement.innerHTML = "<span class=\"glyphicon glyphicon-user\"></span> LDAP Users";
        htmlNavElement.onclick = function(){ ldapShowOverview(); }
        navAppend( htmlNavElement );
    }


}

function ldapShowOverview(){

    htmlLoadFile( "output", "html/ldapUsers.html", function(){
        jsLoadFile( "js/services/ldapUsers.js" );
    });



}


wsServiceRegister( newService );
newService = null;
