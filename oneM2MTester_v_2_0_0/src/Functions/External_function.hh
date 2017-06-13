/****************************************************************************************
* Copyright (c) 2017  Korea Electronics Technology Institute.				*
* All rights reserved. This program and the accompanying materials			*
* are made available under the terms of                                         	*
* - Eclipse Public License v1.0(http://www.eclipse.org/legal/epl-v10.html),     	*
* - BSD-3 Clause Licence(http://www.iotocean.org/license/),                    		* 
* - MIT License   (https://github.com/open-source-parsers/jsoncpp/blob/master/LICENSE), * 
* - zlib License  (https://github.com/leethomason/tinyxml2#license).                    *	
*											*
* Contributors:										*
*   Ting Martin MIAO - initial implementation                                   	*   
*   Nak-Myoung Sung                                                             	*
*                                                                               	* 
* Updated:  2017-05-13                                                          	*
*****************************************************************************************/
//  File:               External_function.hh
//  Description:        Header files for external functions in oneM2M_DualFaceMapping modules 
//  Rev:                R2I


#include <TTCN3.hh>
#include <string>
#include <vector>
#include "tinyxml2.h"
#include "json.h"
#include "json-forwards.h"

#if TTCN3_VERSION != 60100
#error Version mismatch detected.\
 Please check the version of the TTCN-3 compiler and the base library.\
 Run make clean and rebuild the project if the version of the compiler changed recently.
#endif

#ifndef LINUX
#error This file should be compiled on LINUX
#endif

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

}


