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
 *
 ******************************************************************************/
#ifndef TSTLogger_HH
#define TSTLogger_HH

#include <string>
#include <map>

namespace TitanLoggerApi
{
  class TitanLogEvent;
  class TimestampType;
  class QualifiedName;
  class TestcaseType;
  class VerdictOp;
}

class ILoggerPlugin;

struct ParameterData
{
  std::string default_value;
  bool optional;
  std::string description;
  bool set;
  std::string value;
  ParameterData(): optional(false), set(false) {}
  ParameterData(const std::string& def_val, const bool opt=true, const std::string& descr=""):
    default_value(def_val), optional(opt), description(descr), set(false) {}
  void set_value(std::string v) { set=true; value=v; }
  std::string get_value() { return set ? value : default_value; }
};

class TSTLogger: public ILoggerPlugin
{
public:
  explicit TSTLogger();
  virtual ~TSTLogger();
  inline bool is_static() { return false; }
  void init(const char *options = NULL);
  void fini();

  void log(const TitanLoggerApi::TitanLogEvent& event, bool log_buffered);
  void log(const TitanLoggerApi::TitanLogEvent& event, bool log_buffered, bool separate_file, bool use_emergency_mask);
  void set_parameter(const char *parameter_name, const char *parameter_value);

private:
  std::string user_agent; // HTTP req. parameter value
  std::map<std::string,ParameterData> parameters; // plugin parameters

  std::string suite_id_;
  std::string tcase_id_;
  int testcase_count_;

  explicit TSTLogger(const TSTLogger&);
  TSTLogger& operator=(const TSTLogger&);

  bool log_plugin_debug();

  static std::string get_tst_time_str(const TitanLoggerApi::TimestampType& timestamp);
  static std::string get_host_name();
  static std::string get_user_name();

  void add_database_params(std::map<std::string,std::string>& req_params);
  std::string post_message(std::map<std::string,std::string> req_params, const std::string& TST_service_uri);

  bool is_main_proc() const;

  void log_testcase_start(const TitanLoggerApi::QualifiedName& testcaseStarted, const TitanLoggerApi::TimestampType& timestamp);
  void log_testcase_stop(const TitanLoggerApi::TestcaseType& testcaseFinished, const TitanLoggerApi::TimestampType& timestamp);
  void log_testsuite_stop(const TitanLoggerApi::TimestampType& timestamp);
  void log_testsuite_start(const TitanLoggerApi::TimestampType& timestamp);
  void log_verdictop_reason(const TitanLoggerApi::VerdictOp& verdictOp);
};

#endif  // TSTLogger_HH
