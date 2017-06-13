/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Delic, Adam
 *   Raduly, Csaba
 *
 ******************************************************************************/
#ifndef JUnitLogger_HH
#define JUnitLogger_HH

namespace TitanLoggerApi { class TitanLogEvent; }

#include "ILoggerPlugin.hh"
#include <stdio.h>

class JUnitLogger: public ILoggerPlugin
{
public:
  JUnitLogger();
  virtual ~JUnitLogger();
  inline bool is_static() { return false; }
  void init(const char *options = 0);
  void fini();

  void log(const TitanLoggerApi::TitanLogEvent& event, bool log_buffered,
           bool separate_file, bool use_emergency_mask);
  void set_parameter(const char *parameter_name, const char *parameter_value);
  // do not implement ILoggerPlugin::set_file_name();
  // it gets a filename skeleton and can't expand it.

  virtual void open_file(bool /*is_first*/);
  virtual void close_file();

  enum xml_escape_char_t { LT=0x01, GT=0x02, QUOT=0x04, APOS=0x08, AMP=0x10 };
  CHARSTRING escape_xml(const CHARSTRING& xml_str, int escape_chars);
  CHARSTRING escape_xml_attribute(const CHARSTRING& attr_str) { return escape_xml(attr_str, QUOT|AMP); }
  CHARSTRING escape_xml_element(const CHARSTRING& elem_str) { return escape_xml(elem_str, LT|AMP); }
  CHARSTRING escape_xml_comment(const CHARSTRING& comm_str) { return escape_xml(comm_str, AMP); /* FIXME: --> should be escaped too */ }

private:
  // parameters
  char *filename_stem_;
  char *testsuite_name_;
  // working values
  char *filename_;
  FILE *file_stream_;
};

#endif  // JUnitLogger_HH
