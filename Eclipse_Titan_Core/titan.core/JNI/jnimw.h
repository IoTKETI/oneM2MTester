/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Lovassy, Arpad
 *
 ******************************************************************************/
#ifndef JNIMW_JNIMW_H
#define JNIMW_JNIMW_H
//----------------------------------------------------------------------------

#include <stdio.h>
#include <pthread.h>
#include <jni.h>
#include "../core/Types.h"
#include "../mctr2/mctr/UserInterface.h"
#include "../mctr2/mctr/MainController.h"
#include "../common/memory.h"
#include "../mctr2/mctr/config_data.h"

//----------------------------------------------------------------------------

namespace jnimw {

//----------------------------------------------------------------------------

/**
 * User interface jnimw implementation.
 */
class Jnimw : public mctr::UserInterface
{
public:
    static Jnimw *userInterface;
    int pipe_fd[2];
    char *pipe_buffer;
    fd_set readfds;

    static bool has_status_message_pending;
    static int pipe_size;

    static pthread_mutex_t mutex;

    /**
     * Configuration data which is filled by calling set_cfg_file()
     * Based on Cli::mycfg
     */
    static config_data mycfg;

public:
    /**
     * Constructor, destructor.
     */
    Jnimw();
    ~Jnimw();

    virtual int enterLoop(int argc, char* argv[]);

    /* Callback interface */
    /**
     * Status of MC has changed.
     */
    virtual void status_change();

    /**
     * Error message from MC.
     */
    virtual void error(int severity, const char* msg);

    /**
     * General notification from MC.
     */
    virtual void notify(const struct timeval* time, const char* source,
                        int severity, const char* msg);

    void create_pipe();
    void destroy_pipe();
    char* read_pipe();
    void write_pipe(const char *buf);

    bool is_pipe_readable();

    static void lock();
    static void unlock();
    static void fatal_error(const char *fmt, ...)
    __attribute__ ((__format__ (__printf__, 1, 2), __noreturn__));
};
//----------------------------------------------------------------------------

}

//----------------------------------------------------------------------------
#endif // JNIMW_JNIMW_H
