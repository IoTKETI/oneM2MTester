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
 *   Lovassy, Arpad
 *   Raduly, Csaba
 *
 ******************************************************************************/
#include "JUnitLogger.hh"

#ifndef TITAN_RUNTIME_2
#include "RT1/TitanLoggerApi.hh"
#else
#include "RT2/TitanLoggerApi.hh"
#endif

#include <unistd.h>
#include <sys/types.h>

#include <sys/time.h>

extern "C"
{
  // It's a static plug-in, destruction is done in the destructor.  We need
  // `extern "C"' for some reason.
  ILoggerPlugin *create_junit_logger() { return new JUnitLogger(); }
}

extern "C" {
  ILoggerPlugin *create_plugin() { return new JUnitLogger(); }
  void destroy_plugin(ILoggerPlugin *plugin) { delete plugin; }
}

JUnitLogger::JUnitLogger()
: filename_stem_(NULL), testsuite_name_(mcopystr("Titan"))
, filename_(NULL), file_stream_(NULL)
{
  // Overwrite values set by the base class constructor
  major_version_ = 1;
  minor_version_ = 0;
  name_ = mcopystr("JUnitLogger");
  help_ = mcopystr("JUnitLogger writes JUnit-compatible XML");
//printf("%5lu:constructed\n", (unsigned long)getpid());
}

JUnitLogger::~JUnitLogger()
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

void JUnitLogger::init(const char */*options*/)
{
//printf("%5lu:init\n", (unsigned long)getpid());
  fprintf(stderr, "Initializing `%s' (v%u.%u): %s\n", name_,
    major_version_, minor_version_, help_);
}

void JUnitLogger::fini()
{
//puts("fini");
  //fprintf(stderr, "JUnitLogger finished logging for PID: `%lu'\n", (unsigned long)getpid());
}

