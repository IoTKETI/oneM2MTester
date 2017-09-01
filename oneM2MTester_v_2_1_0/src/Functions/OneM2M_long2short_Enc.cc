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
* Updated:  2017-09-01                                                          	*
*****************************************************************************************/
//  File:               OneM2M_long2short_Enc.cc
//  Description:        Functions for oneM2M long to short, and short to long name conversion,
//                      and XML, JSON parsing   
//  Rev:                R2I

#include "OneM2M_DualFaceMapping.hh"
#include "tinyxml2.h"
#include "json.h"
#include "json-forwards.h"
#include <vector>
#include <string>
#include <iostream>
#include "HashMap.h"
#include <sstream>
#include <cctype>
#include <stdint.h>
#include <fstream>
#include <algorithm>
#include <functional>
#include "External_function.hh"
#include <stdlib.h>

using namespace tinyxml2;
using namespace Json;
using namespace std;

struct MyKeyHash {
	unsigned long operator()(const std::string& k) const
	{
	  	return k.length() % 10;
	}
};

namespace OneM2M__DualFaceMapping {
	//initial array for storing resource name
	int array_size = 20480;
	//long-2-short name mapping
	HashMap<std::string, std::string, 50, MyKeyHash> hmap_l2s; 
	//short-2-long name mapping
	HashMap<std::string, std::string, 50, MyKeyHash> hmap_s2l;

	//TODO to be replace by PIXIT value
	const char* XML_NAMESPACE 	= "http://www.onem2m.org/xml/protocols";
	//TODO to be added in constant module
	const char* NAMESPACE_TAG	= "xmlns:m2m";
	const char* RESOURCE_NAME	= "resourceName";
	const char* PRIMITIVE_CONTENT 	= "PrimitiveContent";		
	const char* XSI			= "http://www.w3.org/2001/XMLSchema-instance";
	const char* XSI_TAG		= "xmlns:xsi";

	const char* CONTENT_ATTR	= "content"; 
	const char* EMBED_VALUES_ATTR	= "embed_values";
	const char* ELEM_LIST_ATTR	= "elem_list";

