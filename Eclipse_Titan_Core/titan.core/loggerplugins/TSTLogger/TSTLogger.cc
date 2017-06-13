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
 *   Pilisi, Gergely
 *
 ******************************************************************************/
#include "ILoggerPlugin.hh"
#include "TSTLogger.hh"

#ifndef TITAN_RUNTIME_2
#include "RT1/TitanLoggerApi.hh"
#else
#include "RT2/TitanLoggerApi.hh"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include <iostream>
#include <sstream>

#ifdef __cplusplus
extern "C"
{
  ILoggerPlugin *create_plugin() { return new TSTLogger(); }
  void destroy_plugin(ILoggerPlugin *plugin) { delete plugin; }
}
#endif

using namespace std;

////////////////////////////////////////////////////////////////////////////////

class SocketException
{
  string error_msg;
  string reason;
public:
  SocketException(const string p_error_msg, const string p_reason=""):error_msg(p_error_msg),reason(p_reason) {}
  const string getMessage() const { return error_msg; }
  const string getReason() const { return reason; }
};

class TimeoutException : public SocketException
{
public:
  TimeoutException(const string p_error_msg): SocketException(p_error_msg) {}
};

////////////////////////////////////////////////////////////////////////////////

class TCPClient
{
  int socket_fd;
  time_t timeout_time;

  enum select_readwrite_t { SELECT_READ, SELECT_WRITE };
  void wait_for_ready(time_t end_time, select_readwrite_t readwrite);
public:
  TCPClient(): socket_fd(-1), timeout_time(30) {}
  // opens connection and returns socket file descriptor, throws exception on error
  void open_connection(const string host_name, const string service_name) throw(SocketException);
  // send a string to a socket, don't return until the whole string is sent
  void send_string(const string& str) throw(SocketException);
  // receive available data to the end of the parameter string, blocks until at least wait_for_bytes chars have arrived,
  // returns false if connection was closed
  bool receive_string(string& str, const size_t wait_for_bytes) throw(SocketException);
  // close connection
  void close_connection() throw(SocketException);
  time_t get_timeout() const { return timeout_time; }
  void set_timeout(time_t t) { timeout_time = t; }
};

void TCPClient::open_connection(const string host_name, const string service_name) throw(SocketException)
{
  if (socket_fd!=-1) {
    close_connection();
  }
  struct addrinfo *res, *aip;
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;
  int status = getaddrinfo(host_name.c_str(), service_name.c_str(), &hints, &res);
  if (status!=0) {
    throw SocketException("Cannot find host and service", gai_strerror(status));
  }
  for (aip = res; aip != NULL; aip = aip->ai_next) {
    // create socket descriptor
    socket_fd = socket(aip->ai_family, aip->ai_socktype, aip->ai_protocol);
    if (socket_fd==-1) continue;
    // try to connect
    status = connect(socket_fd, aip->ai_addr, aip->ai_addrlen);
    if (status==0) break; // connected
  }
  freeaddrinfo(res);
  if (aip==NULL) {
    socket_fd = -1;
    throw SocketException("Cannot connect to host and service");
  }
}

void TCPClient::close_connection() throw(SocketException)
{
  if (socket_fd==-1) {
    return;
  }
  int rv = close(socket_fd);
  socket_fd = -1;
  if (rv!=0) {
    throw SocketException("Cannot close socket", strerror(errno));
  }
}

// end_time - is time in seconds (from time(NULL)+timeout, when should we give up waiting
// readwrite - are we checking for read or write readyness on the file descriptor
void TCPClient::wait_for_ready(time_t end_time, select_readwrite_t readwrite)
{
  struct timeval tv;
  tv.tv_sec = end_time - time(NULL);
  tv.tv_usec = 0;
  fd_set fds;
wait_reset:
  FD_ZERO(&fds);
  FD_SET(socket_fd, &fds);
  int status = select(socket_fd+1,
                (readwrite==SELECT_READ)  ? &fds : NULL, 
                (readwrite==SELECT_WRITE) ? &fds : NULL,
                NULL, &tv);
  if (status==-1) {
    if (errno==EINTR) { // some signal
      // update time
      tv.tv_sec = end_time - time(NULL);
      tv.tv_usec = 0;
      goto wait_reset; // wait again
    }
    throw SocketException("Error while waiting on socket", strerror(errno));
  }
  if (FD_ISSET(socket_fd, &fds)==0) {
    throw TimeoutException("Timeout while waiting on socket");
  }
}

