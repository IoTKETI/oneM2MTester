/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Baji, Laszlo
 *   Balasko, Jeno
 *   Lovassy, Arpad
 *
 ******************************************************************************/
#include "jnimw.h"
#include "org_eclipse_titan_executor_jni_JNIMiddleWare.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../core/Textbuf.hh"
#include "../common/memory.h"
#include "../common/license.h"
#include "../common/version.h"
#include <string.h>
#include <stdlib.h>


using mctr::MainController;
using namespace jnimw;

JNIEXPORT jlong JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_init(JNIEnv *, jobject, jint max_ptcs)
{
  Jnimw *userInterface;
  userInterface = new Jnimw();
  Jnimw::userInterface = userInterface;
  MainController::initialize(*userInterface, (int)max_ptcs);
  return (jlong)userInterface;
}

JNIEXPORT void JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_terminate(JNIEnv *, jobject)
{
  MainController::terminate();
  delete Jnimw::userInterface;
  Jnimw::userInterface = NULL;
}

JNIEXPORT void JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_add_1host(JNIEnv *env, jobject, jstring group_name, jstring host_name)
{
  // Fix for TR HO56282.
  if (!Jnimw::userInterface)
    return;
  jboolean iscopy_grp;
  jboolean iscopy_hst;
  const char *grp_name = env->GetStringUTFChars(group_name, &iscopy_grp);
  const char *hst_name = env->GetStringUTFChars(host_name, &iscopy_hst);
  MainController::add_host(grp_name, hst_name);
  env->ReleaseStringUTFChars(group_name, grp_name);
  env->ReleaseStringUTFChars(host_name, hst_name);
}

JNIEXPORT void JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_assign_1component(JNIEnv *env, jobject, jstring host_or_group, jstring component_id)
{
  // Fix for TR HO56282.
  if (!Jnimw::userInterface)
    return;
  jboolean iscopy_hog;
  jboolean iscopy_cid;
  const char *hog = env->GetStringUTFChars(host_or_group, &iscopy_hog);
  const char *cid = env->GetStringUTFChars(component_id, &iscopy_cid);
  MainController::assign_component(hog, cid);
  env->ReleaseStringUTFChars(host_or_group, hog);
  env->ReleaseStringUTFChars(component_id, cid);
}

JNIEXPORT void JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_destroy_1host_1groups(JNIEnv *, jobject)
{
  // Fix for TR HO56282.
  if (!Jnimw::userInterface)
    return;
  MainController::destroy_host_groups();
}

JNIEXPORT void JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_set_1kill_1timer(JNIEnv *, jobject, jdouble timer_val)
{
  // Fix for TR HO56282.
  if (!Jnimw::userInterface)
    return;
  MainController::set_kill_timer((double)timer_val);
}

JNIEXPORT jint JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_start_1session(JNIEnv *env, jobject, jstring local_address, jint tcp_port, jboolean unixdomainsocketenabled)
{
  // Fix for TR HO56282.
  if (!Jnimw::userInterface)
    return (jint)-1;
  jboolean iscopy;
  const char *local_addr = env->GetStringUTFChars(local_address, &iscopy);
  if (strcmp(local_addr, "NULL") == 0) {
     env->ReleaseStringUTFChars(local_address, local_addr);
     local_addr = NULL;
  }
  unsigned short tcp_listen_port = (unsigned short)tcp_port;
  int ret = MainController::start_session(local_addr, tcp_listen_port, unixdomainsocketenabled);
  if (local_addr != NULL) env->ReleaseStringUTFChars(local_address, local_addr);
  return (jint)ret;
}

JNIEXPORT void JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_shutdown_1session(JNIEnv *, jobject)
{
  // Fix for TR HO56282.
  if (!Jnimw::userInterface)
    return;
  MainController::shutdown_session();
}

/**
 * ASYNCHRONOUS
 * Configure MainController and HCs.
 * It is also known as "Set parameter" operation.
 * 
 * It can be called in the following states:
 *   MC_LISTENING            -> MC_LISTENING_CONFIGURED will be the next state, when configuring operation is finished
 *   MC_LISTENING_CONFIGURED -> MC_LISTENING_CONFIGURED will be the next state, when configuring operation is finished
 *   MC_HC_CONNECTED         -> MC_CONFIGURING immediately and MC_ACTIVE when configuring operation is finished
 * 
 * @param env            Pointer to the caller java environment (mandatory parameter of every JNI function)
 * @param jobject        The caller java object (mandatory parameter of every JNI function)
 * @param config_file config string, generated from the config file by removing unnecessary parts. It can be empty string, and also null.
 *                    Generating config string can be done in 2 ways:
 *                      by "hand" (if the parameter is not empty string and not null):
 *                        on the java side, and pass it as a parameter, for an example see JniExecutor.DEFAULT_CONFIG_STRING or BaseExecutor.generateCfgString()
 *                      by MainController (if the parameter is empty string or null):
 *                        call set_cfg_file() after init() with the cfg file, and let MainController process it and create the config string,
 *                        and then config() can be called with empty string or null as a parameter, which means that config string from MainController config data will be used.
 * 
 * NOTE: Function comment is here, because function declaration is in org_eclipse_titan_executor_jni_JNIMiddleWare.h, which is generated.
 */
JNIEXPORT void JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_configure(JNIEnv *env, jobject, jstring config_file)
{
  // Fix for TR HO56282.
  if (!Jnimw::userInterface)
    return;
  if (config_file == NULL || env->GetStringLength( config_file ) == 0 ) {
    // use config string, which was generated by MainController
    MainController::configure(Jnimw::mycfg.config_read_buffer);
  }
  else {
    jboolean iscopy;
    const char *conf_file = env->GetStringUTFChars(config_file, &iscopy);
    MainController::configure(conf_file);
    env->ReleaseStringUTFChars(config_file, conf_file);
  }
}

/**
 * The name of the config file is sent here for syntactic and semantic analysis.
 * The result config string is needed by start_session() and configure().
 * Result is stored in Jnimw::mycfg.
 * We need the following info from it:
 *   config_read_buffer: passed to configure() if parameter is an empty string
 *   local_address:      needed by the java side for start_session() and starting the HC, it is read by calling get_mc_host()
 *   tcp_listen_port:    needed by the java side for start_session(), it is read by calling get_port()
 *   kill_timer:         processed in this function
 *   group_list:         processed in this function
 *   component_list:     processed in this function
 * This code is based on the following code: Cli.cc: Cli::enterloop()
 * 
 * @param env            Pointer to the caller java environment (mandatory parameter of every JNI function)
 * @param jobject        The caller java object (mandatory parameter of every JNI function)
 * @param conf_file_name The configuration file name of the TTCN-3 test project
 * 
 * NOTE: Function comment is here, because function declaration is in org_eclipse_titan_executor_jni_JNIMiddleWare.h, which is generated.
 */
JNIEXPORT void JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_set_1cfg_1file(JNIEnv *env, jobject, jstring config_file_name)
{
  if (!Jnimw::userInterface)
    return;
  jboolean iscopy;
  if (config_file_name == NULL)
    return;
  const char *conf_file_name = env->GetStringUTFChars(config_file_name, &iscopy);
  
  if (process_config_read_file(conf_file_name, &Jnimw::mycfg)) {
    MainController::error("Error was found in the configuration file. Exiting.");
  }
  else {
    MainController::set_kill_timer(Jnimw::mycfg.kill_timer);

    for (int i = 0; i < Jnimw::mycfg.group_list_len; ++i) {
      const group_item& item = Jnimw::mycfg.group_list[i];
      MainController::add_host( item.group_name, item.host_name );
    }

    for (int i = 0; i < Jnimw::mycfg.component_list_len; ++i) {
      const component_item& item = Jnimw::mycfg.component_list[i];
      MainController::assign_component( item.host_or_group, item.component );
    }
  }

  env->ReleaseStringUTFChars(config_file_name, conf_file_name);
}

