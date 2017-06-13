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
#include "JUnitLogger2.hh"

#include <unistd.h>
#include <sys/types.h>

#include <sys/time.h>

extern "C"
{
  // It's a static plug-in, destruction is done in the destructor.  We need
  // `extern "C"' for some reason.
  ILoggerPlugin *create_junit_logger() { return new JUnitLogger2(); }
}

extern "C" {
  ILoggerPlugin *create_plugin() { return new JUnitLogger2(); }
  void destroy_plugin(ILoggerPlugin *plugin) { delete plugin; }
}

TestSuite::~TestSuite()
{  
  for (TestCases::const_iterator it = testcases.begin(); it != testcases.end(); ++it) {
	  delete (*it);
	}
}

JUnitLogger2::JUnitLogger2()
: filename_stem_(NULL), testsuite_name_(mcopystr("Titan")), filename_(NULL), file_stream_(NULL)
{
  // Overwrite values set by the base class constructor
  
  fprintf(stderr, "construct junitlogger\n");
  major_version_ = 2;
  minor_version_ = 0;
  name_ = mcopystr("JUnitLogger");
  help_ = mcopystr("JUnitLogger writes JUnit-compatible XML");
  dte_reason = "";
  //printf("%5lu:constructed\n", (unsigned long)getpid());
}

JUnitLogger2::~JUnitLogger2()
{
  //printf("%5lu:desstructed\n", (unsigned long)getpid());
  close_file();

  Free(name_);
  Free(help_);
  Free(filename_);
  Free(testsuite_name_);
  Free(filename_stem_);
  name_ = help_ = filename_ = filename_stem_ = NULL;
  file_stream_ = NULL;
}

void JUnitLogger2::init(const char */*options*/)
{
//printf("%5lu:init\n", (unsigned long)getpid());
  fprintf(stderr, "Initializing `%s' (v%u.%u): %s\n", name_, major_version_, minor_version_, help_);
}

void JUnitLogger2::fini()
{
  //puts("fini");
  //fprintf(stderr, "JUnitLogger2 finished logging for PID: `%lu'\n", (unsigned long)getpid());
}

void JUnitLogger2::set_parameter(const char *parameter_name, const char *parameter_value) {
//puts("set_par");
  if (!strcmp("filename_stem", parameter_name)) {
    if (filename_stem_ != NULL)
      Free(filename_stem_);
    filename_stem_ = mcopystr(parameter_value);
  } else if (!strcmp("testsuite_name", parameter_name)) {
    if (filename_stem_ != NULL)
      Free(testsuite_name_);
    testsuite_name_ = mcopystr(parameter_value);
  } else {
    fprintf(stderr, "Unsupported parameter: `%s' with value: `%s'\n",
      parameter_name, parameter_value);
  }
}

void JUnitLogger2::open_file(bool is_first) {
  if (is_first) {
    if (filename_stem_ == NULL) {
      filename_stem_ = mcopystr("junit-xml");
    }
  }

  if (file_stream_ != NULL) return; // already open

  if (!TTCN_Runtime::is_single() && !TTCN_Runtime::is_mtc()) return; // don't bother, only MTC has testcases

  filename_ = mprintf("%s-%lu.log", filename_stem_, (unsigned long)getpid());

  file_stream_ = fopen(filename_, "w");
  if (!file_stream_) {
    fprintf(stderr, "%s was unable to open log file `%s', reinitialization "
      "may help\n", plugin_name(), filename_);
    return;
  }

  is_configured_ = true;
  time(&(testsuite.start_ts));
  testsuite.ts_name = testsuite_name_;
}

void JUnitLogger2::close_file() {  
  if (file_stream_ != NULL) {
    time(&(testsuite.end_ts));
    testsuite.write(file_stream_);
    fclose(file_stream_);
    file_stream_ = NULL;
  }
  if (filename_) {
    Free(filename_);
    filename_ = NULL;
  }
}