void TCPClient::send_string(const string& str) throw(SocketException)
{
  if (socket_fd==-1) {
    throw SocketException("Connection is not open");
  }
  time_t end_time = time(NULL) + timeout_time;
  size_t sent_cnt = 0;
  size_t str_len = str.size();
  while (sent_cnt<str_len) {
    wait_for_ready(end_time, SELECT_WRITE);
    ssize_t n = send(socket_fd, str.c_str()+sent_cnt, str_len-sent_cnt, 0);
    if (n==-1) {
      throw SocketException("Cannot send data on socket", strerror(errno));
    }
    sent_cnt += n;
  }
}

// wait_for_bytes - wait until at least so many bytes arrived or timeout or connection closed,
//                  if zero then wait until connection is closed
bool TCPClient::receive_string(string& str, const size_t wait_for_bytes) throw(SocketException)
{
  if (socket_fd==-1) {
    throw SocketException("Connection is not open");
  }
  time_t end_time = time(NULL) + timeout_time;
  bool open = true;
  char buff[1024];
  size_t bytes_received = 0;
  while (wait_for_bytes==0 || bytes_received<wait_for_bytes) {
    wait_for_ready(end_time, SELECT_READ);
    ssize_t recv_cnt = recv(socket_fd, buff, sizeof(buff), 0);
    if (recv_cnt==-1) {
        throw SocketException("Cannot read data from socket", strerror(errno));
    }
    if (recv_cnt==0) { // socket was closed by the other side
      open = false;
      close_connection();
      break;
    }
    bytes_received += recv_cnt;
    // store received data
    str.append(buff, recv_cnt);
  }
  return open;
}

////////////////////////////////////////////////////////////////////////////////

class HttpException : public SocketException
{
public:
  HttpException(const string p_error_msg, const string p_reason=""):
    SocketException(p_error_msg, p_reason) {}
};

typedef map<string,string> string_map;

class HTTPClient : public TCPClient
{
public:
  HTTPClient(): TCPClient() {}
  string url_encode(const string& str);
  string post_request(const string& host, const string& uri, const string& user_agent, const string_map& req_params) throw(SocketException);
};

string HTTPClient::url_encode(const string& str)
{
  static char hex[] = "0123456789abcdef";
  stringstream ss;
  for (size_t i=0; i<str.size(); i++) {
    char c = str[i];
    if (isalnum(c) || c=='-' || c=='_' || c=='.' || c=='~') {
      ss << c;
    } else if (c==' ') {
      ss << '+';
    } else {
      ss << '%' << hex[(c>>4) & 15] << hex[(c&15) & 15];
    }
  }
  return ss.str();
}