	/**
         * @desc encode oneM2M resource primitive e.g. AE, container etc. into primitiveContent primitive
         * @source_str: resource primitive representation
	 * @serial_type: serialization type in which the resource primitive is represented
         */
	CHARSTRING f__primitiveContent__Dec(const CHARSTRING& source_str, const CHARSTRING& serial_type){

		if(!initial_mapping()){
			TTCN_Logger::log(TTCN_DEBUG, "\n[WARNING]oneM2M lon&short mapping initialization failed!!\n\n");
		}

		CHARSTRING encoded_message	= "";
		const char* p_body		= (const char*)source_str;		
	
		if( "" == source_str){
			return "";
		}

		if("json" == serial_type){

			Value jsonDoc(objectValue);
			Value jsonRoot(objectValue);
			Reader jsonReader;
			Value rootTag;
			Value elemName;			
			Value elemObj(objectValue);
			Value subelemObj(objectValue);
			Value grandelemObj(objectValue);

			Value jsonRootClone(objectValue);
			Value jsonObjClone(objectValue);
			Value jsonArrayRoot(arrayValue);		

			std::string rootTag_short;
			std::string name_long;
			std::string parent_tag = "";

			bool parsingSuccessful = jsonReader.parse(p_body, jsonRoot, false);
			if ( !parsingSuccessful ) {
				TTCN_Logger::log(TTCN_DEBUG, "JsonCPP API parsing error!");
				return "JsonCPP API parsing error!";
			}

			TTCN_Logger::log(TTCN_DEBUG, "[Decoding] Read Json document for decoding:\n%s", jsonRoot.toStyledString().c_str());	
	
			if(jsonRoot.isObject()){  
				
				for (Value::iterator iter = jsonRoot.begin(); iter != jsonRoot.end(); ++iter) {
					
					elemName = iter.key();
					elemObj = jsonRoot.get(elemName.asString(), "");
					
					name_long = getLongName(elemName.asString()); 
					if(name_long == ""){
						name_long = elemName.asString();
					}

					parent_tag = name_long;
					
					if(elemObj.isArray()){
						
						for(unsigned int index = 0; index < elemObj.size(); index++){
							grandelemObj = elemObj[index];
							jsonObjClone[parent_tag.c_str()] = grandelemObj;
						}


					}else if(!elemObj.isObject() && !elemObj.isArray()){
						jsonObjClone[name_long.c_str()] = elemObj;

					}else if(elemObj.isObject()){  
						
						jsonObjClone[parent_tag.c_str()] = elemObj;
						jsonRootClone = JSONDeepParserDec(jsonObjClone, jsonRootClone, jsonRootClone);
						
					}
				}

			}else if(jsonRoot.isArray()){
			
				for(unsigned int index = 0; index < jsonRoot.size(); index++){
					elemObj = jsonRoot[index];
					jsonObjClone = elemObj;
				}
			}

			jsonRootClone = JSONDeepParserDec(jsonObjClone, jsonRootClone, jsonRootClone);

			TTCN_Logger::log(TTCN_DEBUG, "Pretty print of DECODED JSON message:\n%s", jsonRootClone.toStyledString().c_str());

			StyledWriter writer;
			std::string json_str = writer.write(jsonRootClone);

			CHARSTRING temp_cs(json_str.c_str());
			encoded_message	= temp_cs;

		}else if("xml" == serial_type){
						
			XMLDocument xmlDoc;
			xmlDoc.Parse(p_body);			
			XMLNode* pRoot;
			XMLDeclaration* pDecl;
			XMLNode* pNode;		
			XMLElement* pElem;
			XMLElement* pElemClone;
			XMLElement* pRootClone;
			XMLElement* pResourceRoot;
			const char* name_el;
			std::string name_long;
			XMLDocument xmlDoclone;	
			XMLElement* pRootElem;
			XMLNode* pTemp;

			for(pTemp = xmlDoc.FirstChild(); pTemp != NULL; pTemp = xmlDoc.LastChild()){
				
				if(pTemp->ToDeclaration()){

					pDecl = pTemp->ToDeclaration();
					
				}else if(pTemp->ToElement()){

					pRoot = pTemp->ToElement();
					XMLPrinter print2;
					pRoot->Accept(&print2);

					break;	
				}
			}

			if(pRoot){
				pDecl = pRoot->ToDeclaration();
				if(pDecl){
					//TTCN_Logger::log(TTCN_DEBUG, "pRoot->ToDeclaration()!");
				}

				pRootElem = pRoot->ToElement();

				if(pRootElem){ 
					
					name_el = pRootElem->Name(); 
					name_long = getLongName(name_el);			

					if("" == name_long){
						name_long = name_el;
					}
			
					pRootClone 	= xmlDoclone.NewElement(PRIMITIVE_CONTENT); 
					pResourceRoot 	= xmlDoclone.NewElement(name_long.c_str());
			
					if(pRootElem->Attribute(NAMESPACE_TAG)){

						std::string  val_xmlns_ns = pRootElem->Attribute(NAMESPACE_TAG);
						pRootClone->SetAttribute(NAMESPACE_TAG, val_xmlns_ns.c_str());

					}else
						pRootClone->SetAttribute(NAMESPACE_TAG, XML_NAMESPACE);
			
					if(pRootElem->Attribute((getShortName(RESOURCE_NAME).c_str()))){ 
						std::string val_resourceName = pRootElem->Attribute((getShortName(RESOURCE_NAME).c_str()));
						pResourceRoot->SetAttribute( getLongName(getShortName(RESOURCE_NAME)).c_str(), val_resourceName.c_str() );
					}

					pRootClone->InsertEndChild(pResourceRoot);
					
					xmlDoclone.InsertEndChild(pRootClone);
					for(pNode = pRoot->FirstChild(); pNode != NULL; pNode = pNode->NextSibling()){
						
						pElem = pNode->ToElement();		
						DeepParserDec(pElem, &xmlDoclone, pElemClone, pResourceRoot);
					}
					
					XMLPrinter print;
					xmlDoclone.Accept(&print);
					encoded_message = print.CStr();
				}
			}

		}

		return encoded_message;
	}
	/**
	 * @desc convert string from uppercase to lowercase
	 * @p__string: string containing uppercase letters
	 * return lowercase string
	 */
	CHARSTRING f__upper2lower(const CHARSTRING& p__string){
		const char* tmp_str = (const char*)p__string;
		std::string s(tmp_str);

		for(unsigned int i = 0; i < s.length(); i++){
			if(s[i] >= 'A' && s[i] <= 'Z'){
				s[i] = char(((int)s[i])+32);
		 	}
		}

		CHARSTRING s_str(s.c_str());

		return s_str;
	}