/**
 * Local host address for start_session() and starting the HC
 * This will contain valid information after successful run of Java_org_eclipse_titan_executor_jni_JNIMiddleWare_set_1cfg_1file()
 * 
 * @param env            Pointer to the caller java environment (mandatory parameter of every JNI function)
 * @param jobject        The caller java object (mandatory parameter of every JNI function)
 * @return Local host address
 * 
 * NOTE: Function comment is here, because function declaration is in org_eclipse_titan_executor_jni_JNIMiddleWare.h, which is generated.
 */
JNIEXPORT jstring JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_get_1mc_1host(JNIEnv *env, jobject)
{
  return env->NewStringUTF( Jnimw::mycfg.local_addr != NULL ? Jnimw::mycfg.local_addr : "NULL" );
}

/**
 * TCP listen port for start_session()
 * This will contain valid information after successful run of Java_org_eclipse_titan_executor_jni_JNIMiddleWare_set_1cfg_1file()
 * 
 * @param env            Pointer to the caller java environment (mandatory parameter of every JNI function)
 * @param jobject        The caller java object (mandatory parameter of every JNI function)
 * @return TCP listen port
 * 
 * NOTE: Function comment is here, because function declaration is in org_eclipse_titan_executor_jni_JNIMiddleWare.h, which is generated.
 */
JNIEXPORT jint JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_get_1port(JNIEnv *, jobject)
{
  return (jint)Jnimw::mycfg.tcp_listen_port;
}

JNIEXPORT void JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_create_1mtc(JNIEnv *, jobject, jint host_index)
{
  // Fix for TR HO56282.
  if (!Jnimw::userInterface)
    return;
  MainController::create_mtc((int)host_index);
}

JNIEXPORT void JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_exit_1mtc(JNIEnv *, jobject)
{
  // Fix for TR HO56282.
  if (!Jnimw::userInterface)
    return;
  MainController::exit_mtc();
}

JNIEXPORT void JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_execute_1control(JNIEnv *env, jobject, jstring module_name)
{
  // Fix for TR HO56282.
  if (!Jnimw::userInterface)
    return;
  jboolean iscopy;
  const char *mod_name = env->GetStringUTFChars(module_name, &iscopy);
  MainController::execute_control(mod_name);
  env->ReleaseStringUTFChars(module_name, mod_name);
}

JNIEXPORT void JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_execute_1testcase(JNIEnv *env, jobject, jstring module_name, jstring testcase_name)
{
  // Fix for TR HO56282.
  if (!Jnimw::userInterface)
    return;
  jboolean iscopy_mod;
  jboolean iscopy_tes;
  const char *mod_name = env->GetStringUTFChars(module_name, &iscopy_mod);
  const char *testc_name = env->GetStringUTFChars(testcase_name, &iscopy_tes);
  MainController::execute_testcase(mod_name, testc_name);
  env->ReleaseStringUTFChars(module_name, mod_name);
  env->ReleaseStringUTFChars(testcase_name, testc_name);
}

/**
 * Gets the length of the execute list.
 * @param env            Pointer to the caller java environment (mandatory parameter of every JNI function)
 * @param jobject        The caller java object (mandatory parameter of every JNI function)
 * @return The length of the execute list,
 *         which is defined in the [EXECUTE] section in the configuration file.
 *         0, if there is no [EXECUTE] section in the configuration file.
 * 
 * NOTE: Function comment is here, because function declaration is in org_eclipse_titan_executor_jni_JNIMiddleWare.h, which is generated.
 */
JNIEXPORT jint JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_get_1execute_1cfg_1len(JNIEnv *, jobject)
{
  return (jint)Jnimw::mycfg.execute_list_len;
}

/**
 * Executes the index-th element form the execute list,
 * which is defined in the [EXECUTE] section in the configuration file.
 * Based on Cli::executeFromList()
 * @param env            Pointer to the caller java environment (mandatory parameter of every JNI function)
 * @param jobject        The caller java object (mandatory parameter of every JNI function)
 * @param index          The test index from the execute list
 * 
 * NOTE: Function comment is here, because function declaration is in org_eclipse_titan_executor_jni_JNIMiddleWare.h, which is generated.
 */
JNIEXPORT void JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_execute_1cfg(JNIEnv *, jobject, jint index)
{
  if (!Jnimw::userInterface)
    return;
  if ( index < 0 || index >= Jnimw::mycfg.execute_list_len ) {
    MainController::error("Java_org_eclipse_titan_executor_jni_JNIMiddleWare_execute_1cfg(): invalid argument: index");
    return;
  }
  const execute_list_item& item = Jnimw::mycfg.execute_list[ index ];
  if ( item.testcase_name == NULL ) {
    MainController::execute_control( item.module_name );
  } else if ( !strcmp( item.testcase_name, "*" ) ) {
    MainController::execute_testcase( item.module_name, NULL );
  } else {
    MainController::execute_testcase( item.module_name, item.testcase_name );
  }
}

JNIEXPORT void JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_stop_1after_1testcase(JNIEnv *, jobject, jboolean new_state)
{
  // Fix for TR HO56282.
  if (!Jnimw::userInterface)
    return;
  MainController::stop_after_testcase((bool)new_state);
}

JNIEXPORT void JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_continue_1testcase(JNIEnv *, jobject)
{
  // Fix for TR HO56282.
  if (!Jnimw::userInterface)
    return;
  MainController::continue_testcase();
}

JNIEXPORT void JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_stop_1execution(JNIEnv *, jobject)
{
  // Fix for TR HO56282.
  if (!Jnimw::userInterface)
    return;
  MainController::stop_execution();
}

JNIEXPORT jobject JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_get_1state(JNIEnv *env, jobject)
{
  jclass cls = env->FindClass("org/eclipse/titan/executor/jni/McStateEnum");
  if (cls == NULL) {
    printf("Can't find class org.eclipse.titan.executor.jni.McStateEnum\n");
  }
  jfieldID fid = 0;
  jobject ret;
  // Fix for TR HO56282.
  // MC's get_state() calls lock(), but the mutex is destroyed already at this
  // point from JNI code...
  if (!Jnimw::userInterface) {
    fid = env->GetStaticFieldID(cls, "MC_INACTIVE",
                                "Lorg/eclipse/titan/executor/jni/McStateEnum;");
    if (fid == 0) {
      printf("Can't find field MC_INACTIVE\n");
    }
  } else {
    switch (MainController::get_state()) {
    case mctr::MC_INACTIVE:
      fid = env->GetStaticFieldID(cls, "MC_INACTIVE",
                                  "Lorg/eclipse/titan/executor/jni/McStateEnum;");
      if (fid == 0) {
        printf("Can't find field MC_INACTIVE\n");
      }
      break;
    case mctr::MC_LISTENING:
      fid = env->GetStaticFieldID(cls, "MC_LISTENING",
                                  "Lorg/eclipse/titan/executor/jni/McStateEnum;");
      if (fid == 0) {
        printf("Can't find field MC_LISTENING\n");
      }
      break;
    case mctr::MC_LISTENING_CONFIGURED:
      fid = env->GetStaticFieldID(cls, "MC_LISTENING_CONFIGURED",
                                  "Lorg/eclipse/titan/executor/jni/McStateEnum;");
      if (fid == 0) {
        printf("Can't find field MC_LISTENING_CONFIGURED\n");
      }
      break;
    case mctr::MC_HC_CONNECTED:
      fid = env->GetStaticFieldID(cls, "MC_HC_CONNECTED",
                                  "Lorg/eclipse/titan/executor/jni/McStateEnum;");
      if (fid == 0) {
        printf("Can't find field MC_HC_CONNECTED\n");
      }
      break;
    case mctr::MC_CONFIGURING:
      fid = env->GetStaticFieldID(cls, "MC_CONFIGURING",
                                  "Lorg/eclipse/titan/executor/jni/McStateEnum;");
      if (fid == 0) {
        printf("Can't find field MC_CONFIGURING\n");
      }
      break;
    case mctr::MC_ACTIVE:
      fid = env->GetStaticFieldID(cls, "MC_ACTIVE",
                                  "Lorg/eclipse/titan/executor/jni/McStateEnum;");
      if (fid == 0) {
        printf("Can't find field MC_ACTIVE\n");
      }
      break;
    case mctr::MC_SHUTDOWN:
      fid = env->GetStaticFieldID(cls, "MC_SHUTDOWN",
                                  "Lorg/eclipse/titan/executor/jni/McStateEnum;");
      if (fid == 0) {
        printf("Can't find field MC_SHUTDOWN\n");
      }
      break;
    case mctr::MC_CREATING_MTC:
      fid = env->GetStaticFieldID(cls, "MC_CREATING_MTC",
                                  "Lorg/eclipse/titan/executor/jni/McStateEnum;");
      if (fid == 0) {
        printf("Can't find field MC_CREATING_MTC\n");
      }
      break;
    case mctr::MC_READY:
      fid = env->GetStaticFieldID(cls, "MC_READY",
                                  "Lorg/eclipse/titan/executor/jni/McStateEnum;");
      if (fid == 0) {
        printf("Can't find field MC_READY\n");
      }
      break;
    case mctr::MC_TERMINATING_MTC:
      fid = env->GetStaticFieldID(cls, "MC_TERMINATING_MTC",
                                  "Lorg/eclipse/titan/executor/jni/McStateEnum;");
      if (fid == 0) {
        printf("Can't find field MC_TERMINATING_MTC\n");
      }
      break;
    case mctr::MC_EXECUTING_CONTROL:
      fid = env->GetStaticFieldID(cls, "MC_EXECUTING_CONTROL",
                                  "Lorg/eclipse/titan/executor/jni/McStateEnum;");
      if (fid == 0) {
        printf("Can't find field MC_EXECUTING_CONTROL\n");
      }
      break;
    case mctr::MC_EXECUTING_TESTCASE:
      fid = env->GetStaticFieldID(cls, "MC_EXECUTING_TESTCASE",
                                  "Lorg/eclipse/titan/executor/jni/McStateEnum;");
      if (fid == 0) {
        printf("Can't find field MC_EXECUTING_TESTCASE\n");
      }
      break;
    case mctr::MC_TERMINATING_TESTCASE:
      fid = env->GetStaticFieldID(cls, "MC_TERMINATING_TESTCASE",
                                  "Lorg/eclipse/titan/executor/jni/McStateEnum;");
      if (fid == 0) {
        printf("Can't find field MC_TERMINATING_TESTCASE\n");
      }
      break;
    case mctr::MC_PAUSED:
      fid = env->GetStaticFieldID(cls, "MC_PAUSED",
                                  "Lorg/eclipse/titan/executor/jni/McStateEnum;");
      if (fid == 0) {
        printf("Can't find field MC_PAUSED\n");
      }
      break;
    default:
      printf("Unknown mc_state_enum state\n");
    }
  }
  ret = env->GetStaticObjectField(cls, fid);
  env->ExceptionClear();
  return ret;
}

