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
*   Nak-Myoung Sung  - nmsung@keti.re.kr                                       	        *
*   Updated:  2018-03-25                                                          	    *
****************************************************************************************/

#include <string>
#include "json.h"
#include "json-forwards.h"
#include "External_function.hh"
#include "OneM2M_DualFaceMapping.hh"

using namespace tinyxml2;
using namespace Json;
using namespace std;

namespace OneM2M__DualFaceMapping {
	CHARSTRING noti_JSON_Dec_Parser(const CHARSTRING& source_str, const CHARSTRING& serial_type){

		CHARSTRING SERIALIZATION_JSON = "json";
		CHARSTRING encoded_message;
		Reader jsonReader;

		Value jsonRoot(objectValue);
		Value resourceRoot(objectValue);
		Value subElemObj(objectValue);

		if(!initial_mapping()){
			TTCN_Logger::log(TTCN_DEBUG, "\n[WARNING]oneM2M long&short mapping initialization failed!!\n\n");
		}

		const char* p_body = (const char*)source_str;

		if("" == source_str){
			return "";
		}

		// 1. Save the root name "m2m:sgn -> notification (changed)"
		CHARSTRING rootName = "notification";

		if(SERIALIZATION_JSON == serial_type){

			std::string name_long;

			bool parsingSuccessful = jsonReader.parse(p_body, jsonRoot, false);

			if ( !parsingSuccessful ) {
				TTCN_Logger::log(TTCN_DEBUG, "JsonCPP API parsing error!");
				return "JsonCPP API parsing error!";
			}

			TTCN_Logger::log(TTCN_DEBUG, "[Decoding] Read JSON document for decoding:\n%s", jsonRoot.toStyledString().c_str());


			// 2. extract the sub element
			jsonRoot = jsonRoot.get("m2m:sgn", "");

			// 3. Checking the notification attributes
			for (Value::iterator iter = jsonRoot.begin(); iter != jsonRoot.end(); ++iter) {

				Value elemName;
				Value elemObj(objectValue);

				// 4. Select the attribute with KEY
				elemName = iter.key();
				elemObj = jsonRoot.get(elemName.asString(), "");

				// 5. Change the oneM2M short name to long name for the TTCN-3
				name_long = getLongName(elemName.asString());
				if(name_long == ""){
					name_long = elemName.asString();
				}

				if(elemObj.isObject()) { // object
					subElemObj = noti_JSON_Dec_Parser_Deep(elemObj, subElemObj, elemName);
				} else if (elemObj.isString()) { // string
					subElemObj[name_long.c_str()] = elemObj;
				} else if (elemObj.isInt()) { // integer
					std::string tmp_str((const char*)(int2str(elemObj.asInt())));
					std::string attr_val = getLongName(tmp_str);
					subElemObj[name_long.c_str()] = attr_val;
				}  else if (elemObj.isBool()){ // boolean
					subElemObj[name_long.c_str()] = elemObj;
				}
			}
		}

		resourceRoot[rootName] = subElemObj;

		TTCN_Logger::log(TTCN_DEBUG, "Pretty print of DECODED JSON message:\n%s", resourceRoot.toStyledString().c_str());

		StyledWriter writer;
		std::string json_str = writer.write(resourceRoot);

		CHARSTRING temp_cs(json_str.c_str());
		encoded_message	= temp_cs;

		return encoded_message;
	}

	Json::Value noti_JSON_Dec_Parser_Deep (Json::Value objectSource, Json::Value objectRoot, Json::Value elemName) {

		// Constant variables for the resource and attributes name
		static const std::string SUBSCRIPTION("subscription"), RESOURCE("resource"), EVENT_NOTIFICATION_CRITERIA("eventNotificationCriteria"),
							     OPERATION_MONITOR_LIST("operationMonitor_list"), EXPIRATION_COUNTER("expirationCounter");

		std::string name_long;
		std::string rootName;

		Value subElemObj(objectValue);
		Value containerForSubElem(objectValue);

		for (Value::iterator iter = objectSource.begin(); iter != objectSource.end(); ++iter) {

			Value elemName;
			Value elemObj(objectValue);

			// 1. Select the attribute with KEY
			elemName = iter.key();
			elemObj = objectSource.get(elemName.asString(), "");

			// 2. Change the oneM2M short name to long name for the TTCN-3
			name_long = getLongName(elemName.asString());
			if(name_long == ""){
				name_long = elemName.asString();
			}

			if(elemObj.isObject()) { // object
				containerForSubElem = noti_JSON_Dec_Parser_Deep(elemObj, containerForSubElem, elemName);
			} else if (elemObj.isArray()) { // array

				Value elemArrayObj(arrayValue);

				for(unsigned int index = 0; index < elemObj.size(); index++){

					Value tempObj = elemObj[index];

					if(tempObj.isString()){
						elemArrayObj.append(tempObj.asString());
					} else if (tempObj.isBool()){
						elemArrayObj.append(tempObj.asBool());
					} else if (tempObj.isDouble()){
						int convertedIntValue = float2int(tempObj.asDouble());
						std::string tmp_str((const char*)(int2str(convertedIntValue)));
						std::string attr_val = getLongName(tmp_str);
						elemArrayObj.append(attr_val);
					} else if(tempObj.isInt64()){
						std::string tmp_str((const char*)(int2str(tempObj.asInt())));
						std::string attr_val = getLongName(tmp_str);
						elemArrayObj.append(attr_val);
					} else {
						TTCN_Logger::log(TTCN_DEBUG, "Unexpected flow");
					}
					containerForSubElem[name_long.c_str()] = elemArrayObj;
				}

				containerForSubElem[name_long.c_str()] = elemArrayObj;
			} else if (elemObj.isString()) { // string
				containerForSubElem[name_long.c_str()] = elemObj;
			} else if (elemObj.isInt()) { // integer
				if(name_long != EXPIRATION_COUNTER && name_long != "currentByteSize" && name_long != "currentNrOfInstances" && name_long != "stateTag" && name_long != "maxInstanceAge") {
					std::string tmp_str((const char*)(int2str(elemObj.asInt())));
					std::string attr_val = getLongName(tmp_str);
					containerForSubElem[name_long.c_str()] = attr_val;
				} else {
					containerForSubElem[name_long.c_str()] = elemObj;
				}
			} else if (elemObj.isDouble()) {
				containerForSubElem[name_long.c_str()] = elemObj;
			} else if (elemObj.isBool()) { // bool
				containerForSubElem[name_long.c_str()] = elemObj;
			}
		}

		// 3. RootName: Change the oneM2M short name to long name for the TTCN-3
		rootName = getLongName(elemName.asString());
		if(rootName == ""){
			rootName = elemName.asString();
		}

		// * operationMonitorList attribute is not mandatory filed but, ATS requires this attribute
		if(rootName == EVENT_NOTIFICATION_CRITERIA) {
			Json::Value emptyArray_for_om;
			emptyArray_for_om.append("int1");
			containerForSubElem[OPERATION_MONITOR_LIST] = emptyArray_for_om;
		}

		// * rootTags such as 'm2m:sub', 'm2m:con' have to be under name "resource"
		if(rootName == SUBSCRIPTION || rootName == "aE" || rootName == "container") {

			Value elemForSub(objectValue);
			elemForSub[rootName] = containerForSubElem;

			objectRoot[RESOURCE] = elemForSub;
		} else { // if attribute is not rootTag
			objectRoot[rootName.c_str()] = containerForSubElem;
		}
		return objectRoot;
	}
}