string HTTPClient::post_request(const string& host, const string& uri, const string& user_agent, const string_map& req_params) throw(SocketException)
{
  // compose request message
  stringstream req_ss;
  req_ss << "POST " << uri << " HTTP/1.1\r\n" << // Host: must be added if HTTP/1.1 is used
    "Host: " << host << "\r\n" <<
    "User-Agent: " << user_agent << "\r\n" <<
    "Connection: Close" << "\r\n" <<
    "Content-Type: application/x-www-form-urlencoded" << "\r\n"; // as if it were post'ed from a web browser html form
  stringstream req_body_ss;
  for (string_map::const_iterator it = req_params.begin(); it!=req_params.end(); ++it) {
    if (it!=req_params.begin()) req_body_ss << '&';
    req_body_ss << url_encode(it->first) << '=' << url_encode(it->second);
  }
  req_ss << "Content-Length: " << req_body_ss.str().size() << "\r\n";
  req_ss << "\r\n" << req_body_ss.str();
  // send request
  //cerr << "HTTP POST REQUEST:\n[" << req_ss.str() << "]\n";
  send_string(req_ss.str());
  // receive response, wait until the server closes the connection (doesn't work with Keep-Alive!)
  string response;
  receive_string(response, 0); // receive until connection is closed by the peer or timeout
  //cerr << "HTTP POST RESPONSE:\n[" << response << "]\n";
  // divide into header and body parts
  size_t pos = response.find("\r\n\r\n");
  if (pos==string::npos) {
    throw HttpException("Invalid HTTP response", "Cannot find body part");
  }
  string head = response.substr(0, pos);
  string body = response.substr(pos+4, string::npos);
  if (head.find("Transfer-Encoding: chunked")!=string::npos) {
    // remove chunked encoding stuff from body
    // FIXME: this doesn't work if the chunks contain \r\n
    string real_body;
    string line;
    bool chunk_line = false;
    for (size_t idx = 0; idx<body.size()-1; idx++) {
      if (body[idx]=='\r' && body[idx+1]=='\n') { // end of line
        if (chunk_line) {
          real_body += line;
        } else {
          if (line=="0") {
            break;
          }
        }
        chunk_line = !chunk_line;
        line = "";
        idx++; 
      } else {
        line += body[idx];
      }
    }
    body = real_body;
  }
  return body;
}

////////////////////////////////////////////////////////////////////////////////


string TSTLogger::get_tst_time_str(const TitanLoggerApi::TimestampType& timestamp)
{
  long long int t = timestamp.seconds().get_long_long_val() * 1000 + timestamp.microSeconds().get_long_long_val() / 1000;
  stringstream s;
  s << (long int)t;
  return s.str();
}

string TSTLogger::get_host_name()
{
  char name[257];
  int status = gethostname(name, 256);
  if (status!=0) return "DefaultExecutingHost";
  return name;
}

string TSTLogger::get_user_name()
{
  return getlogin();
}

TSTLogger::TSTLogger()
{
  this->major_version_ = 1;
  this->minor_version_ = 0;
  this->name_ = mputstr(this->name_, "TSTLogger");
  this->help_ = mputstr(this->help_, "TITAN Logger Plugin for TestStatistics");

  // parameters of this plugin
  parameters["tst_host_name"] = ParameterData("eta-teststatistics.rnd.ki.sw.ericsson.se", true, "TestStatistics web service host name");
  parameters["tst_service_name"] = ParameterData("http", true, "TestStatistics web service name or port number");

  parameters["tst_tcstart_url"] = ParameterData("/ts-rip/rip/tcstart");
  parameters["tst_tcstop_url"] = ParameterData("/ts-rip/rip/tcstop");
  parameters["tst_tsstart_url"] = ParameterData("/ts-rip/rip/tsstart");
  parameters["tst_tsstop_url"] = ParameterData("/ts-rip/rip/tsstop");
  parameters["tst_tcfailreason_url"] = ParameterData("/ts-rip/rip/tcfailreason");

  parameters["dbsUrl"] = ParameterData("esekilx0007-sql5.rnd.ki.sw.ericsson.se:3314", true, "database URL");
  parameters["dbUser"] = ParameterData("demo", true, "database user");
  parameters["dbPass"] = ParameterData("demo", true, "plain text password of the user");
  parameters["dbName"] = ParameterData("teststatistics_demo", true, "name of the database");

  parameters["log_plugin_debug"] = ParameterData("0");

  parameters["testConfigName"] = ParameterData("DefaultConfigName", false, "name of this specific configuration of the test suite");
  parameters["suiteName"] = ParameterData("DefaultSuiteName", false, "name of test suite");
  parameters["executingHost"] = ParameterData(get_host_name(), true, "host where the test was executed");
  parameters["sutId"] = ParameterData("0.0.0.0", true, "IP address of SUT");
  parameters["sutName"] = ParameterData("DefaultSUTName", true, "name of SUT");
  parameters["lsvMajor"] = ParameterData("1", true, "major version number of SUT");
  parameters["lsvMinor"] = ParameterData("0", true, "minor version number of SUT");
  parameters["runByUser"] = ParameterData(get_user_name(), true, "name of user running the tests");
  parameters["projectName"] = ParameterData("DefaultProjectname", true, "name of the project");
  parameters["productName"] = ParameterData("DefaultProductName", true, "name of the product");
  parameters["productVersion"] = ParameterData("0.0", true, "version of the product");
  parameters["configType"] = ParameterData("configType", true, "");
  parameters["configVersion"] = ParameterData("configVersion", true, "");
  parameters["testType"] = ParameterData("testType", true, "");
  parameters["logLink"] = ParameterData("default_log_location", false, "absolute location of log files");
  parameters["logEnd"] = ParameterData("default_web_log_dir", false, "log directory relative to web server root");
  parameters["reportEmail"] = ParameterData(get_user_name()+"@ericsson.com", false, "who is to be notified via email");
  parameters["reportTelnum"] = ParameterData("0", false, "where to send the SMS notification");

  stringstream ss;
  ss << "TITAN " << this->name_ << ' ' << this->major_version_ << '.' << this->minor_version_;
  user_agent = ss.str();

  this->testcase_count_ = 0;
}