void JUnitLogger::set_parameter(
  const char *parameter_name, const char *parameter_value)
{
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

void JUnitLogger::open_file(bool is_first)
{
  if (is_first) {
    if (filename_stem_ == NULL) {
      filename_stem_ = mcopystr("junit-xml");
    }
  }

  if (file_stream_ != NULL) return; // already open

  if (!TTCN_Runtime::is_single()
  &&  !TTCN_Runtime::is_mtc()) return; // don't bother, only MTC has testcases

  filename_ = mprintf("%s-%lu.log", filename_stem_, (unsigned long)getpid());
//printf("\n%5lu:open  [%s]\n", (unsigned long)getpid(), filename_);

  file_stream_ = fopen(filename_, "w");
  if (!file_stream_) {
    fprintf(stderr, "%s was unable to open log file `%s', reinitialization "
      "may help\n", plugin_name(), filename_);
    return;
  }

  is_configured_ = true;

  fprintf(file_stream_,
    "<?xml version=\"1.0\"?>\n"
    "<testsuite name='%s'>"
    // We don't know the number of tests yet; leave out the tests=... attrib
    "<!-- logger name=\"%s\" version=\"v%u.%u\" -->\n",
    testsuite_name_,
    plugin_name(), major_version(), minor_version());
  fflush(file_stream_);
}

void JUnitLogger::close_file()
{
//printf("%5lu:close%c[%s]\n", (unsigned long)getpid(), file_stream_ ? ' ' : 'd', filename_ ? filename_ : "<none>");
  if (file_stream_ != NULL) {
    fputs("</testsuite>\n", file_stream_);
    fflush(file_stream_);
    fclose(file_stream_);
    file_stream_ = NULL;
  }
  if (filename_) {
    Free(filename_);
    filename_ = NULL;
  }
}

void JUnitLogger::log(const TitanLoggerApi::TitanLogEvent& event,
  bool /*log_buffered*/, bool /*separate_file*/,
  bool /*use_emergency_mask*/)
{
//puts("log");
  if (file_stream_ == NULL) return;
  static RInt seconds, microseconds;

  const TitanLoggerApi::LogEventType_choice& choice = event.logEvent().choice();

  switch (choice.get_selection()) {
  case TitanLoggerApi::LogEventType_choice::ALT_testcaseOp: {
    const TitanLoggerApi::TestcaseEvent_choice& tcev = choice.testcaseOp().choice();
    switch (tcev.get_selection()) {
    case TitanLoggerApi::TestcaseEvent_choice::ALT_testcaseStarted: {
      fprintf(file_stream_, "<!-- Testcase %s started -->\n",
        (const char*)tcev.testcaseStarted().testcase__name());
      const TitanLoggerApi::TimestampType& ts = event.timestamp();
      // remember the start time
      seconds      = ts.seconds();
      microseconds = ts.microSeconds();
      break; }

    case TitanLoggerApi::TestcaseEvent_choice::ALT_testcaseFinished: {
      const TitanLoggerApi::TestcaseType& tct = tcev.testcaseFinished();
      const TitanLoggerApi::TimestampType& ts = event.timestamp();
      long long now = 1000000LL * (long long)ts.seconds() + (long long)ts.microSeconds();
      long long then= 1000000LL * (long long)   seconds   + (long long)   microseconds  ;
      fprintf(file_stream_, "<!-- Testcase %s finished in %f, verdict: %s%s%s -->\n",
        (const char*)tct.name().testcase__name(),
        (now - then) / 1000000.0,
        verdict_name[tct.verdict()],
        (tct.reason().lengthof() > 0 ? ", reason: " : ""),
        (const char*)escape_xml_element(tct.reason()));

      fprintf(file_stream_, "  <testcase classname='%s' name='%s' time='%f'>\n",
        (const char*)tct.name().module__name(),
        (const char*)tct.name().testcase__name(),
        (now - then) / 1000000.0);

      switch (tct.verdict()) {
      case TitanLoggerApi::Verdict::UNBOUND_VALUE:
      case TitanLoggerApi::Verdict::UNKNOWN_VALUE:
        // should not happen
        break;

      case TitanLoggerApi::Verdict::v0none:
        fprintf(file_stream_, "    <skipped>no verdict</skipped>\n");
        break;

      case TitanLoggerApi::Verdict::v1pass:
        // do nothing; this counts as success
        break;

      case TitanLoggerApi::Verdict::v2inconc:
        // JUnit doesn't seem to have the concept of "inconclusive"
        // do nothing; this will appear as success
        break;

      case TitanLoggerApi::Verdict::v3fail: {
        fprintf(file_stream_, "    <failure type='fail-verdict'>%s\n",
          (const char*)escape_xml_element(tct.reason()));

        // Add a stack trace
        const TitanLoggerApi::TitanLogEvent_sourceInfo__list& stack =
          event.sourceInfo__list();
        int stack_depth = stack.size_of();
        for (int i=0; i < stack_depth; ++i) {
          const TitanLoggerApi::LocationInfo& location = stack[i];
          fprintf(file_stream_, "\n      %s:%d %s ",
            (const char*)location.filename(), (int)location.line(),
            (const char*)location.ent__name());

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
            fputs("control part", file_stream_);
            break;
          case TitanLoggerApi::LocationInfo_ent__type::testcase__:
            fputs("testcase", file_stream_);
            break;
          case TitanLoggerApi::LocationInfo_ent__type::altstep__:
            fputs("altstep", file_stream_);
            break;
          case TitanLoggerApi::LocationInfo_ent__type::function__:
            fputs("function", file_stream_);
            break;
          case TitanLoggerApi::LocationInfo_ent__type::external__function:
            fputs("external function", file_stream_);
            break;
          case TitanLoggerApi::LocationInfo_ent__type::template__:
            fputs("template", file_stream_);
            break;
          }
        }
        fputs("\n    </failure>\n", file_stream_);
        break; }

      case TitanLoggerApi::Verdict::v4error:
        fprintf(file_stream_, "    <error type='DTE'>%s</error>\n",
          (const char*)escape_xml_element(tct.reason()));
        break;
      }
      // error or skip based on verdict
      fputs("  </testcase>\n", file_stream_);
      break; }

    case TitanLoggerApi::TestcaseEvent_choice::UNBOUND_VALUE:
      fputs("<!-- Unbound testcaseOp.choice !! -->\n", file_stream_);
      break;
    } // switch testcaseOp().choice.get_selection()

    break; } // testcaseOp

  case TitanLoggerApi::LogEventType_choice::ALT_errorLog: {
    // A DTE is about to be thrown
    const TitanLoggerApi::Categorized& cat = choice.errorLog();
    fprintf(file_stream_, "    <error type='DTE'>%s</error>\n",
      (const char*)escape_xml_element(cat.text()));
    break; }

  default:
    break; // NOP
  } // switch event.logEvent().choice().get_selection()

  fflush(file_stream_);
}

CHARSTRING JUnitLogger::escape_xml(const CHARSTRING& xml_str, int escape_chars)
{
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