JNIEXPORT jboolean JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_get_1stop_1after_1testcase(JNIEnv *, jobject)
{
  // Fix for TR HO56282.
  if (!Jnimw::userInterface)
    return (jboolean)true;
  return (jboolean)MainController::get_stop_after_testcase();
}

JNIEXPORT jint JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_get_1nof_1hosts(JNIEnv *, jobject)
{
  // Fix for TR HO56282.
  if (!Jnimw::userInterface)
    return (jint)-1;
  return (jint)MainController::get_nof_hosts();
}

JNIEXPORT jobject JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_get_1host_1data(JNIEnv *env, jobject, jint host_index)
{
  // Fix for TR HO56282.
  if (!Jnimw::userInterface)
    return NULL;
  mctr::host_struct *hs = MainController::get_host_data((int)host_index);
  if (hs == NULL) return NULL;
  IPAddress *ip_addr = hs->ip_addr;
  const char *hostname = hs->hostname;
  const char *hostname_local = hs->hostname_local;
  const char *machine_type = hs->machine_type;
  const char *system_name = hs->system_name;
  const char *system_release = hs->system_release;
  const char *system_version = hs->system_version;
  boolean *transport_supported = hs->transport_supported;
  const char *log_source = hs->log_source;
  mctr::hc_state_enum hc_state = hs->hc_state;
  int hc_fd = hs->hc_fd;
  Text_Buf *text_buf = hs->text_buf;
  int n_components = hs->n_components;
  component *components = hs->components;
  mctr::string_set allowed_components = hs->allowed_components;
  bool all_components_allowed = hs->all_components_allowed;
  int n_active_components = hs->n_active_components;

  /*// --- DEBUG --- filling test data
  in_addr t_i;
  t_i.s_addr = 121345;
  in_addr ip_addr = t_i;
  const char *hostname = "test hostname";
  const char *hostname_local = "test hostname_local";
  const char *machine_type = "test machine_type";
  const char *system_name = "test system_name";
  const char *system_release = "test system_release";
  const char *system_version = "test system_version";
  boolean test[3] = {true, true, false};
  boolean *transport_supported = test;
  const char *log_source = "test log_source";
  mctr::hc_state_enum hc_state = mctr::HC_CONFIGURING;
  int hc_fd = 8;
  Text_Buf *text_buf = new Text_Buf();
  text_buf->push_string("test Text_Buf");
  int n_components = 3;
  int test_i[3] = {2,4,5};
  component *components = (component*) test_i;
  mctr::string_set s_set;
  s_set.n_elements = 3;
  char *t_stringarray[3] = {"test 1", "test 2", "test 3"};
  s_set.elements = t_stringarray;
  mctr::string_set allowed_components = s_set;
  bool all_components_allowed = true;
  int n_active_components = 5;
  // --- END OF DEBUG ---*/

  // creating HostStruct
  jclass cls = env->FindClass("org/eclipse/titan/executor/jni/HostStruct");
  if( cls == NULL ) {
    printf("Can't find class org.eclipse.titan.executor.jni.HostStruct\n");
  }
  jfieldID fid;
  jmethodID mid;
  jobject HostStruct;
  mid = env->GetMethodID(cls, "<init>", "(IIII)V");
  HostStruct = env->NewObject(cls, mid, 3, text_buf->get_len(), n_components, allowed_components.n_elements);

  // processing struct fields
  // ip_addr
  const char *ipaddr = ip_addr->get_addr_str();
  fid = env->GetFieldID(cls, "ip_addr",  "Ljava/lang/String;");
  if (fid == 0) {
    printf("Can't find field hostname\n");
  }
  env->SetObjectField(HostStruct, fid, env->NewStringUTF(ipaddr));

  // hostname
  fid = env->GetFieldID(cls, "hostname",  "Ljava/lang/String;");
  if (fid == 0) {
    printf("Can't find field hostname\n");
  }
  env->SetObjectField(HostStruct, fid, env->NewStringUTF(hostname));

  // hostname_local
  fid = env->GetFieldID(cls, "hostname_local",  "Ljava/lang/String;");
  if (fid == 0) {
    printf("Can't find field hostname_local\n");
  }
  env->SetObjectField(HostStruct, fid, env->NewStringUTF(hostname_local));

  // machine_type
  fid = env->GetFieldID(cls, "machine_type",  "Ljava/lang/String;");
  if (fid == 0) {
    printf("Can't find field machine_type\n");
  }
  env->SetObjectField(HostStruct, fid, env->NewStringUTF(machine_type));

  // system_name
  fid = env->GetFieldID(cls, "system_name",  "Ljava/lang/String;");
  if (fid == 0) {
    printf("Can't find field system_name\n");
  }
  env->SetObjectField(HostStruct, fid, env->NewStringUTF(system_name));

  // system_release
  fid = env->GetFieldID(cls, "system_release",  "Ljava/lang/String;");
  if (fid == 0) {
    printf("Can't find field system_release\n");
  }
  env->SetObjectField(HostStruct, fid, env->NewStringUTF(system_release));

  // system_version
  fid = env->GetFieldID(cls, "system_version",  "Ljava/lang/String;");
  if (fid == 0) {
    printf("Can't find field system_version\n");
  }
  env->SetObjectField(HostStruct, fid, env->NewStringUTF(system_version));

  // transport_supported
  fid = env->GetFieldID(cls, "transport_supported",  "[Z");
  if (fid == 0) {
    printf("Can't find field transport_supported\n");
  }
  jbooleanArray TransportSupported = (jbooleanArray)env->GetObjectField(HostStruct, fid);
  env->SetBooleanArrayRegion(TransportSupported, (jsize)0, 3,(jboolean *)transport_supported);

  // log_source
  fid = env->GetFieldID(cls, "log_source",  "Ljava/lang/String;");
  if (fid == 0) {
    printf("Can't find field log_source\n");
  }
  env->SetObjectField(HostStruct, fid, env->NewStringUTF(log_source));

  // hc_state
  fid = env->GetFieldID(cls, "hc_state",  "Lorg/eclipse/titan/executor/jni/HcStateEnum;");
  if (fid == 0) {
    printf("Can't find field hc_state\n");
  }

  jclass cls_hc = env->FindClass("org/eclipse/titan/executor/jni/HcStateEnum");
  if( cls_hc == NULL ) {
    printf("Can't find class org.eclipse.titan.executor.jni.HcStateEnum\n");
  }
  jfieldID fid_hc = 0;
  jobject ret;
  switch(hc_state) {
    case mctr::HC_IDLE:
      fid_hc = env->GetStaticFieldID(cls_hc, "HC_IDLE",  "Lorg/eclipse/titan/executor/jni/HcStateEnum;");
      if (fid_hc == 0) {
        printf("Can't find field HC_IDLE\n");
      }
      break;
    case mctr::HC_CONFIGURING:
      fid_hc = env->GetStaticFieldID(cls_hc, "HC_CONFIGURING",  "Lorg/eclipse/titan/executor/jni/HcStateEnum;");
      if (fid_hc == 0) {
        printf("Can't find field HC_CONFIGURING\n");
      }
      break;
    case mctr::HC_ACTIVE:
      fid_hc = env->GetStaticFieldID(cls_hc, "HC_ACTIVE",  "Lorg/eclipse/titan/executor/jni/HcStateEnum;");
      if (fid_hc == 0) {
        printf("Can't find field HC_ACTIVE\n");
      }
      break;
    case mctr::HC_OVERLOADED:
      fid_hc = env->GetStaticFieldID(cls_hc, "HC_OVERLOADED",  "Lorg/eclipse/titan/executor/jni/HcStateEnum;");
      if (fid_hc == 0) {
        printf("Can't find field HC_OVERLOADED\n");
      }
      break;
    case mctr::HC_CONFIGURING_OVERLOADED:
      fid_hc = env->GetStaticFieldID(cls_hc, "HC_CONFIGURING_OVERLOADED",  "Lorg/eclipse/titan/executor/jni/HcStateEnum;");
      if (fid_hc == 0) {
        printf("Can't find field HC_CONFIGURING_OVERLOADED\n");
      }
      break;
    // ----------------------------------------------- 6-7
    case mctr::HC_EXITING:
      fid_hc = env->GetStaticFieldID(cls_hc, "HC_EXITING",  "Lorg/eclipse/titan/executor/jni/HcStateEnum;");
      if (fid_hc == 0) {
        printf("Can't find field HC_EXITING\n");
      }
      break;
    case mctr::HC_DOWN:
      fid_hc = env->GetStaticFieldID(cls_hc, "HC_DOWN",  "Lorg/eclipse/titan/executor/jni/HcStateEnum;");
      if (fid_hc == 0) {
        printf("Can't find field HC_DOWN\n");
      }
      break;
    default:
      printf("Unknown hc_state_enum state\n");
  }
  ret = env->GetStaticObjectField(cls_hc, fid_hc);
  env->ExceptionClear();
  env->SetObjectField(HostStruct, fid, ret);

  // hc_fd
  fid = env->GetFieldID(cls, "hc_fd",  "I");
  if (fid == 0) {
    printf("Can't find field hc_fd\n");
  }
  env->SetIntField(HostStruct, fid, (jint)hc_fd);

  // text_buf
  fid = env->GetFieldID(cls, "text_buf",  "[B");
  if (fid == 0) {
    printf("Can't find field text_buf\n");
  }
  jbyteArray TextBuf = (jbyteArray)env->GetObjectField(HostStruct, fid);
  env->SetByteArrayRegion(TextBuf, (jsize)0, text_buf->get_len(),
    const_cast<jbyte *>((const jbyte *)text_buf->get_data()));

  // components
  fid = env->GetFieldID(cls, "components",  "[I");
  if (fid == 0) {
    printf("Can't find field components\n");
  }

  jintArray Components = (jintArray)env->GetObjectField(HostStruct, fid);
  env->SetIntArrayRegion(Components, (jsize)0,n_components,(jint *)components);

  // allowed_components
  fid = env->GetFieldID(cls, "allowed_components",  "[Ljava/lang/String;");
  if (fid == 0) {
    printf("Can't find field allowed_components\n");
  }
  jobjectArray allowedComponents = (jobjectArray) env->GetObjectField(HostStruct, fid);
  for(int i = 0; i < allowed_components.n_elements; i++) {
  	env->SetObjectArrayElement(allowedComponents, i, env->NewStringUTF(allowed_components.elements[i]));
  }

  // all_components_allowed
  fid = env->GetFieldID(cls, "all_components_allowed",  "Z");
  if (fid == 0) {
    printf("Can't find field all_components_allowed\n");
  }
  env->SetBooleanField(HostStruct, fid, (jboolean)all_components_allowed);

  // n_active_components
  fid = env->GetFieldID(cls, "n_active_components",  "I");
  if (fid == 0) {
    printf("Can't find field n_active_components\n");
  }
  env->SetIntField(HostStruct, fid, (jint)n_active_components);

  return HostStruct;
}