	/**
	 * @desc Serialize oneM2M request/responsePrimitives
 	 * @p__source: primitiveContent
	 * @p__serialization__type: serialization type (XML or JSON)
	 * 
         */
	CHARSTRING f__serialization__Enc(const CHARSTRING& p__source, const CHARSTRING& p__serialization__type){

//		TTCN_Logger::log(TTCN_DEBUG, "Enter f_serialization_Enc()...");

		if(!initial_mapping()){
			TTCN_Logger::log(TTCN_DEBUG, "[WARNING]oneM2M long-short mapping initialization failed!!");
		}

		const char* p_body		= (const char*)p__source;
		CHARSTRING serial_str		= p__serialization__type;
		CHARSTRING encoded_message	= "";

		if("json" == serial_str){

			Value jsonDoc(objectValue);
			Value jsonRoot(objectValue);
			Reader jsonReader;

			Value rootTag;
			Value elemName;			
			Value elemObj(objectValue);
			Value subelemObj(objectValue);
			Value grandelemObj(objectValue);

			Value jsonRootClone(objectValue);
			Value jsonObjClone(objectValue);

			std::string rootTag_short;
			std::string name_short;
			std::string parent_tag = "";

			bool parsingSuccessful = jsonReader.parse(p_body, jsonRoot, false);
			if ( !parsingSuccessful ) {
				TTCN_Logger::log(TTCN_DEBUG, "JsonCPP API parsing error!");
				return "JsonCPP API parsing error!";
			}

			if(jsonRoot.isObject()){  

				for (Value::iterator iter = jsonRoot.begin(); iter != jsonRoot.end(); ++iter) {
					
					elemName = iter.key();
					elemObj = jsonRoot.get(elemName.asString(), "");
			
					name_short = getShortName(elemName.asString());
					if(name_short == ""){
						name_short = elemName.asString();
					}

					parent_tag = name_short;
					
					if(elemObj.isArray()){
						for(unsigned int index = 0; index < elemObj.size(); index++){
							grandelemObj = elemObj[index];
							jsonObjClone[parent_tag.c_str()] = grandelemObj;
						}

					}else if(!elemObj.isObject() && !elemObj.isArray()){ 

						jsonRootClone[name_short.c_str()] = elemObj;


					}else if(elemObj.isObject()){ //go deep parsing child objects
						jsonObjClone[parent_tag.c_str()] = elemObj;

						jsonRootClone = JSONDeepParser(jsonObjClone, jsonRootClone, jsonRootClone);
					}
										
				}

			}

			StyledWriter writer;
			std::string json_str = writer.write(jsonRootClone);

			CHARSTRING temp_cs(json_str.c_str());
			encoded_message = temp_cs;

			return encoded_message;

		}else if("xml" == serial_str){

			XMLDocument xmlDoc;
			xmlDoc.Parse(p_body);

			XMLNode* pRoot = xmlDoc.FirstChild();
			XMLNode* pNode;
			XMLNode* pTempNode;
			
			XMLElement* pElem;
			XMLElement* pElemClone;
			XMLElement* pRootClone;
			XMLElement* pRootElem2;

			const char* name_el;
			std::string name_short;
			XMLDocument xmlDoclone;

			XMLElement* pRootElem = pRoot->ToElement();
			name_el = pRootElem->Name();
			name_short = getShortName(name_el);

			if(name_short == ""){
				name_short = name_el;
			}
						
			if(PRIMITIVE_CONTENT == name_short){
				pTempNode = pRoot->FirstChild();
				pRootElem2 = pTempNode->ToElement();
				
				pRootElem = pRootElem2;
				name_el = pRootElem2->Name();
				name_short = getShortName(name_el);

				if(name_short == ""){
					name_short = name_el;
				}
			}
			XMLDeclaration* xml_declar = xmlDoclone.NewDeclaration();
			xmlDoclone.InsertFirstChild(xml_declar);

			pRootClone = xmlDoclone.NewElement(name_short.c_str());
			
			if(pRootElem->Attribute(NAMESPACE_TAG)){
				std::string  val_xmlns_ns = pRootElem->Attribute(NAMESPACE_TAG);
				pRootClone->SetAttribute(NAMESPACE_TAG, val_xmlns_ns.c_str());
			}
			if(pRootElem->Attribute(XSI_TAG)){
				std::string  val_xmlns_xsi = pRootElem->Attribute(XSI_TAG);
				pRootClone->SetAttribute(XSI_TAG, val_xmlns_xsi.c_str());
			}
			
			if(!pRootElem->Attribute(NAMESPACE_TAG) && !pRootElem->Attribute(XSI_TAG)){

				pRootClone->SetAttribute(NAMESPACE_TAG, XML_NAMESPACE);
				pRootClone->SetAttribute(XSI_TAG, XSI);
			}

			if(pRootElem->Attribute(RESOURCE_NAME)){ 
				std::string val_resourceName = pRootElem->Attribute(RESOURCE_NAME);
				pRootClone->SetAttribute((getShortName(RESOURCE_NAME).c_str()), val_resourceName.c_str());
			}				

			xmlDoclone.InsertEndChild(pRootClone);
			for(pNode = pTempNode->FirstChild(); pNode != NULL; pNode = pNode->NextSibling()){
				pElem = pNode->ToElement();
				
				DeepParser(pElem, &xmlDoclone, pElemClone, pRootClone);

			}
			
			XMLPrinter print;
			xmlDoclone.Accept(&print);
			encoded_message = print.CStr();

			return encoded_message;

		}

		return "";

	}