void JUnitLogger2::log(const TitanLoggerApi::TitanLogEvent& event,
  bool /*log_buffered*/, bool /*separate_file*/,
  bool /*use_emergency_mask*/)
{
  if (file_stream_ == NULL) return;

  const TitanLoggerApi::LogEventType_choice& choice = event.logEvent().choice();
  
  switch (choice.get_selection()) {
    case TitanLoggerApi::LogEventType_choice::ALT_testcaseOp: {
      const TitanLoggerApi::TestcaseEvent_choice& tcev = choice.testcaseOp().choice();  
    
      switch (tcev.get_selection()) {
        case TitanLoggerApi::TestcaseEvent_choice::ALT_testcaseStarted: {
          testcase.tc_name = tcev.testcaseStarted().testcase__name();
          // remember the start time
          testcase.tc_start = 1000000LL * (long long)event.timestamp().seconds() + (long long)event.timestamp().microSeconds();
          break; }

        case TitanLoggerApi::TestcaseEvent_choice::ALT_testcaseFinished: {
          const TitanLoggerApi::TestcaseType& tct = tcev.testcaseFinished();
          testcase.reason = escape_xml_element(tct.reason());
          testcase.module_name = tct.name().module__name();
          
          const TitanLoggerApi::TimestampType& ts = event.timestamp();
          long long tc_end = 1000000LL * (long long)ts.seconds() + (long long)ts.microSeconds();
          testcase.time = (tc_end - testcase.tc_start) / 1000000.0;

          testcase.setTCVerdict(event);
          testcase.dte_reason = dte_reason.data();
          dte_reason = "";
          testsuite.addTestCase(testcase);
          testcase.reset();
          break; }

        case TitanLoggerApi::TestcaseEvent_choice::UNBOUND_VALUE:
          testcase.verdict = TestCase::Unbound;
          break; } // switch testcaseOp().choice.get_selection()

      break; } // testcaseOp
      
    case TitanLoggerApi::LogEventType_choice::ALT_errorLog: {// A DTE is about to be thrown
      const TitanLoggerApi::Categorized& cat = choice.errorLog();
      dte_reason = escape_xml_element(cat.text());
      break; }
   
    default: break;
  } // switch event.logEvent().choice().get_selection()

  fflush(file_stream_);
}

CHARSTRING JUnitLogger2::escape_xml(const CHARSTRING& xml_str, int escape_chars) {
  expstring_t escaped = NULL;
  int len = xml_str.lengthof();
  for (int i=0; i<len; i++) {
    char c = *(((const char*)xml_str)+i);
    switch (c) {
    case '<':
      if (escape_chars&LT) escaped = mputstr(escaped, "&lt;");
      else escaped = mputc(escaped, c);
      break;
    case '>':
      if (escape_chars&GT) escaped = mputstr(escaped, "&gt;");
      else escaped = mputc(escaped, c);
      break;
    case '"':
      if (escape_chars&QUOT) escaped = mputstr(escaped, "&quot;");
      else escaped = mputc(escaped, c);
      break;
    case '\'':
      if (escape_chars&APOS) escaped = mputstr(escaped, "&apos;");
      else escaped = mputc(escaped, c);
      break;
    case '&':
      if (escape_chars&AMP) escaped = mputstr(escaped, "&amp;");
      else escaped = mputc(escaped, c);
      break;
    default:
      escaped = mputc(escaped, c);
    }
  }
  CHARSTRING ret_val(escaped);
  Free(escaped);
  return ret_val;
}

void TestCase::writeTestCase(FILE* file_stream_) const{
  switch(verdict){
    case None: {
      fprintf(file_stream_, "  <testcase classname='%s' name='%s' time='%f'>\n", module_name.data(), tc_name.data(), time);
      fprintf(file_stream_, "    <skipped>no verdict</skipped>\n");
      fprintf(file_stream_, "  </testcase>\n");
      break; }
    case Fail:  {
      fprintf(file_stream_, "  <testcase classname='%s' name='%s' time='%f'>\n", module_name.data(), tc_name.data(), time);
      fprintf(file_stream_, "    <failure type='fail-verdict'>%s\n", reason.data());
      fprintf(file_stream_, "%s\n", stack_trace.data());
      fprintf(file_stream_, "    </failure>\n");
      fprintf(file_stream_, "  </testcase>\n");
      break; }
    case Error:  {
      fprintf(file_stream_, "  <testcase classname='%s' name='%s' time='%f'>\n", module_name.data(), tc_name.data(), time);
      fprintf(file_stream_, "    <error type='DTE'>%s</error>\n", dte_reason.data());
      fprintf(file_stream_, "  </testcase>\n");
      break; }
    default: 
      fprintf(file_stream_, "  <testcase classname='%s' name='%s' time='%f'/>\n", module_name.data(), tc_name.data(), time);
      break;
  }
  fflush(file_stream_);
}