JNIEXPORT jobject JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_get_1component_1data(JNIEnv *env, jobject, jint component_reference)
{
  // Fix for TR HO56282.
  if (!Jnimw::userInterface)
    return NULL;
  mctr::component_struct *cs = MainController::get_component_data((int)component_reference);
  if (cs == NULL) return NULL;

  // getting data from struct

  // If any of the pointers are NULL, we return null.
  // Some of the pointers may be NULL if its value is not initialized,
  // or temporarily set to NULL, like text_buf is NULL during the create MTC operation: it is set to NULL when create_mtc() is called,
  // and it will have valid data when asynchronous request of MTC creation is finished.
  // Reading component data makes sense only in the following states:
  // MC_READY, MC_EXECUTING_CONTROL, MC_EXECUTING_TESTCASE, MC_TERMINATING_TESTCASE, MC_PAUSED
  component comp_ref = cs->comp_ref;
  qualified_name comp_type = cs->comp_type;
  const char *comp_name = cs->comp_name;
  if ( NULL == comp_name ) {
    return NULL;
  }
  const char *log_source = cs->log_source;
  if ( NULL == log_source ) {
    return NULL;
  }
  mctr::host_struct *comp_location = cs->comp_location;
  if ( NULL == comp_location ) {
    return NULL;
  }
  mctr::tc_state_enum tc_state = cs->tc_state;
  verdicttype local_verdict = cs->local_verdict;
  int tc_fd = cs->tc_fd;
  Text_Buf *text_buf = cs->text_buf;
  if ( NULL == text_buf ) {
    return NULL;
  }
  qualified_name tc_fn_name = cs->tc_fn_name;
  const char *return_type = cs->return_type;
  if ( NULL == return_type ) {
    return NULL;
  }
  int return_value_len = cs->return_value_len;
  bool is_alive = cs->is_alive;
  bool stop_requested = cs->stop_requested;
  bool process_killed = cs->process_killed;

  /*// --- DEBUG --- filling test data
  component comp_ref = (component) 5;
  qualified_name comp_type;
  comp_type.module_name = "test module";
  comp_type.definition_name = "test definition";
  const char *comp_name = "test comp_name";
  const char *log_source = "test log_source";
    // --- DEBUG --- filling inner host_struct
    mctr::host_struct debug_comp_location;
    in_addr t_i;
    t_i.s_addr = 121345;
    debug_comp_location.ip_addr = t_i;
    debug_comp_location.hostname = "inner test hostname";
    debug_comp_location.hostname_local = "inner test hostname_local";
    debug_comp_location.machine_type = "inner test machine_type";
    debug_comp_location.system_name = "inner test system_name";
    debug_comp_location.system_release = "inner test system_release";
    debug_comp_location.system_version = "inner test system_version";
    boolean test[3] = {true, true, false};
    debug_comp_location.transport_supported = test;
    debug_comp_location.log_source = "inner test log_source";
    debug_comp_location.hc_state = mctr::HC_CONFIGURING;
    debug_comp_location.hc_fd = 8;
    debug_comp_location.text_buf = new Text_Buf();
    debug_comp_location.text_buf->push_string("inner test Text_Buf");
    debug_comp_location.n_components = 3;
    int test_i[3] = {2,4,5};
    debug_comp_location.components = (component*) test_i;
    mctr::string_set s_set;
    s_set.n_elements = 3;
    char *t_stringarray[3] = {"inner test 1", "inner test 2", "inner test 3"};
    s_set.elements = t_stringarray;
    debug_comp_location.allowed_components = s_set;
    debug_comp_location.all_components_allowed = true;
    debug_comp_location.n_active_components = 5;
    // --- END OF DEBUG host_struct ---
  mctr::host_struct *comp_location = &debug_comp_location;
  mctr::tc_state_enum tc_state = mctr::TC_INITIAL;
  verdicttype local_verdict = FAIL;
  int tc_fd = 7;
  Text_Buf *text_buf = new Text_Buf();
  text_buf->push_string("test Text_Buf component");
  qualified_name tc_fn_name;
  tc_fn_name.module_name = "test tc_fn_name module_name";
  tc_fn_name.definition_name = "test tc_fn_name definition_name";
  const char *return_type = "test return_type";
  int return_value_len = 10;
  bool is_alive = true;
  bool stop_requested = false;
  bool process_killed = false;
  // --- END OF DEBUG ---*/

  // creating ComponentStruct
  jclass cls = env->FindClass("org/eclipse/titan/executor/jni/ComponentStruct");
  if( cls == NULL ) {
    printf("Can't find class org.eclipse.titan.executor.jni.ComponentStruct\n");
  }
  jfieldID fid;
  jmethodID mid;
  jobject ComponentStruct;
  mid = env->GetMethodID(cls, "<init>", "(I)V");
  ComponentStruct = env->NewObject(cls, mid, text_buf->get_len());

  // processing struct fields
  // comp_ref
  fid = env->GetFieldID(cls, "comp_ref",  "I");
  if (fid == 0) {
    printf("Can't find field comp_ref\n");
  }
  env->SetIntField(ComponentStruct, fid, (jint) comp_ref);

  // comp_type
  fid = env->GetFieldID(cls, "comp_type",  "Lorg/eclipse/titan/executor/jni/QualifiedName;");
  if (fid == 0) {
    printf("Can't find field comp_type\n");
  }
  jclass cls_q = env->FindClass("org/eclipse/titan/executor/jni/QualifiedName");
  if( cls_q == NULL ) {
    printf("Can't find class QualifiedName\n");
  }
    // creating new QualifiedName object
  jmethodID mid_q = env->GetMethodID(cls_q, "<init>", "(Ljava/lang/String;Ljava/lang/String;)V");
  jobject qname = env->NewObject(cls_q, mid_q, env->NewStringUTF(comp_type.module_name),
    env->NewStringUTF(comp_type.definition_name));
  env->ExceptionClear();
  env->SetObjectField(ComponentStruct, fid, qname);

  // comp_name
  fid = env->GetFieldID(cls, "comp_name",  "Ljava/lang/String;");
  if (fid == 0) {
    printf("Can't find field comp_name\n");
  }
  env->SetObjectField(ComponentStruct, fid, env->NewStringUTF(comp_name));

  // log_source
  fid = env->GetFieldID(cls, "log_source",  "Ljava/lang/String;");
  if (fid == 0) {
    printf("Can't find field log_source\n");
  }
  env->SetObjectField(ComponentStruct, fid, env->NewStringUTF(log_source));

  // comp_location ---------------------------------------
  fid = env->GetFieldID(cls, "comp_location",  "Lorg/eclipse/titan/executor/jni/HostStruct;");
  jclass cls_hs = env->FindClass("org/eclipse/titan/executor/jni/HostStruct");
  if( cls_hs == NULL ) {
    printf("Can't find class org.eclipse.titan.executor.jni.HostStruct\n");
  }
  jfieldID fid_hs;
  jmethodID mid_hs;
  jobject HostStruct;
  mid_hs = env->GetMethodID(cls_hs, "<init>", "(IIII)V");
  HostStruct = env->NewObject(cls_hs, mid_hs, 3, comp_location->text_buf->get_len(), comp_location->n_components, comp_location->allowed_components.n_elements);

  if(env->ExceptionOccurred()) {
    printf("error at creating HostStruct\n");
    env->ExceptionDescribe();
    env->ExceptionClear();
  }
  // processing struct fields
  // ip_addr
  const char *ipaddr = comp_location->ip_addr->get_addr_str();
  fid_hs = env->GetFieldID(cls_hs, "ip_addr",  "Ljava/lang/String;");
  if (fid_hs == 0) {
    printf("Can't find field hostname\n");
  }
  env->SetObjectField(HostStruct, fid_hs, env->NewStringUTF(ipaddr));

  // hostname
  fid_hs = env->GetFieldID(cls_hs, "hostname",  "Ljava/lang/String;");
  if (fid_hs == 0) {
    printf("Can't find field hostname\n");
  }
  env->SetObjectField(HostStruct, fid_hs, env->NewStringUTF(comp_location->hostname));

  // hostname_local
  fid_hs = env->GetFieldID(cls_hs, "hostname_local",  "Ljava/lang/String;");
  if (fid_hs == 0) {
    printf("Can't find field hostname_local\n");
  }
  env->SetObjectField(HostStruct, fid_hs, env->NewStringUTF(comp_location->hostname_local));

  // machine_type
  fid_hs = env->GetFieldID(cls_hs, "machine_type",  "Ljava/lang/String;");
  if (fid_hs == 0) {
    printf("Can't find field machine_type\n");
  }
  env->SetObjectField(HostStruct, fid_hs, env->NewStringUTF(comp_location->machine_type));

  // system_name
  fid_hs = env->GetFieldID(cls_hs, "system_name",  "Ljava/lang/String;");
  if (fid_hs == 0) {
    printf("Can't find field system_name\n");
  }
  env->SetObjectField(HostStruct, fid_hs, env->NewStringUTF(comp_location->system_name));

  // system_release
  fid_hs = env->GetFieldID(cls_hs, "system_release",  "Ljava/lang/String;");
  if (fid_hs == 0) {
    printf("Can't find field system_release\n");
  }
  env->SetObjectField(HostStruct, fid_hs, env->NewStringUTF(comp_location->system_release));

  // system_version
  fid_hs = env->GetFieldID(cls_hs, "system_version",  "Ljava/lang/String;");
  if (fid_hs == 0) {
    printf("Can't find field system_version\n");
  }
  env->SetObjectField(HostStruct, fid_hs, env->NewStringUTF(comp_location->system_version));

  // transport_supported
  fid_hs = env->GetFieldID(cls_hs, "transport_supported",  "[Z");
  if (fid_hs == 0) {
    printf("Can't find field transport_supported\n");
  }
  jbooleanArray TransportSupported = (jbooleanArray)env->GetObjectField(HostStruct, fid_hs);
  env->SetBooleanArrayRegion(TransportSupported, (jsize)0, 3,(jboolean *)comp_location->transport_supported);

  // log_source
  fid_hs = env->GetFieldID(cls_hs, "log_source",  "Ljava/lang/String;");
  if (fid_hs == 0) {
    printf("Can't find field log_source\n");
  }
  env->SetObjectField(HostStruct, fid_hs, env->NewStringUTF(comp_location->log_source));

  // hc_state
  fid_hs = env->GetFieldID(cls_hs, "hc_state",  "Lorg/eclipse/titan/executor/jni/HcStateEnum;");
  if (fid_hs == 0) {
    printf("Can't find field hc_state\n");
  }

  jclass cls_hs_hc = env->FindClass("org/eclipse/titan/executor/jni/HcStateEnum");
  if( cls_hs_hc == NULL ) {
    printf("Can't find class org.eclipse.titan.executor.jni.HcStateEnum\n");
  }
  jfieldID fid_hs_hc = 0;
  jobject ret;
  switch(comp_location->hc_state) {
    case mctr::HC_IDLE:
      fid_hs_hc = env->GetStaticFieldID(cls_hs_hc, "HC_IDLE",  "Lorg/eclipse/titan/executor/jni/HcStateEnum;");
      if (fid_hs_hc == 0) {
        printf("Can't find field HC_IDLE\n");
      }
      break;
    case mctr::HC_CONFIGURING:
      fid_hs_hc = env->GetStaticFieldID(cls_hs_hc, "HC_CONFIGURING",  "Lorg/eclipse/titan/executor/jni/HcStateEnum;");
      if (fid_hs_hc == 0) {
        printf("Can't find field HC_CONFIGURING\n");
      }
      break;
    case mctr::HC_ACTIVE:
      fid_hs_hc = env->GetStaticFieldID(cls_hs_hc, "HC_ACTIVE",  "Lorg/eclipse/titan/executor/jni/HcStateEnum;");
      if (fid_hs_hc == 0) {
        printf("Can't find field HC_ACTIVE\n");
      }
      break;
    case mctr::HC_OVERLOADED:
      fid_hs_hc = env->GetStaticFieldID(cls_hs_hc, "HC_OVERLOADED",  "Lorg/eclipse/titan/executor/jni/HcStateEnum;");
      if (fid_hs_hc == 0) {
        printf("Can't find field HC_OVERLOADED\n");
      }
      break;
    case mctr::HC_CONFIGURING_OVERLOADED:
      fid_hs_hc = env->GetStaticFieldID(cls_hs_hc, "HC_CONFIGURING_OVERLOADED",  "Lorg/eclipse/titan/executor/jni/HcStateEnum;");
      if (fid_hs_hc == 0) {
        printf("Can't find field HC_CONFIGURING_OVERLOADED\n");
      }
      break;
    // ----------------------------------------------- 6-7
    case mctr::HC_EXITING:
      fid_hs_hc = env->GetStaticFieldID(cls_hs_hc, "HC_EXITING",  "Lorg/eclipse/titan/executor/jni/HcStateEnum;");
      if (fid_hs_hc == 0) {
        printf("Can't find field HC_EXITING\n");
      }
      break;
    case mctr::HC_DOWN:
      fid_hs_hc = env->GetStaticFieldID(cls_hs_hc, "HC_DOWN",  "Lorg/eclipse/titan/executor/jni/HcStateEnum;");
      if (fid_hs_hc == 0) {
        printf("Can't find field HC_DOWN\n");
      }
      break;
    default:
      printf("Unknown hc_state_enum state\n");
  }
  ret = env->GetStaticObjectField(cls_hs_hc, fid_hs_hc);
  env->ExceptionClear();
  env->SetObjectField(HostStruct, fid_hs, ret);

  // hc_fd
  fid_hs = env->GetFieldID(cls_hs, "hc_fd",  "I");
  if (fid_hs == 0) {
    printf("Can't find field hc_fd\n");
  }
  env->SetIntField(HostStruct, fid_hs, (jint)comp_location->hc_fd);

  // text_buf
  fid_hs = env->GetFieldID(cls_hs, "text_buf",  "[B");
  if (fid_hs == 0) {
    printf("Can't find field text_buf\n");
  }
  jbyteArray TextBuf = (jbyteArray)env->GetObjectField(HostStruct, fid_hs);
  env->SetByteArrayRegion(TextBuf, (jsize)0, comp_location->text_buf->get_len(),
    const_cast<jbyte *>((const jbyte *)text_buf->get_data()));

  // components
  fid_hs = env->GetFieldID(cls_hs, "components",  "[I");
  if (fid_hs == 0) {
    printf("Can't find field components\n");
  }

  jintArray Components = (jintArray)env->GetObjectField(HostStruct, fid_hs);
  env->SetIntArrayRegion(Components, (jsize)0, comp_location->n_components, (jint *)comp_location->components);

  // allowed_components
  fid_hs = env->GetFieldID(cls_hs, "allowed_components",  "[Ljava/lang/String;");
  if (fid_hs == 0) {
    printf("Can't find field allowed_components\n");
  }
  jobjectArray allowedComponents = (jobjectArray) env->GetObjectField(HostStruct, fid_hs);
  for(int i = 0; i < comp_location->allowed_components.n_elements; i++) {
  	env->SetObjectArrayElement(allowedComponents, i, env->NewStringUTF(comp_location->allowed_components.elements[i]));
  }

  // all_components_allowed
  fid_hs = env->GetFieldID(cls_hs, "all_components_allowed",  "Z");
  if (fid_hs == 0) {
    printf("Can't find field all_components_allowed\n");
  }
  env->SetBooleanField(HostStruct, fid_hs, (jboolean)comp_location->all_components_allowed);

  // n_active_components
  fid_hs = env->GetFieldID(cls_hs, "n_active_components",  "I");
  if (fid_hs == 0) {
    printf("Can't find field n_active_components\n");
  }
  env->SetIntField(HostStruct, fid_hs, (jint)comp_location->n_active_components);


  env->SetObjectField(ComponentStruct, fid, HostStruct); // finally setting inner host_struct
  // end of comp_location --------------------------------

  // tc_state
  fid = env->GetFieldID(cls, "tc_state",  "Lorg/eclipse/titan/executor/jni/TcStateEnum;");
  if (fid == 0) {
    printf("Can't find field tc_state\n");
  }

  jclass cls_tc = env->FindClass("org/eclipse/titan/executor/jni/TcStateEnum");
  if( cls_tc == NULL ) {
    printf("Can't find class org.eclipse.titan.executor.jni.TcStateEnum\n");
  }
  jfieldID fid_tc = 0;
  jobject ret_tc;
  switch(tc_state) {
    case mctr::TC_INITIAL:
      fid_tc = env->GetStaticFieldID(cls_tc, "TC_INITIAL",  "Lorg/eclipse/titan/executor/jni/TcStateEnum;");
      if (fid_tc == 0) {
        printf("Can't find field TC_INITIAL\n");
      }
      break;
    case mctr::TC_IDLE:
      fid_tc = env->GetStaticFieldID(cls_tc, "TC_IDLE",  "Lorg/eclipse/titan/executor/jni/TcStateEnum;");
      if (fid_tc == 0) {
        printf("Can't find field TC_IDLE\n");
      }
      break;
    case mctr::TC_CREATE:
      fid_tc = env->GetStaticFieldID(cls_tc, "TC_CREATE",  "Lorg/eclipse/titan/executor/jni/TcStateEnum;");
      if (fid_tc == 0) {
        printf("Can't find field TC_CREATE\n");
      }
      break;
    case mctr::TC_START:
      fid_tc = env->GetStaticFieldID(cls_tc, "TC_START",  "Lorg/eclipse/titan/executor/jni/TcStateEnum;");
      if (fid_tc == 0) {
        printf("Can't find field TC_START\n");
      }
      break;
    case mctr::TC_STOP:
      fid_tc = env->GetStaticFieldID(cls_tc, "TC_STOP",  "Lorg/eclipse/titan/executor/jni/TcStateEnum;");
      if (fid_tc == 0) {
        printf("Can't find field TC_STOP\n");
      }
      break;
    // ----------------------------------------------- 6-10
    case mctr::TC_KILL:
      fid_tc = env->GetStaticFieldID(cls_tc, "TC_KILL",  "Lorg/eclipse/titan/executor/jni/TcStateEnum;");
      if (fid_tc == 0) {
        printf("Can't find field TC_KILL\n");
      }
      break;
    case mctr::TC_CONNECT:
      fid_tc = env->GetStaticFieldID(cls_tc, "TC_CONNECT",  "Lorg/eclipse/titan/executor/jni/TcStateEnum;");
      if (fid_tc == 0) {
        printf("Can't find field TC_CONNECT\n");
      }
      break;
    case mctr::TC_DISCONNECT:
      fid_tc = env->GetStaticFieldID(cls_tc, "TC_DISCONNECT",  "Lorg/eclipse/titan/executor/jni/TcStateEnum;");
      if (fid_tc == 0) {
        printf("Can't find field TC_DISCONNECT\n");
      }
      break;
    case mctr::TC_MAP:
      fid_tc = env->GetStaticFieldID(cls_tc, "TC_MAP",  "Lorg/eclipse/titan/executor/jni/TcStateEnum;");
      if (fid_tc == 0) {
        printf("Can't find field TC_MAP\n");
      }
      break;
    case mctr::TC_UNMAP:
      fid_tc = env->GetStaticFieldID(cls_tc, "TC_UNMAP",  "Lorg/eclipse/titan/executor/jni/TcStateEnum;");
      if (fid_tc == 0) {
        printf("Can't find field TC_UNMAP\n");
      }
      break;
    // ----------------------------------------------- 11-15
    case mctr::TC_STOPPING:
      fid_tc = env->GetStaticFieldID(cls_tc, "TC_STOPPING",  "Lorg/eclipse/titan/executor/jni/TcStateEnum;");
      if (fid_tc == 0) {
        printf("Can't find field TC_STOPPING\n");
      }
      break;
    case mctr::TC_EXITING:
      fid_tc = env->GetStaticFieldID(cls_tc, "TC_EXITING",  "Lorg/eclipse/titan/executor/jni/TcStateEnum;");
      if (fid_tc == 0) {
        printf("Can't find field TC_EXITING\n");
      }
      break;
    case mctr::TC_EXITED:
      fid_tc = env->GetStaticFieldID(cls_tc, "TC_EXITED",  "Lorg/eclipse/titan/executor/jni/TcStateEnum;");
      if (fid_tc == 0) {
        printf("Can't find field TC_EXITED\n");
      }
      break;
    case mctr::MTC_CONTROLPART:
      fid_tc = env->GetStaticFieldID(cls_tc, "MTC_CONTROLPART",  "Lorg/eclipse/titan/executor/jni/TcStateEnum;");
      if (fid_tc == 0) {
        printf("Can't find field MTC_CONTROLPART\n");
      }
      break;
    case mctr::MTC_TESTCASE:
      fid_tc = env->GetStaticFieldID(cls_tc, "MTC_TESTCASE",  "Lorg/eclipse/titan/executor/jni/TcStateEnum;");
      if (fid_tc == 0) {
        printf("Can't find field MTC_TESTCASE\n");
      }
      break;
    // ----------------------------------------------- 16-20
    case mctr::MTC_ALL_COMPONENT_STOP:
      fid_tc = env->GetStaticFieldID(cls_tc, "MTC_ALL_COMPONENT_STOP",  "Lorg/eclipse/titan/executor/jni/TcStateEnum;");
      if (fid_tc == 0) {
        printf("Can't find field MTC_ALL_COMPONENT_STOP\n");
      }
      break;
    case mctr::MTC_ALL_COMPONENT_KILL:
      fid_tc = env->GetStaticFieldID(cls_tc, "MTC_ALL_COMPONENT_KILL",  "Lorg/eclipse/titan/executor/jni/TcStateEnum;");
      if (fid_tc == 0) {
        printf("Can't find field MTC_ALL_COMPONENT_KILL\n");
      }
      ret_tc = env->GetStaticObjectField(cls_tc, fid_tc);
      env->ExceptionClear();
      break;
    case mctr::MTC_TERMINATING_TESTCASE:
      fid_tc = env->GetStaticFieldID(cls_tc, "MTC_TERMINATING_TESTCASE",  "Lorg/eclipse/titan/executor/jni/TcStateEnum;");
      if (fid_tc == 0) {
        printf("Can't find field MTC_TERMINATING_TESTCASE\n");
      }
      break;
    case mctr::MTC_PAUSED:
      fid_tc = env->GetStaticFieldID(cls_tc, "MTC_PAUSED",  "Lorg/eclipse/titan/executor/jni/TcStateEnum;");
      if (fid_tc == 0) {
        printf("Can't find field MTC_PAUSED\n");
      }
      break;
    case mctr::PTC_FUNCTION:
      fid_tc = env->GetStaticFieldID(cls_tc, "PTC_FUNCTION",  "Lorg/eclipse/titan/executor/jni/TcStateEnum;");
      if (fid_tc == 0) {
        printf("Can't find field PTC_FUNCTION\n");
      }
      break;
    // ----------------------------------------------- 21-25
    case mctr::PTC_STARTING:
      fid_tc = env->GetStaticFieldID(cls_tc, "PTC_STARTING",  "Lorg/eclipse/titan/executor/jni/TcStateEnum;");
      if (fid_tc == 0) {
        printf("Can't find field PTC_STARTING\n");
      }
      break;
    case mctr::PTC_STOPPED:
      fid_tc = env->GetStaticFieldID(cls_tc, "PTC_STOPPED",  "Lorg/eclipse/titan/executor/jni/TcStateEnum;");
      if (fid_tc == 0) {
        printf("Can't find field PTC_STOPPED\n");
      }
      break;
    case mctr::PTC_KILLING:
      fid_tc = env->GetStaticFieldID(cls_tc, "PTC_KILLING",  "Lorg/eclipse/titan/executor/jni/TcStateEnum;");
      if (fid_tc == 0) {
        printf("Can't find field PTC_KILLING\n");
      }
      break;
    case mctr::PTC_STOPPING_KILLING:
      fid_tc = env->GetStaticFieldID(cls_tc, "PTC_STOPPING_KILLING",  "Lorg/eclipse/titan/executor/jni/TcStateEnum;");
      if (fid_tc == 0) {
        printf("Can't find field PTC_STOPPING_KILLING\n");
      }
      break;
    case mctr::PTC_STALE:
      fid_tc = env->GetStaticFieldID(cls_tc, "PTC_STALE",  "Lorg/eclipse/titan/executor/jni/TcStateEnum;");
      if (fid_tc == 0) {
        printf("Can't find field PTC_STALE\n");
      }
      break;
    // ----------------------------------------------- 26
    case mctr::TC_SYSTEM:
      fid_tc = env->GetStaticFieldID(cls_tc, "TC_SYSTEM",  "Lorg/eclipse/titan/executor/jni/TcStateEnum;");
      if (fid_tc == 0) {
        printf("Can't find field TC_SYSTEM\n");
      }
      break;
    default:
      printf("Unknown tc_state_enum state\n");
  }
  ret_tc = env->GetStaticObjectField(cls_tc, fid_tc);
  env->ExceptionClear();
  env->SetObjectField(ComponentStruct, fid, ret_tc);

  // local_verdict
  fid = env->GetFieldID(cls, "local_verdict",  "Lorg/eclipse/titan/executor/jni/VerdictTypeEnum;");
  if (fid == 0) {
    printf("Can't find field local_verdict\n");
  }
  jclass cls_v = env->FindClass("org/eclipse/titan/executor/jni/VerdictTypeEnum");
  if( cls_v == NULL ) {
    printf("Can't find class org.eclipse.titan.executor.jni.VerdictTypeEnum\n");
  }
  jfieldID fid_v = 0;
  jobject ret_v;
  switch(local_verdict) {
    case NONE:
      fid_v = env->GetStaticFieldID(cls_v, "NONE",  "Lorg/eclipse/titan/executor/jni/VerdictTypeEnum;");
      if (fid_v == 0) {
        printf("Can't find field NONE\n");
      }
      break;
    case PASS:
      fid_v = env->GetStaticFieldID(cls_v, "PASS",  "Lorg/eclipse/titan/executor/jni/VerdictTypeEnum;");
      if (fid_v == 0) {
        printf("Can't find field PASS\n");
      }
      break;
    case INCONC:
      fid_v = env->GetStaticFieldID(cls_v, "INCONC",  "Lorg/eclipse/titan/executor/jni/VerdictTypeEnum;");
      if (fid_v == 0) {
        printf("Can't find field INCONC\n");
      }
      break;
    case FAIL:
      fid_v = env->GetStaticFieldID(cls_v, "FAIL",  "Lorg/eclipse/titan/executor/jni/VerdictTypeEnum;");
      if (fid_v == 0) {
        printf("Can't find field FAIL\n");
      }
      break;
    case ERROR:
      fid_v = env->GetStaticFieldID(cls_v, "ERROR",  "Lorg/eclipse/titan/executor/jni/VerdictTypeEnum;");
      if (fid_v == 0) {
        printf("Can't find field ERROR\n");
      }
      break;
    default:
      printf("Unknown tc_state_enum state\n");
  }
  ret_v = env->GetStaticObjectField(cls_v, fid_v);
  env->ExceptionClear();
  env->SetObjectField(ComponentStruct, fid, ret_v);

  // tc_fd
  fid = env->GetFieldID(cls, "tc_fd",  "I");
  if (fid == 0) {
    printf("Can't find field tc_fd\n");
  }
  env->SetIntField(ComponentStruct, fid, (jint)tc_fd);

  // text_buf
  fid = env->GetFieldID(cls, "text_buf",  "[B");
  if (fid == 0) {
    printf("Can't find field text_buf\n");
  }
  jbyteArray TextBuf_c = (jbyteArray)env->GetObjectField(ComponentStruct, fid);
  env->SetByteArrayRegion(TextBuf_c, (jsize)0, text_buf->get_len(),
    const_cast<jbyte *>((const jbyte *)text_buf->get_data()));

  // tc_fn_name
  fid = env->GetFieldID(cls, "tc_fn_name",  "Lorg/eclipse/titan/executor/jni/QualifiedName;");
  if (fid == 0) {
    printf("Can't find field tc_fn_name\n");
  }
  jclass cls_qualified = env->FindClass("org/eclipse/titan/executor/jni/QualifiedName");
  if( cls_qualified == NULL ) {
    printf("Can't find class org.eclipse.titan.executor.jni.QualifiedName\n");
  }
  jmethodID mid_qualified;
  jobject QualifiedName;
  mid_qualified = env->GetMethodID(cls_qualified, "<init>", "(Ljava/lang/String;Ljava/lang/String;)V");
  QualifiedName = env->NewObject(cls_qualified, mid_qualified, env->NewStringUTF(tc_fn_name.module_name),
  	env->NewStringUTF(tc_fn_name.definition_name));
  env->SetObjectField(ComponentStruct, fid, QualifiedName);

  // return_type
  fid = env->GetFieldID(cls, "return_type",  "Ljava/lang/String;");
  if (fid == 0) {
    printf("Can't find field return_type\n");
  }
  env->SetObjectField(ComponentStruct, fid, env->NewStringUTF(return_type));

  // return_value_len
  fid = env->GetFieldID(cls, "return_value_len",  "I");
  if (fid == 0) {
    printf("Can't find field return_value_len\n");
  }
  env->SetIntField(ComponentStruct, fid, (jint)return_value_len);

  // is_alive
  fid = env->GetFieldID(cls, "is_alive",  "Z");
  if (fid == 0) {
    printf("Can't find field is_alive\n");
  }
  env->SetBooleanField(ComponentStruct, fid, (jboolean)is_alive);

  // stop_requested
  fid = env->GetFieldID(cls, "stop_requested",  "Z");
  if (fid == 0) {
    printf("Can't find field stop_requested\n");
  }
  env->SetBooleanField(ComponentStruct, fid, (jboolean)stop_requested);

  // process_killed
  fid = env->GetFieldID(cls, "process_killed",  "Z");
  if (fid == 0) {
    printf("Can't find field process_killed\n");
  }
  env->SetBooleanField(ComponentStruct, fid, (jboolean)process_killed);

  return ComponentStruct;
}

