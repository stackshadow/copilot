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



class coreService : public coPlugin
{

public:
                                coreService();
                                ~coreService();

private:

// overloaded functions
public:
// get data
    bool                        onMessage(  const char*     msgHostName, 
                                            const char*     msgGroup, 
                                            const char*     msgCommand, 
                                            const char*     msgPayload, 
                                            json_t*         jsonAnswerObject );


};












#endif // DODB_H