	//===========================
	// internal functions
	// mapping long-2-short names
	//===========================
	//initial mapping functions
	bool initial_mapping(){

		const char* file_path_l2s 	= "../Lib/Resource_Mapping_Table/long-2-short-mapping.txt";
		const char* file_path_s2l	= "../Lib/Resource_Mapping_Table/short-2-long-mapping.txt";
		const std::string LONG2SHORT	= "L2S";
		const std::string SHORT2LONG	= "S2L";
		bool l2s_flag = long_to_short_name_mapping(file_path_l2s, LONG2SHORT);
		bool s2l_flag = long_to_short_name_mapping(file_path_s2l, SHORT2LONG);
		if(l2s_flag && s2l_flag)
			return true;

		return false;
	}

	//read mapping information from a text file and add the mapping info into a hash map
	bool long_to_short_name_mapping(const char* file_path, const std::string mapping_tag){

		bool flag = false;
		int pos = 0;
		size_t pos_1 = 0;
		size_t pos_2 = 0;

		char* map_array = new char[array_size];

		std::string map_str 	= "";
		std::string str_temp 	= "";
		std::string source_str 	= "";
		std::string dest_str 	= "";

		std::vector<std::string> vector_map_list;
		std::vector<std::string> vector_l2s;

		ifstream fin(file_path);

		if(fin.is_open()){
			pos =0;
			while(!fin.eof() && pos < array_size){
				fin.get(map_array[pos]);
				pos++;
			}
		
			map_array[pos-1] = '\0';

			for(int i = 0; map_array[i] != '\0'; i++)
				map_str = map_str + map_array[i];

			if(map_str != ""){

				vector_map_list = split(map_str, '\n');

				for(unsigned int i = 0;i < vector_map_list.size(); i++){

					pos_2 = map_str.find('\n', pos_1);
					vector_map_list[i] = map_str.substr(pos_1, (pos_2-pos_1));

					str_temp = vector_map_list[i];
					vector_l2s = split(str_temp, '=');
					for(unsigned int j = 0; j < vector_l2s.size(); j++){
						if(0 == j && vector_l2s[j] != "")
							source_str = vector_l2s[j];
						if(j > 0 && vector_l2s[j] != "")
							dest_str = vector_l2s[j];
					}
					
					if(source_str != "" && dest_str != ""){
						if("L2S" == mapping_tag){
							hmap_l2s.put(source_str, dest_str);
							flag = true;
	
						}else if("S2L" == mapping_tag){
							hmap_s2l.put(source_str, dest_str);
							flag = true;
							
						}else
							return false;
						

						
					}else
						return false;

					pos_1 = pos_2 + 1;
				}

			}else
				return false;
		}else
			return false;

		return flag;

	}//


	/**
	 * @desc get long/short name from hash mapping table * 
         */	
	std::string getShortName(std::string long_name){ 

		std::string short_name = "";
		bool isOk =  hmap_l2s.get(long_name, short_name);
		if(isOk)
			return short_name;

		return "";
	}//
	
	std::string getLongName(std::string short_name){

		std::string long_name = "";
		bool isOk =  hmap_s2l.get(short_name, long_name);
		if(isOk)
			return long_name;

		return "";
	}//

	/**
	 * @desc split a string with specified delim and get the substr after the delim
	 *
	 */
	CHARSTRING f__split(const CHARSTRING& p__cs, const CHARSTRING& p__delim){
		
		const char* cs_str = (const char*)p__cs;
		std::string src_str(cs_str);
		
		const char* temp_str = (const char*)p__delim;
		char* delim_str = const_cast<char*>(temp_str);

		std::vector<std::string> vector_elems = split(src_str, *delim_str);
		for(unsigned int i=0; i< vector_elems.size(); i++){
		
			CHARSTRING tmp_str((vector_elems[i]).c_str());

			if(i == 1 && tmp_str != ""){		
				return tmp_str;
			}		
		}

		return "";
	}
	/**
	 * @desc find charstring p__str1 in charstring p__source if any and replace with charstring p__str1 with charstring p__str2
	 * @desc if p__str1 is not found, charstring p__source will not be updated
	 */
	CHARSTRING f__replace(const CHARSTRING& p__source, const CHARSTRING& p__str1, const CHARSTRING& p__str2){
		
		const char* cs_temp  = (const char*)p__source;
		const char* cs_temp2 = (const char*)p__str1;
		const char* cs_temp3 = (const char*)p__str2;

		std::string str_source(cs_temp);
		std::string str1(cs_temp2);
		std::string str2(cs_temp3);
		std::size_t pos = str_source.find_first_of(str1);
		while(pos != std::string::npos){
			
			str_source.replace(pos, 1, str2);
			pos = str_source.find_first_of(str1, pos+1);

		}
		CHARSTRING tmp_str(str_source.c_str());
		return tmp_str;	
		
	}