JNIEXPORT void JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_release_1data
  (JNIEnv *, jobject) {
  MainController::release_data();
}

JNIEXPORT jstring JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_get_1mc_1state_1name
  (JNIEnv *env, jobject, jobject state) {
  jclass cls = env->GetObjectClass(state);
  if( cls == NULL ) {
    printf("Can't find class org.eclipse.titan.executor.jni.McStateEnum\n");
  }
  jmethodID mid = env->GetMethodID(cls, "getValue", "()I");
  if( mid == NULL ) {
    printf("Can't find method getValue\n");
  }
  int value = env->CallIntMethod(state, mid);
  const char *state_name = MainController::get_mc_state_name((mctr::mc_state_enum)value);
  return (env->NewStringUTF(state_name));
}

JNIEXPORT jstring JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_get_1hc_1state_1name
  (JNIEnv *env, jobject, jobject state) {
  jclass cls = env->GetObjectClass(state);
  if( cls == NULL ) {
    printf("Can't find class org.eclipse.titan.executor.jni.HcStateEnum\n");
  }
  jmethodID mid = env->GetMethodID(cls, "getValue", "()I");
  if( mid == NULL ) {
    printf("Can't find method getValue\n");
  }
  int value = env->CallIntMethod(state, mid);
  const char *state_name = MainController::get_hc_state_name((mctr::hc_state_enum)value);
  return (env->NewStringUTF(state_name));
}

