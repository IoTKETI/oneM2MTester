/****************************************************************************************
* Copyright (c) 2017  Korea Electronics Technology Institute.				            *
* All rights reserved. This program and the accompanying materials			            *
* are made available under the terms of                                         	    *
* - Eclipse Public License v1.0(http://www.eclipse.org/legal/epl-v10.html),     	    *
* - BSD-3 Clause Licence(http://www.iotocean.org/license/),                    		    *
* - MIT License   (https://github.com/open-source-parsers/jsoncpp/blob/master/LICENSE), * 
* - zlib License  (https://github.com/leethomason/tinyxml2#license).                    *	
*											                                            *
* Contributors:										                                    *
*   JaeYoung Hwang   - forest62590@gmail.com                                   	        *
*   Nak-Myoung Sung                                                            	        *
*   Ting Martin MIAO - initial implementation                                           *
* Updated:  2017-09-01                                                          	    *
*****************************************************************************************/
//  File:               OneM2MTimeStamp_GEN.cc
//  Description:        Generate oneM2M timestamp in format YYYYMMDDTHHMMSS based on current time   
//  Rev:                R2I

#include "OneM2M_Functions.hh"
#include <time.h>
#include <string>

namespace OneM2M__Functions {
    
     CHARSTRING fx__generateTimestamp(){
        std::string str_time = "";
        time_t now = time(0);
        struct tm tstruct;
        char buf[80];   

        tstruct = *localtime(&now);
        strftime(buf, sizeof(buf), "%Y%m%dT%I%M%S", &tstruct);   
        str_time = buf;

        CHARSTRING time_stamp(str_time.c_str());

        return time_stamp;
    }
}