	/**
	 * @desc generate oneM2M addressing format for different protocol binding with the received the request URI
	 * @desc : SP-relative addressing format //in-cse/csename --> /~/in-cse/csename
	 */
	CHARSTRING f__adressingFormatter(const CHARSTRING& p__source, const CHARSTRING& p__str1, const CHARSTRING& p__str2){
		
		const char* cs_temp  = (const char*)p__source;
		const char* cs_temp2 = (const char*)p__str1;
		const char* cs_temp3 = (const char*)p__str2;

		std::string str_source(cs_temp);
		std::string str1(cs_temp2); 
		std::string str2(cs_temp3); 
		std::size_t pos = str_source.find_first_of(str1);
		if(pos != std::string::npos){
			
			str_source.replace(pos, 1, str2);
			
		}
		CHARSTRING tmp_str(str_source.c_str());
		return tmp_str;	
	}

	/**
	 * @desc split a structured uri into single elements that are separated by slash "/"
	 *
	 */
	CoAP__Types::Charstring__List f__split__uri(const CHARSTRING& p__uri, const CHARSTRING& p__delim){

		CoAP__Types::Charstring__List csList;		

		const char* cs_temp = (const char*)p__uri;

		std::string uri_str(cs_temp);
		std::string tmp_string = uri_str.substr(0,1);
		if( (uri_str.substr(0,1)) != "/"){
			TTCN_Logger::log(TTCN_DEBUG, "check first character of uri_string is '/' or not :(%s)", tmp_string.c_str());
		}else{

			uri_str = uri_str.substr(1);

			const char* temp_str = (const char*)p__delim;
			char* delim_str = const_cast<char*>(temp_str);

			std::vector<std::string> vector_elems = split(uri_str, *delim_str);

			for(unsigned int i = 0; i < vector_elems.size(); i++){

				CHARSTRING tmp_str((vector_elems[i]).c_str());
			
				if(tmp_str != ""){				
					csList[i] = tmp_str;
				
				}	
				
			}
		}
	
		return csList;

	}
	/**
	 * @desc split string with delimiter
         */
	std::vector<std::string> split(const std::string &s, char delim) {
		std::vector<std::string> elems;
		stringstream ss(s);
		std::string item;
		while (getline(ss, item, delim)) {
		    elems.push_back(item);
		}
		return elems;
       }

	/**
	 * @desc Clone one node including children into another node
	 * @newNode: new node
	 * @nodeSrc: source node
	 * @DocDest: destination xml document
	 * @parent: parent node
	 */
	void DeepClone(XMLNode *newNode, const XMLNode *nodeSrc, XMLDocument *DocDest, const XMLNode *parent) {
		const tinyxml2::XMLNode *child = nodeSrc->FirstChild();
		tinyxml2::XMLNode *newNode2;
		if (child)
		{
			newNode2 = child->ShallowClone(DocDest);
			DeepClone(newNode2,child,DocDest,nodeSrc);
			newNode->InsertFirstChild(newNode2);
		}
		else return;
		const tinyxml2::XMLNode *child2;
		tinyxml2::XMLNode *lastClone = newNode2;
		while (1)
		{
			child2 = child->NextSibling();
			if (!child2)
			break;
			tinyxml2::XMLNode *newNode2 = child2->ShallowClone(DocDest);
			DeepClone(newNode2,child2,DocDest,nodeSrc);
			newNode->InsertAfterChild(lastClone,newNode2);
			child = child2;
			lastClone = newNode2;
		}
	}