TSTLogger::~TSTLogger()
{
  Free(this->name_);
  Free(this->help_);
  this->name_ = this->help_ = NULL;
}

bool TSTLogger::log_plugin_debug()
{
  return parameters["log_plugin_debug"].get_value()!="0";
}

void TSTLogger::init(const char */*options*/)
{
  cout << "Initializing `" << this->name_ << "' (v" << this->major_version_ << "." << this->minor_version_ << "): " << this->help_ << endl;
  this->is_configured_ = true;
}

void TSTLogger::fini()
{
  if (is_main_proc()) {
    TitanLoggerApi::TimestampType timestamp;
    struct timeval tm;
    gettimeofday(&tm, NULL);
    timestamp.seconds().set_long_long_val((long long int)tm.tv_sec);
    timestamp.microSeconds().set_long_long_val((long long int)tm.tv_usec);
    log_testsuite_stop(timestamp); // TODO: call this from log()
  }
  this->is_configured_ = false;
}

void TSTLogger::set_parameter(const char *parameter_name, const char *parameter_value)
{
  map<string,ParameterData>::iterator it = parameters.find(parameter_name);
  if (it!=parameters.end()) {
    it->second.set_value(parameter_value);
  } else {
    cerr << this->name_ << ": " << "Unsupported parameter: `" << parameter_name << "' with value: `"
         << parameter_value << "'" << endl;
  }
}

void TSTLogger::add_database_params(std::map<std::string,std::string>& req_params)
{
  req_params["dbsUrl"] = parameters["dbsUrl"].get_value();
  req_params["dbUser"] = parameters["dbUser"].get_value();
  req_params["dbPass"] = parameters["dbPass"].get_value();
  req_params["dbName"] = parameters["dbName"].get_value();
}

string TSTLogger::post_message(std::map<std::string,std::string> req_params, const string& TST_service_uri)
{
  add_database_params(req_params);
  try {
    HTTPClient client;
    client.open_connection(parameters["tst_host_name"].get_value(), parameters["tst_service_name"].get_value());
    string response = client.post_request(parameters["tst_host_name"].get_value(), TST_service_uri, user_agent, req_params);
    client.close_connection();
    return response;
  }
  catch (SocketException exc) {
    cerr << this->name_ << ": " << "HTTP error: " << exc.getMessage() << " (" << exc.getReason() << ")\n";
  }
  return "";
}

// most of the message types should be sent only from one main process (MTC in parallel mode)
bool TSTLogger::is_main_proc() const
{
  return TTCN_Runtime::is_mtc() || TTCN_Runtime::is_single();
}

void TSTLogger::log(const TitanLoggerApi::TitanLogEvent& event, bool log_buffered)
{
  log(event, log_buffered, false, false);
}

