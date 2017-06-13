/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Raduly, Csaba
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef _Common_path_H
#define _Common_path_H

#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Error handling function that shall be provided by the application
 * that uses this library. The meaning of argument(s) is the same as in
 * \c printf() */
extern void path_error(const char *fmt, ...)
    __attribute__ ((__format__ (__printf__, 1, 2)));

/** Returns the current working directory of the process in canonical form.
 * The string returned shall be deallocated by the caller using \a Free(). */
extern expstring_t get_working_dir(void);

/** Sets the current working directory of the process to \a new_dir.
 * Returns 0 on success, function \a path_error() is called and non-zero value
 * is returned in case of any error. If \a new_dir is NULL the unsuccessful
 * status code is simply returned, \a path_error() is not called. */
extern int set_working_dir(const char *new_dir);

enum path_status_t {
    PS_FILE, /**< the pathname is a file */
    PS_DIRECTORY, /**< the pathname is a directory */
    PS_NONEXISTENT /**< the pathname does not exist */
};

/** Returns the status of \a path_name. Symbolic links are followed.
 * In case of any problem other than non-existent file or directory
 * function \a path_error() is called. */
extern enum path_status_t get_path_status(const char *path_name);

/** Returns the directory part of \a path_name. It is assumed that the
 * argument points to a file. NULL pointer is returned if \a path_name is a
 * simple file name without any slash. The string returned shall be
 * deallocated by the caller using \a Free(). */
extern expstring_t get_dir_from_path(const char *path_name);

/** Returns the file name part of \a path_name. It is assumed that the
 * argument points to a file. NULL pointer is returned if \a path_name ends
 * with a slash. The string returned shall be deallocated by the caller using
 * \a Free(). */
extern expstring_t get_file_from_path(const char *path_name);

/** Concatenates the given directory \a dir_name and file name \a file_name
 * to get a path name. If either \a dir_name or \a file_name is NULL or empty
 * string the resulting path name will contain only the other component. The
 * slash is inserted between \a dir_name and \a file_name only when necessary.
 * The string returned shall be deallocated by the caller using \a Free(). */
extern expstring_t compose_path_name(const char *dir_name,
    const char *file_name);

/** Converts \a dir_name, which is relative to \a base_dir, to an absolute
 * directory path. If \a base_dir is NULL the current working directory of
 * the process is used. It is assumed that both \a dir_name and \a base_dir
 * are existing directories. The returned directory name is in canonical form
 * (i.e. symlinks in it are resolved). NULL pointer returned in case of error.
 * The string returned shall be deallocated by the caller using \a Free().
 * Note: The working directory of the current process might change during the
 * function call, but it is restored before the function returns.
 * If the with_error is true, then it won't sign error when set_working_dir
 * is called.*/
extern expstring_t get_absolute_dir(const char *dir_name, const char *base_dir, const int with_error);

/** Converts \a dir_name to a relative path name based on \a working_dir. If
 * \a working_dir is NULL the current working directory of the process is used.
 * It is assumed that both \a dir_name and \a working_dir are existing
 * directories. NULL pointer is returned in case of any error.
 * The string returned shall be deallocated by the caller using \a Free().*/
extern expstring_t get_relative_dir(const char *dir_name,
    const char *working_dir);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _Common_path_H */
