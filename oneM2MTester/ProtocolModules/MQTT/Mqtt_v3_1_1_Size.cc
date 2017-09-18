/******************************************************************************
* Copyright (c) 2017 Easy Global Market
* All rights reserved. This program and the accompanying materials
* are made available under the terms of the Eclipse Public License v1.0
* which accompanies this distribution, and is available at
* http://www.eclipse.org/legal/epl-v10.html
*
* Contributors:
*  Spaseski Naum - initial implementation
******************************************************************************/

#include "Mqtt_v3_1_1_IPL4SizeFunction.hh"


namespace Mqtt__v3__1__1__IPL4SizeFunction {

INTEGER f__calc__MQTT__length(const OCTETSTRING& data){
    
    int multiplier, value, i, j;
    i = 0; // encoded byte
    j = 2; // additional real size
    multiplier = 1;
    value = 0;
    do {
        i++;
        value += (oct2int(data[i]) & 127) * multiplier + j;
        multiplier *= 128;
        if (multiplier > 128*128*128){
            if(TTCN_Logger::log_this_event(LOG_ALL)){
                TTCN_Logger::begin_event(LOG_ALL);
                TTCN_Logger::log_event("Error: Wrong size of the message!");
                TTCN_Logger::end_event();
            }
            return -1; //error case, bigger than the MQTT limit
        }
	j = 1;
    }while((oct2int(data[i]) & 128) != 0);

  return value;
}

}
