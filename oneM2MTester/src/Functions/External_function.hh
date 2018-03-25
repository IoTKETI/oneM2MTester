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
//  File:               External_function.hh
//  Description:        Header files for external functions in oneM2M_DualFaceMapping modules 
//  Rev:                R2I

#include "tinyxml2.h"
#include "json.h"
#include "json-forwards.h"
#include <TTCN3.hh>
#include <string>
#include <vector>

namespace OneM2M__DualFaceMapping {
	bool initial_mapping();
	std::string getShortName(std::string long_name);
	std::string getLongName(std::string short_name);
	std::vector<std::string> split(const std::string &s, char delim);
	bool long_to_short_name_mapping(const char* file_path, const std::string mapping_tag);
	void DeepClone(tinyxml2::XMLNode *newNode, const tinyxml2::XMLNode *nodeSrc, tinyxml2::XMLDocument *DocDest, const tinyxml2::XMLNode *parent);

	void DeepParser(tinyxml2::XMLElement* pRootElem, tinyxml2::XMLDocument* xmlDoclone, tinyxml2::XMLElement* pRootElemClone, tinyxml2::XMLElement* pDestParent);
	void DeepParserDec(tinyxml2::XMLElement* pRootElem, tinyxml2::XMLDocument* xmlDoclone, tinyxml2::XMLElement* pRootElemClone, tinyxml2::XMLElement* pDestParent);
	Json::Value JSONDeepParser(Json::Value jsonSrc, Json::Value jsonObjClone, Json::Value jsonParent);
	Json::Value JSONDeepParserDec(Json::Value jsonSrc, Json::Value jsonObjClone, Json::Value jsonTarget);

	/*********************************************************
	 * Parser functions will be here for all oneM2M resources
	 *********************************************************/

	/******* Encoding functions *******/

	/******* Decoding functions *******/
	CHARSTRING noti_JSON_Dec_Parser(const CHARSTRING& source_str, const CHARSTRING& serial_type);
	Json::Value noti_JSON_Dec_Parser_Deep(Json::Value objectSource, Json::Value objectRoot, Json::Value elemName);
}
