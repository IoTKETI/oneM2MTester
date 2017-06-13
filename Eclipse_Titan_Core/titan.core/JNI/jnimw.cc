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
#include "jnimw.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <string.h>
#include <errno.h>

#include "../core/Logger.hh"

using mctr::MainController;
using namespace jnimw;

Jnimw *Jnimw::userInterface;
bool Jnimw::has_status_message_pending;
int Jnimw::pipe_size;

/**
 * The last MC state. It is needed by status_change(), as
 * status change message is written to the pipe when any status (MC, TC, HC) was changed AND
 *  ( currently there is no status change message on the pipe (signalled by has_status_message_pending)
 *  OR MC state is changed )
 */
mctr::mc_state_enum last_mc_state;

pthread_mutex_t Jnimw::mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Config data, which was created by Java_org_eclipse_titan_executor_jni_JNIMiddleWare_set_1cfg_1file()
 * by a JNI request, and the result will be used by
 * Java_org_eclipse_titan_executor_jni_JNIMiddleWare_configure().
 * This is done this way to use process_config_read_file() for processing the config file
 * instead of processing it on the Java side.
 */
config_data Jnimw::mycfg;

void Jnimw::lock()
{
  int result = pthread_mutex_lock(&mutex);
  if (result > 0) {
    fatal_error("Jni middleware::lock: "
    "pthread_mutex_lock failed with code %d.", result);
  }
}

void Jnimw::unlock()
{
  int result = pthread_mutex_unlock(&mutex);
  if (result > 0) {
    fatal_error("Jni middleware:::unlock: "
    "pthread_mutex_unlock failed with code %d.", result);
  }
}

void Jnimw::fatal_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    if (errno != 0) fprintf(stderr, " (%s)", strerror(errno));
    putc('\n', stderr);
    exit(EXIT_FAILURE);
}

int Jnimw::enterLoop(int, char*[]) {
  return EXIT_SUCCESS;
}

Jnimw::Jnimw()
{
  pipe_buffer = NULL;
  pipe_fd[0] = -1;
  pipe_fd[1] = -1;

  create_pipe();
  FD_ZERO(&readfds);
  FD_SET(pipe_fd[0], &readfds);

  has_status_message_pending = false;
  last_mc_state = mctr::MC_INACTIVE;
  pipe_size = 0;

  if (pthread_mutex_init(&mutex, NULL))
  fatal_error("Jni middleware::constructor: pthread_mutex_init failed.");

}

Jnimw::~Jnimw()
{
  destroy_pipe();
  pthread_mutex_destroy(&mutex);
}


void strreverse(char* begin, char* end) {
  char aux;
  while(end>begin){
    aux=*end, *end--=*begin, *begin++=aux;
  }
}

/**
 * Ansi C "itoa" based on Kernighan & Ritchie's "Ansi C":
 */
void itoa(int value, char* str) {
  static char num[] = "0123456789";
  char* wstr=str;

  // Conversion. Number is reversed.
  do *wstr++ = num[value%10]; while(value/=10);
  *wstr='\0';

  // Reverse string
  strreverse(str,wstr-1);
}

void create_packet_header(const int source_length, char* dest, char method_id) {
  char packet_size[6];
  dest[0] = method_id;
  itoa(source_length, packet_size);
  int i;
  for(i = 1; i < 6; i++) dest[i] = '0';
  dest[6] = '\0';
  int j = strlen(packet_size);
  for(i = 0; i < j; i++) dest[5-i] = packet_size[j-i-1];
}

char* stuffer(const char* msg){
  char* msg_stuffed = (char*) malloc(strlen(msg)*2);
  int i = 0;
  int j = 0;
  while(msg[i] != '\0') {
    if(msg[i] != '|' && msg[i] != '\\') {
      msg_stuffed[j++] = msg[i];
    } else {
      msg_stuffed[j++] = '\\';
      msg_stuffed[j++] = msg[i];
    }
    i++;
  }
  msg_stuffed[j] = '\0';

  return msg_stuffed;
}

