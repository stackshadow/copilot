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

#ifndef coCorePluginElement_H
#define coCorePluginElement_H

#include "coPlugin.h"

class coPluginElement {

public:
    etString*       listenHostName;     // the hostname we listen for or "" if we accept every target
    etString*       listenGroup;
    coPlugin*       plugin;


public:
                    coPluginElement();
                    ~coPluginElement();

};




#endif // ldapService_C
