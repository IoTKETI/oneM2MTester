/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Raduly, Csaba
 *
 ******************************************************************************/
#include "ReadXml.hh"
#include "XmlReader.hh"
//#include "libxml/xmlreader.h"

namespace ReadXml {

static XmlReaderWrap *reader;

INTEGER FromMemory(const OCTETSTRING& o)
{
  if (reader != 0) {
    TTCN_warning("Dangling XML reader encountered");
    delete reader;
  }
  TTCN_Buffer buf(o);
  reader = new XmlReaderWrap(buf);
  return 0;
}

void Cleanup()
{
  if (reader == 0) TTCN_error("XML reader not created");
  delete reader;
  reader = 0;
}

INTEGER XmlRead()
{
  if (reader == 0) TTCN_error("XML reader not created");
  return reader->Read();
}

xmlReaderTypes NodeType()
{
  if (reader == 0) TTCN_error("XML reader not created");
  return reader->NodeType();
}

INTEGER Depth()
{
  if (reader == 0) TTCN_error("XML reader not created");
  return reader->Depth();
}

CHARSTRING Name()
{
  if (reader == 0) TTCN_error("XML reader not created");
  return (const char*)reader->Name();
}

CHARSTRING Value()
{
  if (reader == 0) TTCN_error("XML reader not created");
  return (const char*)reader->Value();
}

CHARSTRING NsUri()
{
  if (reader == 0) TTCN_error("XML reader not created");
  return (const char*)reader->NamespaceUri(); // NUL results in empty string
}

// attribute handling

INTEGER FirstAttribute()
{
  if (reader == 0) TTCN_error("XML reader not created");
  return reader->MoveToFirstAttribute();
}

INTEGER NextAttribute()
{
  if (reader == 0) TTCN_error("XML reader not created");
  return reader->MoveToNextAttribute();
}

}
