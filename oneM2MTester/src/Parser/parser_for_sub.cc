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
	CHARSTRING sub_JSON_Enc_Parser(const CHARSTRING& p__source){

		Value elemName;
		Value elemObj(objectValue);
		Value subElemObj(objectValue);
		Value resourceRoot(objectValue);
		Value jsonRoot(objectValue);

		Reader jsonReader;
		std::string name_short;

		const char* p_body			= (const char*)p__source;
		CHARSTRING encoded_message	= "";

		// 1. Make the JSOn object from the string data
		bool parsingSuccessful = jsonReader.parse(p_body, jsonRoot, false);
		if ( !parsingSuccessful ) {
			TTCN_Logger::log(TTCN_DEBUG, "JsonCPP API parsing error!");
			return "JsonCPP API parsing error!";
		}

		// 2. Save the root name "subscription" -> m2m:sub (changed)"
		CHARSTRING rootName = "m2m:sub";

		// 3. extract the sub element
		jsonRoot = jsonRoot.get("subscription", "");

		for (Value::iterator iter = jsonRoot.begin(); iter != jsonRoot.end(); ++iter) {
			elemName = iter.key();
			elemObj = jsonRoot.get(elemName.asString(), "");

			name_short = getShortName(elemName.asString());
			if(name_short == ""){
				name_short = elemName.asString();
			}

			if(elemObj.isObject()) { // object
				subElemObj = sub_JSON_Enc_Parser_Deep(elemObj, subElemObj, elemName);
			} else if (elemObj.isArray()) { // array
				Value elemArrayObj(arrayValue);

				for(unsigned int index = 0; index < elemObj.size(); index++){

					Value tempObj = elemObj[index];

					if(tempObj.isString()){
						elemArrayObj.append(tempObj.asString());
					}else if(tempObj.isBool()){
						elemArrayObj.append(tempObj.asBool());
					}else if(tempObj.isDouble()){
						elemArrayObj.append(tempObj.asDouble());
					}else if(tempObj.isInt64()){
						elemArrayObj.append(tempObj.asInt64());
					}
				}
				subElemObj[name_short.c_str()] = elemArrayObj;

			} else if (elemObj.isString()) { // string
				subElemObj[name_short.c_str()] = elemObj;
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

	Json::Value sub_JSON_Enc_Parser_Deep (Json::Value objectSource, Json::Value objectRoot, Json::Value elemName) {

		std::string name_short;
		std::string rootName;

		Value subElemObj(objectValue);
		Value containerForSubElem(objectValue);

		for (Value::iterator iter = objectSource.begin(); iter != objectSource.end(); ++iter) {

			Value elemName;
			Value elemObj(objectValue);

			// 1. Select the attribute with KEY
			elemName = iter.key();
			elemObj = objectSource.get(elemName.asString(), "");

			// 2. Change the oneM2M long name to short name for SUT
			name_short = getShortName(elemName.asString());
			if(name_short == ""){
				name_short = elemName.asString();
			}

			if (elemObj.isArray()) { // array
				Value elemArrayObj(arrayValue);

				for(unsigned int index = 0; index < elemObj.size(); index++){

					Value tempObj = elemObj[index];

					if(tempObj.isString()){
						std::string attr_val = getShortName(tempObj.asString());
						int tmp_int = atoi(attr_val.c_str());
						elemArrayObj.append(tmp_int);
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
						elemArrayObj.append(tempObj.asInt());
					} else {
						TTCN_Logger::log(TTCN_DEBUG, "Unexpected flow");
					}
					containerForSubElem[name_short.c_str()] = elemArrayObj;
				}
				containerForSubElem[name_short.c_str()] = elemArrayObj;
			}
		}

		// 3. RootName: Change the oneM2M short name to long name for the TTCN-3
		rootName = getShortName(elemName.asString());
		if(rootName == ""){
			rootName = elemName.asString();
		}

		objectRoot[rootName.c_str()] = containerForSubElem;

		return objectRoot;
	}
}