void TestSuite::addTestCase(const TestCase& testcase) {
	testcases.push_back(new TestCase(testcase));
	all++;
	switch(testcase.verdict) {
    case TestCase::Fail: failed++; break;
    case TestCase::None: skipped++; break;
    case TestCase::Error: error++; break;
    case TestCase::Inconc: inconc++; break;
    default: break;
	}
}

void TestSuite::write(FILE* file_stream_) {
  double difftime_ = difftime(end_ts, start_ts);
  fprintf(file_stream_, "<?xml version=\"1.0\"?>\n"
    "<testsuite name='%s' tests='%d' failures='%d' errors='%d' skipped='%d' inconc='%d' time='%.2f'>\n",
    ts_name.data(), all, failed, error, skipped, inconc, difftime_);
  fflush(file_stream_);
  
  for (TestCases::const_iterator it = testcases.begin(); it != testcases.end(); ++it) {
	  (*it)->writeTestCase(file_stream_);
	}
	
  fprintf(file_stream_, "</testsuite>\n");
  fflush(file_stream_);
}

void TestCase::setTCVerdict(const TitanLoggerApi::TitanLogEvent& event){
  TitanLoggerApi::Verdict tc_verdict = event.logEvent().choice().testcaseOp().choice().testcaseFinished().verdict();
  switch (tc_verdict) {
    case TitanLoggerApi::Verdict::UNBOUND_VALUE:
    case TitanLoggerApi::Verdict::UNKNOWN_VALUE:
      // should not happen
      break;

    case TitanLoggerApi::Verdict::v0none:
      verdict = TestCase::None;
      break;

    case TitanLoggerApi::Verdict::v1pass:
      verdict = TestCase::Pass;
      break;

    case TitanLoggerApi::Verdict::v2inconc:
      verdict = TestCase::Inconc;
      break;

    case TitanLoggerApi::Verdict::v3fail: {
      verdict = TestCase::Fail;
      addStackTrace(event);
      break; }

    case TitanLoggerApi::Verdict::v4error:
      verdict = TestCase::Error;
      break;
  }
}

void TestCase::addStackTrace(const TitanLoggerApi::TitanLogEvent& event){
// Add a stack trace
  const TitanLoggerApi::TitanLogEvent_sourceInfo__list& stack = event.sourceInfo__list();
  
  int stack_depth = stack.size_of();
  for (int i=0; i < stack_depth; ++i) {
    const TitanLoggerApi::LocationInfo& location = stack[i];
    
    stack_trace += "      ";
    stack_trace += location.filename();
    stack_trace += ":";
    stack_trace += int2str(location.line());
    stack_trace += " ";
    stack_trace += location.ent__name();
    stack_trace += " ";
    
    // print the location type
    switch (location.ent__type()) {
    case TitanLoggerApi::LocationInfo_ent__type:: UNKNOWN_VALUE:
    case TitanLoggerApi::LocationInfo_ent__type:: UNBOUND_VALUE:
      // can't happen
      break;
    case TitanLoggerApi::LocationInfo_ent__type::unknown:
      // do nothing
      break;
    case TitanLoggerApi::LocationInfo_ent__type::controlpart:
      stack_trace += "control part";
      break;
    case TitanLoggerApi::LocationInfo_ent__type::testcase__:
      stack_trace += "testcase";
      break;
    case TitanLoggerApi::LocationInfo_ent__type::altstep__:
      stack_trace += "altstep";
      break;
    case TitanLoggerApi::LocationInfo_ent__type::function__:
      stack_trace += "function";
      break;
    case TitanLoggerApi::LocationInfo_ent__type::external__function:
      stack_trace += "external function";
      break;
    case TitanLoggerApi::LocationInfo_ent__type::template__:
      stack_trace += "template";
      break;
    }
    
    if(i<stack_depth-1) stack_trace += "\n";
  }
}