void TSTLogger::log(const TitanLoggerApi::TitanLogEvent& event,
  bool /*log_buffered*/, bool /*separate_file*/, bool /*use_emergency_mask*/)
{
  const TitanLoggerApi::LogEventType_choice& choice = event.logEvent().choice();
  switch (choice.get_selection()) {
  case TitanLoggerApi::LogEventType_choice::ALT_testcaseOp: {
    const TitanLoggerApi::TestcaseEvent_choice& tec = choice.testcaseOp().choice();
    switch (tec.get_selection()) {
    case TitanLoggerApi::TestcaseEvent_choice::ALT_testcaseStarted:
      if (is_main_proc()) {
        log_testcase_start(tec.testcaseStarted(), event.timestamp());
      }
      break;
    case TitanLoggerApi::TestcaseEvent_choice::ALT_testcaseFinished:
      if (is_main_proc()) {
        log_testcase_stop(tec.testcaseFinished(), event.timestamp());
      }
      break;
    default:
      break;
    }
    break; }
  case TitanLoggerApi::LogEventType_choice::ALT_verdictOp:
    // allow these messages from PTCs
    log_verdictop_reason(choice.verdictOp());
    break;
  default:
    break;
  }
}

void TSTLogger::log_testcase_start(const TitanLoggerApi::QualifiedName& testcaseStarted, const TitanLoggerApi::TimestampType& timestamp)
{
  if (this->testcase_count_ == 0) {
    log_testsuite_start(timestamp); // TODO: call this from log()
  }
  ++this->testcase_count_;

  map<string,string> req_params;
  req_params["suiteId"] = this->suite_id_;
  req_params["tcId"] = (const char *)testcaseStarted.testcase__name();
  req_params["tcHeader"] = req_params["tcId"];
  req_params["tcStartTime"] = get_tst_time_str(timestamp);
  req_params["tcState"] = "0";
  //req_params["tcHtml"] = "";
  req_params["tcClass"] = (const char *)testcaseStarted.module__name();
  req_params["tcMethod"] = req_params["tcId"];

  string resp = post_message(req_params, parameters["tst_tcstart_url"].get_value());

  if ((resp.find("done") != string::npos) && (resp.find("tcaseId") != string::npos)) {
    this->tcase_id_ = resp.substr(resp.find("=") + 1);
    if (log_plugin_debug()) {
      cout << this->name_ << ": " << "Operation `log_testcase_start' successful, returned tcaseId=" << this->tcase_id_ << endl;
    }    
  } else {
    cerr << this->name_ << ": " << "Operation `log_testcase_start' failed: " << resp << endl;
    // Add better error handling.
    return;
  }
}

void TSTLogger::log_testcase_stop(const TitanLoggerApi::TestcaseType& testcaseFinished, const TitanLoggerApi::TimestampType& timestamp)
{
  string tc_state;
  switch (testcaseFinished.verdict()) {
  case TitanLoggerApi::Verdict::v0none: // start (?)
    tc_state = "0";
    break;
  case TitanLoggerApi::Verdict::v1pass: // pass
    tc_state = "1";
    break;
  case TitanLoggerApi::Verdict::v2inconc: // inconclusive
    tc_state = "7";
    break;
  case TitanLoggerApi::Verdict::v3fail: // fail
    tc_state = "2";
    break;
  case TitanLoggerApi::Verdict::v4error: // error
    tc_state = "3";
    break;
  default:
    tc_state = "0";
  }
  map<string,string> req_params;
  req_params["tcaseId"] = this->tcase_id_;
  req_params["tcEndTime"] = get_tst_time_str(timestamp);
  req_params["tcState"] = tc_state;
  req_params["tcUndefined"] = "false"; //?
  req_params["tcAssertion"] = "false"; //?
  req_params["tcTrafficLoss"] = "false"; //?

  string resp = post_message(req_params, parameters["tst_tcstop_url"].get_value());

  if (!resp.compare("done")) {
    if (log_plugin_debug()) {
      cout << this->name_ << ": " << "Operation `log_testcase_stop' successful" << endl;
    }
  } else {
    cerr << this->name_ << ": " << "Operation `log_testcase_stop' failed: " << resp << endl;
    return;
  }
}