	/**
	 * @desc Parse each attribute and convert it from long to short name
	 * @pRootElem: source element
	 * @xmlDoclone: destination xml document
	 * @pRootElemClone: destination element
	 * @pDestParent: parent element of destination element
	 */
	void DeepParser(XMLElement* pSrcElem, XMLDocument* xmlDoclone, XMLElement* pSrcElemClone, XMLElement* pDestParent){

		const char* name_el;
		std::string name_short;
		std::string val_resourceName;
		const char* val_el;

		name_el = pSrcElem->Name();
		name_short = getShortName(name_el);
		if(name_short == ""){
			name_short = name_el;
		}

		pSrcElemClone = xmlDoclone->NewElement(name_short.c_str());

		if( pSrcElem->Attribute(RESOURCE_NAME) ){ 
			val_resourceName = pSrcElem->Attribute(RESOURCE_NAME);
			pSrcElemClone->SetAttribute((getShortName(RESOURCE_NAME).c_str()), val_resourceName.c_str());
		}

		pDestParent->InsertEndChild(pSrcElemClone);

		if(pSrcElem->FirstChildElement() != NULL){
			for(XMLElement* pElem = pSrcElem->FirstChildElement(); pElem != NULL; pElem = pElem->NextSiblingElement()){

				XMLElement* pElemClone;

				DeepParser(pElem, xmlDoclone, pElemClone, pSrcElemClone);
			}

		}else{
			val_el = pSrcElem->GetText();
			if(!(val_el == NULL))
				pSrcElemClone->SetText(val_el);

		}

	}
	/**
	 * @desc Parse each attribute and convert it from short to long name
	 * @pRootElem: source element
	 * @xmlDoclone: destination xml document
	 * @pRootElemClone: destination element
	 * @pDestParent: parent element of destination element
	 */
	void DeepParserDec(XMLElement* pSrcElem, XMLDocument* xmlDoclone, XMLElement* pSrcElemClone, XMLElement* pDestParent){

		const char* name_el;
		std::string name_long;
		std::string val_resourceName;
		const char* val_el;

		name_el = pSrcElem->Name();
		name_long = getLongName(name_el);
		
		if( "" == name_long){
			name_long = name_el;
		}

		pSrcElemClone = xmlDoclone->NewElement(name_long.c_str());
		
		pDestParent->InsertEndChild(pSrcElemClone);

		if(pSrcElem->FirstChildElement() != NULL){
			for(XMLElement* pElem = pSrcElem->FirstChildElement(); pElem != NULL; pElem = pElem->NextSiblingElement()){

				XMLElement* pElemClone;

				DeepParserDec(pElem, xmlDoclone, pElemClone, pSrcElemClone);
			}

		}else{
			val_el = pSrcElem->GetText();
			if(!(val_el == NULL))
				pSrcElemClone->SetText(val_el);

		}

	}

