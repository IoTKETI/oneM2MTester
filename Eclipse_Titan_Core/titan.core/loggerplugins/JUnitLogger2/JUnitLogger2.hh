/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Susanszky, Eszter
 *
 ******************************************************************************/
#ifndef JUnitLogger_HH2
#define JUnitLogger_HH2

namespace TitanLoggerApi { class TitanLogEvent; }

#ifndef TITAN_RUNTIME_2
#include "RT1/TitanLoggerApi.hh"
#else
#include "RT2/TitanLoggerApi.hh"
#endif

#include "ILoggerPlugin.hh"
#include <stdio.h>
#include <string>
#include <vector>

struct TestCase{
	typedef enum {
		Pass,
		Inconc,
		Fail,
		Error,
		None,
		Unbound
	} Verdict;
	
	Verdict verdict;
	std::string tc_name;
	std::string module_name;
	std::string reason;
	std::string dte_reason;
	std::string stack_trace;
	long long tc_start;
	double time;
	
	TestCase():tc_name(""), module_name(""), reason(""), dte_reason(""), stack_trace(""), tc_start(0), time(0.0){}
	
	void writeTestCase(FILE* file_stream_) const;
	void setTCVerdict(const TitanLoggerApi::TitanLogEvent& event);
	void addStackTrace(const TitanLoggerApi::TitanLogEvent& event);
  void reset() {
    tc_name = "";
    module_name = ""; 
    reason = "";
    dte_reason = "";
    stack_trace = "";
    tc_start = 0;
    time = 0.0;
  }
};


struct TestSuite {
	typedef std::vector<TestCase*> TestCases;
	
//  TitanLoggerApi::TimestampType timestamp; TODO
  std::string ts_name;
	int all;
	int skipped;
	int failed;
	int error;
	int inconc;
	TestCases testcases;
  time_t start_ts;
  time_t end_ts;

	TestSuite():ts_name(""), all(0), skipped(0), failed(0), error(0), inconc(0) {}
	~TestSuite();
	
  void addTestCase(const TestCase& element);
	void write(FILE* file_stream_);
	
};

class JUnitLogger2: public ILoggerPlugin
{
public:
  JUnitLogger2();
  virtual ~JUnitLogger2();
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
  TestSuite testsuite;
  TestCase testcase;
  std::string dte_reason;
  
  FILE *file_stream_;
};

#endif  // JUnitLogger_HH2