//----------------------------------------------------------------------------
// USERINTERFACE

void Jnimw::status_change()
{
  lock();
  mctr::mc_state_enum mc_state = MainController::get_state();
  if(last_mc_state != mc_state || !has_status_message_pending){
    char str[7];
    sprintf( str,"S%02d000", mc_state );
    write_pipe( str );
  }
  has_status_message_pending = true;
  last_mc_state = mc_state;
  unlock();
}

//----------------------------------------------------------------------------
// USERINTERFACE

void Jnimw::error(int severity, const char* msg)
{
  char *msg_stuffed = stuffer(msg);
  expstring_t pipe_s;

  // creating packet header
  char packet_header[7];
  expstring_t tmp;
  tmp = mprintf("%d|%s", severity, msg_stuffed);
  create_packet_header(strlen(tmp), packet_header, 'E');

  pipe_s = mprintf("%s%s", packet_header, tmp);
  free(msg_stuffed);

  write_pipe(pipe_s);
}

//----------------------------------------------------------------------------
// USERINTERFACE

void Jnimw::notify(const struct timeval* time, const char* source,
                 int severity, const char* msg)
{
  char *source_stuffed = stuffer(source);
  char *msg_stuffed = stuffer(msg);
  expstring_t pipe_s;

  // creating packet header
  char packet_header[7];
  expstring_t tmp;
  tmp = mprintf("%ld|%ld|%s|%d|%s", time->tv_sec, time->tv_usec, source_stuffed, severity, msg_stuffed);
  create_packet_header(strlen(tmp), packet_header, 'N');

  pipe_s = mprintf("%s%s", packet_header, tmp);
  write_pipe(pipe_s);
  free(source_stuffed);
  free(msg_stuffed);
  Free(tmp);
  Free(pipe_s);
}

void Jnimw::create_pipe()
{
  if (pipe(pipe_fd)){
    printf("Jnimw::create_pipes(): pipe system call failed.\n");
  }
}

void Jnimw::destroy_pipe()
{
  close(pipe_fd[0]);
  pipe_fd[0] = -1;
  close(pipe_fd[1]);
  pipe_fd[1] = -1;
}

bool Jnimw::is_pipe_readable(){
  // TODO maybe this could get faster
  timeval time;
  time.tv_sec = 0;
  time.tv_usec = 0;
  fd_set read_set;
  FD_ZERO(&read_set);
  FD_SET(pipe_fd[0], &read_set);
  int ret = select(pipe_fd[0] + 1 , &read_set, NULL, NULL, &time);
  return ret > 0;
}

char* Jnimw::read_pipe()
{
  select(pipe_fd[0] + 1 , &readfds, NULL, NULL, NULL);
  lock();

  pipe_buffer = (char*)malloc(7);
  int ret = read(pipe_fd[0], pipe_buffer, 6);
  if(ret != 6){
    printf("Malformed packet arrived!\n");
  }

  pipe_size-= ret;

  if(pipe_buffer[0] == 'S'){
    has_status_message_pending = false;

    unlock();
    return pipe_buffer;
  }

  int packet_size = (pipe_buffer[1]-48) * 10000 + (pipe_buffer[2]-48) * 1000 +
                    (pipe_buffer[3]-48) * 100 + (pipe_buffer[4]-48) * 10 + (pipe_buffer[5]-48);

  pipe_buffer = (char*)realloc(pipe_buffer, packet_size + 7);

  ret = read(pipe_fd[0],pipe_buffer + 6, packet_size);
  if(ret != packet_size){
    printf("Jnimw::read_pipe(): read system call failed\n");
  }
  pipe_buffer[packet_size + 6] = '\0';

  pipe_size-=ret;

  unlock();
  return pipe_buffer;
}

void Jnimw::write_pipe(const char *buf)
{
  if (write(pipe_fd[1], buf, strlen(buf)) < 0){
    printf("Jnimw::write_pipe(): write system call failed\n");
  }

  pipe_size+=strlen(buf);
}