JNIEXPORT jstring JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_get_1tc_1state_1name
  (JNIEnv *env, jobject, jobject state) {
  jclass cls = env->GetObjectClass(state);
  if( cls == NULL ) {
    printf("Can't find class org.eclipse.titan.executor.jni.TcStateEnum\n");
  }
  jmethodID mid = env->GetMethodID(cls, "getValue", "()I");
  if( mid == NULL ) {
    printf("Can't find method getValue\n");
  }
  int value = env->CallIntMethod(state, mid);
  const char *state_name = MainController::get_tc_state_name((mctr::tc_state_enum)value);
  return (env->NewStringUTF(state_name));
}

JNIEXPORT jstring JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_get_1transport_1name
  (JNIEnv *env, jobject, jobject transport) {
  jclass cls = env->GetObjectClass(transport);
  if( cls == NULL ) {
    printf("Can't find class org.eclipse.titan.executor.jni.TransportTypeEnum\n");
  }
  jmethodID mid = env->GetMethodID(cls, "getValue", "()I");
  if( mid == NULL ) {
    printf("Can't find method getValue\n");
  }
  int value = env->CallIntMethod(transport, mid);
  const char *transport_name = MainController::get_transport_name((transport_type_enum)value);
  return (env->NewStringUTF(transport_name));
}