	/**
	 * @jsonSrc: json root element that is parsing
         * @jsonCurrent: input a jsonCurrent
	 * return a jsonParent included the encode json data
	 */
	Json::Value JSONDeepParser(Json::Value jsonSrc, Json::Value jsonObjClone, Json::Value jsonTarget){
	
		Value elemName;
		Value elemObj(objectValue);
		Value subelemObj(objectValue);
		Value subObjClone(objectValue);
		Value elemObjClone(objectValue);
		Value grandelemObj(objectValue);
		Value tempObj;
		Value tempObjClone;
		
		std::string parent_tag = "";
		std::string root_tag = "";
		std::string name_short = "";

		for (Value::iterator iter = jsonSrc.begin(); iter != jsonSrc.end(); iter++) {
			
			elemName = iter.key();
			elemObj = jsonSrc.get(elemName.asString(), "");
			
			name_short = getShortName(elemName.asString());
			if(name_short == ""){
				name_short = elemName.asString();
			}

			parent_tag = name_short;

			if(elemObj.isObject()){ 

				root_tag = name_short;

				elemObjClone = JSONDeepParser(elemObj, elemObjClone, jsonObjClone);

				for (Value::iterator iter = elemObj.begin(); iter != elemObj.end(); ++iter) {

					elemName = iter.key();
					subelemObj = elemObj.get(elemName.asString(), "");

					name_short = getShortName(elemName.asString());
					if(name_short == ""){
						name_short = elemName.asString();
					}
					
					CHARSTRING tmp_1(elemName.asCString());

					if(CONTENT_ATTR == tmp_1){
						TTCN_Logger::log(TTCN_DEBUG, "\n\n Content attribute...");					
						
					}
	
					bool flag_1 = false;
					
					parent_tag = name_short;

					if(subelemObj.isObject()){
						
						for (Value::iterator iter = subelemObj.begin(); iter != subelemObj.end(); ++iter) {
							
							elemName = iter.key();
							
							grandelemObj = subelemObj.get(elemName.asString(), "");

							name_short = getShortName(elemName.asString());
							if(name_short == ""){
								name_short = elemName.asString();
							}

							CHARSTRING tmp_2(elemName.asCString());

							if(EMBED_VALUES_ATTR == tmp_2){
								
								flag_1 = true;
							}


							if(grandelemObj.isArray()){
								Value elemArrayObj(arrayValue);

								for(unsigned int index = 0; index < grandelemObj.size(); index++){

									tempObj = grandelemObj[index];

									if(tempObj.isString()){
										if(flag_1){
											elemObjClone[parent_tag.c_str()] = tempObj.asString();

										}else{
								
											elemArrayObj.append(tempObj.asString());
											elemObjClone[parent_tag.c_str()] = elemArrayObj;
										}
									}else if(tempObj.isBool()){

										elemArrayObj.append(tempObj.asBool());
										elemObjClone[parent_tag.c_str()] = elemArrayObj;

									}else if(tempObj.isDouble()){

										elemArrayObj.append(tempObj.asDouble());
										elemObjClone[parent_tag.c_str()] = elemArrayObj;

									}else if(tempObj.isInt64()){

										elemArrayObj.append(tempObj.asInt64());
										elemObjClone[parent_tag.c_str()] = elemArrayObj;

									}else if(tempObj.isObject()){
										tempObjClone = JSONDeepParser(tempObj, tempObjClone, subObjClone);

										elemArrayObj.append(tempObjClone);

										subObjClone[name_short.c_str()] = elemArrayObj;

										elemObjClone[parent_tag.c_str()] = subObjClone;

									}
								}


							}

						}

					}else if(subelemObj.isArray()){
						Value elemArrayObj(arrayValue);

						for(unsigned int index = 0; index < subelemObj.size(); index++){

							tempObj = subelemObj[index];
							
							if(tempObj.isString()){
								elemArrayObj.append(tempObj.asString());

							}else if(tempObj.isBool()){
							
								elemArrayObj.append(tempObj.asBool());

							}else if(tempObj.isDouble()){
								
								elemArrayObj.append(tempObj.asDouble());

							}else if(tempObj.isInt64()){

								elemArrayObj.append(tempObj.asInt64());

							}else if(tempObj.isObject()){

								tempObjClone = JSONDeepParser(tempObj, tempObjClone, subObjClone);

								elemArrayObj.append(tempObjClone);								

							}

							elemObjClone[parent_tag.c_str()] = elemArrayObj;
						}

					}					

				}

				jsonObjClone[root_tag.c_str()] = elemObjClone;

				
			}else if(elemObj.isString()){			

				if(	"op" 	== name_short   || "operation" 			== name_short || 
					"ty" 	== name_short   || "resourceType" 		== name_short || 
	            			"acop" 	== name_short   || "accessControlOperations" 	== name_short ||
                    			"rcn" 	== name_short   || "resultContent" 		== name_short ||
					"csy"	== name_short	|| "consistencyStrategy" 	== name_short ||
					"cst"	== name_short	|| "cseType"			== name_short ){ //TODO: add all enumerated type here
					
					std::string attr_val = getShortName(elemObj.asString());

					int tmp_int = atoi(attr_val.c_str());

					jsonObjClone[name_short.c_str()] = tmp_int;

				}else
					jsonObjClone[name_short.c_str()] = elemObj.asString();

			}else{				
				jsonObjClone[name_short.c_str()] = elemObj;
			}

		}

		return jsonObjClone;

	}