void TSTLogger::log_testsuite_start(const TitanLoggerApi::TimestampType& timestamp)
{
  map<string,string> req_params;
  req_params["fwName"] = "TITAN";
  stringstream titanver_ss;
  titanver_ss << TTCN3_MAJOR << '.' << TTCN3_MINOR << ".pl" << TTCN3_PATCHLEVEL;
  req_params["fwVersion"] = titanver_ss.str();
  req_params["suiteStartTime"] = get_tst_time_str(timestamp);
  req_params["testConfigName"] = parameters["testConfigName"].get_value();
  req_params["suiteName"] = parameters["suiteName"].get_value();
  req_params["executingHost"] = parameters["executingHost"].get_value();
  req_params["sutId"] = parameters["sutId"].get_value();
  req_params["sutName"] = parameters["sutName"].get_value();
  req_params["lsvMajor"] = parameters["lsvMajor"].get_value();
  req_params["lsvMinor"] = parameters["lsvMinor"].get_value();
  req_params["runByUser"] = parameters["runByUser"].get_value();
  req_params["projectName"] = parameters["projectName"].get_value();
  req_params["productName"] = parameters["productName"].get_value();
  req_params["productVersion"] = parameters["productVersion"].get_value();
  req_params["notRun"] = "1"; // FIXME?
  req_params["configType"] = parameters["configType"].get_value();
  req_params["configVersion"] = parameters["configVersion"].get_value();
  req_params["testType"] = parameters["testType"].get_value();
  req_params["logLink"] = parameters["logLink"].get_value();
  req_params["logEnd"] = parameters["logEnd"].get_value();
  req_params["reportEmail"] = parameters["reportEmail"].get_value();
  req_params["reportTelnum"] = parameters["reportTelnum"].get_value();

  string resp = post_message(req_params, parameters["tst_tsstart_url"].get_value());

  if ((resp.find("done") != string::npos) && (resp.find("suiteId") != string::npos)) {
    this->suite_id_ = resp.substr(resp.find("=") + 1);
    if (log_plugin_debug()) {
      cout << this->name_ << ": " << "Operation `log_testsuite_start' successful, returned suiteId=" << this->suite_id_ << endl;    
    }
  } else {
    cerr << this->name_ << ": " << "Operation `log_testsuite_start' failed: " << resp << endl;
    return;
  }
}

void TSTLogger::log_testsuite_stop(const TitanLoggerApi::TimestampType& timestamp)
{
  map<string,string> req_params;
  req_params["suiteId"] = this->suite_id_;
  req_params["tsEndTime"] = get_tst_time_str(timestamp);
  req_params["reportEmail"] = parameters["reportEmail"].get_value();
  req_params["reportTelnum"] = parameters["reportTelnum"].get_value();

  string resp = post_message(req_params, parameters["tst_tsstop_url"].get_value());

  if (!resp.compare("done")) {
    if (log_plugin_debug()) {
      cout << this->name_ << ": " << "Operation `log_testsuite_stop' successful" << endl;
    }
  } else {
    cerr << this->name_ << ": " << "Operation `log_testsuite_stop' failed: " << resp << endl;
  }
}

void TSTLogger::log_verdictop_reason(const TitanLoggerApi::VerdictOp& verdictOp)
{
  if (verdictOp.choice().get_selection() == TitanLoggerApi::VerdictOp_choice::ALT_setVerdict) {
    TitanLoggerApi::SetVerdictType setVerdict = verdictOp.choice().setVerdict();
    if (setVerdict.newReason().ispresent() && setVerdict.newReason()().lengthof()>0) {
      map<string,string> req_params;
      req_params["tcaseId"] = this->tcase_id_;
      req_params["tcFailType"] = "0"; //?
      req_params["tcFailNum"] = "1";
      req_params["tcFailReason"] = (const char *)setVerdict.newReason()();
      string resp = post_message(req_params, parameters["tst_tcfailreason_url"].get_value());
      if (!resp.compare("done")) {
        if (log_plugin_debug()) {
          cout << this->name_ << ": " << "Operation log_verdictop_reason' successful" << endl;
        }
      } else {
        cerr << this->name_ << ": " << "Operation log_verdictop_reason' failed: " << resp << endl;
      }
    }
  }
}