// *******************************************************
// Other native functions
// *******************************************************

JNIEXPORT void JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_check_1mem_1leak
  (JNIEnv *env, jobject, jstring program_name) {
  jboolean iscopy;
  const char *p_name = env->GetStringUTFChars(program_name, &iscopy);
  check_mem_leak(p_name);
  env->ReleaseStringUTFChars(program_name, p_name);
}

JNIEXPORT void JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_print_1license_1info
  (JNIEnv *, jobject) {
#ifdef LICENSE
  print_license_info();
#endif
}

JNIEXPORT jint JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_check_1license
  (JNIEnv *, jobject) {
  int max_ptcs;
#ifdef LICENSE
  license_struct lstr;
  init_openssl();
  load_license(&lstr);
  if (!verify_license(&lstr)) {
    free_license(&lstr);
    free_openssl();
    exit(EXIT_FAILURE);
  }
  if (!check_feature(&lstr, FEATURE_MCTR)) {
    fputs("The license key does not allow the starting of TTCN-3 "
      "Main Controller.\n", stderr);
    return 2;
  }
  max_ptcs = lstr.max_ptcs;
  free_license(&lstr);
  free_openssl();
#else
  max_ptcs = -1;
#endif
  return (jint)max_ptcs;
}

JNIEXPORT jstring JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_readPipe
  (JNIEnv *env, jobject) {
  char *buf = Jnimw::userInterface->read_pipe();
  return (env->NewStringUTF(buf));
}

JNIEXPORT jboolean JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_isPipeReadable
  (JNIEnv *, jobject) {
  return ((jboolean)Jnimw::userInterface->is_pipe_readable());
}

JNIEXPORT jlong JNICALL Java_org_eclipse_titan_executor_jni_JNIMiddleWare_getSharedLibraryVersion
  (JNIEnv *, jclass) {
  return (jlong) TTCN3_VERSION;
}