	/**
	 * @desc decoding json formated short name represented data into long name format 
	 * @jsonSrc: json root element that is parsing
         * @jsonCurrent: input a jsonCurrent
	 * return a jsonParent included the encode json data
	 */
	Json::Value JSONDeepParserDec(Json::Value jsonSrc, Json::Value jsonObjClone, Json::Value jsonTarget){
		
		Value elemName;
		Value elemObj(objectValue);
		Value subelemObj(objectValue);
		Value subObjClone(objectValue);
		Value elemObjClone(objectValue);
		Value grandelemObj(objectValue);
		Value tempObj;
		Value tempObjClone;
	
		std::string parent_tag = "";
		std::string root_tag = "";
		std::string name_long = "";

		for (Value::iterator iter = jsonSrc.begin(); iter != jsonSrc.end(); iter++) {
			
			elemName = iter.key();
			
			elemObj = jsonSrc.get(elemName.asString(), "");
			
			name_long = getLongName(elemName.asString());
			if(name_long == ""){
				name_long = elemName.asString();
			}
			
			parent_tag = name_long;

			if(elemObj.isObject()){ 
				root_tag = name_long;

				elemObjClone = JSONDeepParserDec(elemObj, elemObjClone, jsonObjClone);
				
				for (Value::iterator iter = elemObj.begin(); iter != elemObj.end(); ++iter) {

					elemName = iter.key();
					subelemObj = elemObj.get(elemName.asString(), "");

					name_long = getLongName(elemName.asString());
					if(name_long == ""){
						name_long = elemName.asString();
					}
					
					parent_tag = name_long;

					if(subelemObj.isObject()){

						for (Value::iterator iter = subelemObj.begin(); iter != subelemObj.end(); ++iter) {
							
							elemName = iter.key();
							grandelemObj = subelemObj.get(elemName.asString(), "");

							name_long = getLongName(elemName.asString());
							if(name_long == ""){
								name_long = elemName.asString();
							}
							
							if(grandelemObj.isArray()){
								
								Value elemArrayObj(arrayValue);

								for(unsigned int index = 0; index < grandelemObj.size(); index++){
								
									Value tempObj = grandelemObj[index];
																	
									if(tempObj.isIntegral()){

										std::string tmp_str((const char*)(int2str(tempObj.asLargestInt())));
										
										std::string attr_val = getLongName(tmp_str);
										elemArrayObj.append(attr_val);

										subObjClone[name_long.c_str()] = elemArrayObj;
										elemObjClone[parent_tag.c_str()] = subObjClone;
										

									}else if(tempObj.isString()){
										
										elemArrayObj.append(tempObj.asString());
										elemObjClone[parent_tag.c_str()] = elemArrayObj;

									}else if(tempObj.isBool()){

										elemArrayObj.append(tempObj.asBool());
										elemObjClone[parent_tag.c_str()] = elemArrayObj;

									}else if(tempObj.isDouble()){

										elemArrayObj.append(tempObj.asDouble());
										elemObjClone[parent_tag.c_str()] = elemArrayObj;

									}else if(tempObj.isUInt()){
										elemArrayObj.append(tempObj.asUInt());
										elemObjClone[parent_tag.c_str()] = elemArrayObj;

									}else if(tempObj.isObject()){

										tempObjClone = JSONDeepParserDec(tempObj, tempObjClone, subObjClone);

										elemArrayObj.append(tempObjClone);

										subObjClone[name_long.c_str()] = elemArrayObj;

										elemObjClone[parent_tag.c_str()] = subObjClone;

									}
								}

							}

						}

					}else if(subelemObj.isArray()){

						Value elemArrayObj(arrayValue);

						for(unsigned int index = 0; index < subelemObj.size(); index++){

							tempObj = subelemObj[index];
							
							if(tempObj.isString()){
						
								elemArrayObj.append(tempObj.asString());

							}else if(tempObj.isBool()){
							
								elemArrayObj.append(tempObj.asBool());

							}else if(tempObj.isDouble()){
								
								elemArrayObj.append(tempObj.asDouble());

							}else if(tempObj.isInt64()){

								elemArrayObj.append(tempObj.asInt64());

							}else if(tempObj.isObject()){

								tempObjClone = JSONDeepParserDec(tempObj, tempObjClone, subObjClone);

								elemArrayObj.append(tempObjClone);								

							}

							elemObjClone[parent_tag.c_str()] = elemArrayObj;
						}

					}					

				}

				jsonObjClone[root_tag.c_str()] = elemObjClone;


			}else if(elemObj.isString()){
				
				jsonObjClone[name_long.c_str()] = elemObj.asString();

			}else if(elemObj.isInt()){ 
				
				std::string tmp_str((const char*)(int2str(elemObj.asInt())));
				
				
				if(	"op" 	== name_long   || "operation" 			== name_long || 
					"ty" 	== name_long   || "resourceType" 		== name_long || 
	            			"acop" 	== name_long   || "accessControlOperations" 	== name_long ||
                    			"rcn" 	== name_long   || "resultContent" 		== name_long ||
					"csy"	== name_long   || "consistencyStrategy" 	== name_long ||
					"mt"	== name_long   || "memberType" 			== name_long ||
					"nct"	== name_long   || "notificationContentType"	== name_long ||
					"cst"	== name_long   || "cseType"			== name_long
				 ){	
					std::string attr_val = getLongName(tmp_str);		
					jsonObjClone[name_long.c_str()] = attr_val;
				

				}else							
					jsonObjClone[name_long.c_str()] = elemObj.asInt();
				

			}else if(elemObj.isInt64()){
								
				jsonObjClone[name_long.c_str()] = elemObj;
	
			}else if(elemObj.isBool()){
							
				jsonObjClone[name_long.c_str()] = elemObj;
	
			}else{									
				jsonObjClone[name_long.c_str()] = elemObj;
			}

		}
		
		return jsonObjClone;

	}

}
