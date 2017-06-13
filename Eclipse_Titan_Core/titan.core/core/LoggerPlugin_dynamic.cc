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
#include "LoggerPlugin.hh"
#include "ILoggerPlugin.hh"

#include <assert.h>
#include <dlfcn.h>

boolean str_ends_with(const char *str, const char *suffix)
{
  if (!str || !suffix) return FALSE;
  size_t lenstr = strlen(str);
  size_t lensuffix = strlen(suffix);
  if (lensuffix > lenstr) return FALSE;
  return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

enum SoType { RT1_SINGLE, RT1_PARALLEL, RT2_SINGLE, RT2_PARALLEL };

// determine the library type from the name ending
SoType get_so_type(const char* filename)
{
  if (str_ends_with(filename, "-rt2.so")) {
    return str_ends_with(filename, "-parallel-rt2.so") ? RT2_PARALLEL : RT2_SINGLE;
  } else { // rt1
    return str_ends_with(filename, "-parallel.so") ? RT1_PARALLEL : RT1_SINGLE;
  }
}

void LoggerPlugin::load()
{
  if (this->filename_) {
    // determine the required library
    boolean single_mode = TTCN_Runtime::is_single();
    #ifndef TITAN_RUNTIME_2
    const SoType required_so_type = single_mode ? RT1_SINGLE : RT1_PARALLEL;
    const char* required_suffix_str = single_mode ? ".so" : "-parallel.so";
    const char* required_runtime_str = single_mode ? "Load Test Single Mode Runtime" : "Load Test Parallel Mode Runtime";
    #else
    const SoType required_so_type = single_mode ? RT2_SINGLE : RT2_PARALLEL;
    const char* required_suffix_str = single_mode ? "-rt2.so" : "-parallel-rt2.so";
    const char* required_runtime_str = single_mode ? "Function Test Single Mode Runtime" : "Function Test Parallel Mode Runtime";
    #endif
    // if the provided filename ends with .so it is assumed to be a full file name,
    // otherwise it's assumed to be the base of the file name that has to be suffixed automatically
    expstring_t real_filename = mcopystr(this->filename_);
    if (str_ends_with(this->filename_,".so")) {
      // check if the filename is correct
      if (get_so_type(this->filename_)!=required_so_type) {
        TTCN_Logger::fatal_error("Incorrect plugin file name was provided (%s). "
          "This executable is linked with the %s, the matching plugin file name must end with `%s'. "
          "Note: if the file name ending is omitted it will be automatically appended.",
          this->filename_, required_runtime_str, required_suffix_str);
      }
    } else { // add the appropriate suffix if the filename extension was not provided
      real_filename = mputstr(real_filename, required_suffix_str);
    }

    // Dynamic plug-in. Try to resolve all symbols, die early with RTLD_NOW.
    this->handle_ = dlopen(real_filename, RTLD_NOW);
    if (!this->handle_) {
      TTCN_Logger::fatal_error("Unable to load plug-in %s with file name %s (%s)", this->filename_, real_filename, dlerror());
    }
    Free(real_filename);

    cb_create_plugin create_plugin =
      (cb_create_plugin)(unsigned long)dlsym(this->handle_, "create_plugin");
    if (!create_plugin) return;
    this->ref_ = (*create_plugin)();
  } else {
    // Static plug-in. We simply instantiate the class without any `dl*()'.
    assert(this->create_);
    this->ref_ = this->create_();
  }

  this->ref_->init();
  this->is_log2str_capable_ = this->ref_->is_log2str_capable();
}

// Completely destroy the logger plug-in and make it useless.  However,
// reloading is possible.  This should be called before the logger is
// deleted.
void LoggerPlugin::unload()
{
  if (!this->ref_) return;
  this->ref_->fini();
  if (this->filename_) {
    cb_destroy_plugin destroy_plugin =
      (cb_destroy_plugin)(unsigned long)dlsym(this->handle_,
                                              "destroy_plugin");
    if (destroy_plugin) {
      (*destroy_plugin)(this->ref_);
    }
    dlclose(this->handle_);
    this->handle_ = NULL;
  } else {
    // For static plug-ins, it's simple.
    delete this->ref_;
    this->create_ = NULL;
  }
  this->ref_ = NULL;
}

