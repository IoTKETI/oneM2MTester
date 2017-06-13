/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Forstner, Matyas
 *   Raduly, Csaba
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#include "memory.h"
#include "path.h"

/* Initial buffer size for getcwd */
#define BUFSIZE 1024

expstring_t get_working_dir(void)
{
    expstring_t ret_val = NULL;
    char buf[BUFSIZE];
    const char *buf_ptr;
    buf_ptr = getcwd(buf, sizeof(buf));
    if (buf_ptr != NULL) ret_val = mcopystr(buf_ptr);
    else if (errno == ERANGE) {
	/* the initial buffer size is not enough */
	size_t size;
	for (size = 2 * BUFSIZE; ; size *= 2) {
	    char *tmp = (char*)Malloc(size);
	    buf_ptr = getcwd(tmp, size);
	    if (buf_ptr != NULL) ret_val = mcopystr(buf_ptr);
	    Free(tmp);
	    if (buf_ptr != NULL || errno != ERANGE) break;
	}
    }
    if (ret_val == NULL) {
	/* an error occurred */
	path_error("Getting the current working directory failed: %s",
	    strerror(errno));
    }
    /* clear the possible error codes */
    errno = 0;
    return ret_val;
}

int set_working_dir(const char *new_dir)
{
    if (new_dir == NULL) {
	/* invalid argument */
	return 1;
    } else if (chdir(new_dir)) {
	/* an error occurred */
	path_error("Setting the current working directory to `%s' failed: %s",
	    new_dir, strerror(errno));
	errno = 0;
	return 1;
    } else {
	/* everything is OK */
	return 0;
    }
}

enum path_status_t get_path_status(const char *path_name)
{
    struct stat buf;
    if (stat(path_name, &buf)) {
      if (errno != ENOENT) {
	path_error("system call stat() failed on `%s': %s", path_name,
          strerror(errno));
      }
      errno = 0;
      return PS_NONEXISTENT;
    }
    if (S_ISDIR(buf.st_mode)) return PS_DIRECTORY;
    else return PS_FILE;
}

expstring_t get_dir_from_path(const char *path_name)
{
    size_t last_slash_index = (size_t)-1;
    size_t i;
    for (i = 0; path_name[i] != '\0'; i++)
      if (path_name[i] == '/') last_slash_index = i;
    if (last_slash_index == (size_t)-1) {
	/* path_name does not contain any slash */
	return NULL;
    } else if (last_slash_index == 0) {
	/* path_name has the format "/filename": return "/" */
	return mcopystr("/");
    } else {
	/* path_name has the format "<something>/filename":
	   return "<something>" */
	expstring_t ret_val = mcopystr(path_name);
	ret_val = mtruncstr(ret_val, last_slash_index);
	return ret_val;
    }
}

expstring_t get_file_from_path(const char *path_name)
{
    size_t last_slash_index = (size_t)-1;
    size_t i;
    for (i = 0; path_name[i] != '\0'; i++)
      if (path_name[i] == '/') last_slash_index = i;
    if (last_slash_index == (size_t)-1) {
	/* path_name does not contain any slash: return the entire input */
	return mcopystr(path_name);
    } else {
	/* path_name has the format "<something>/filename": return "filename" */
	return mcopystr(path_name + last_slash_index + 1);
    }
}

expstring_t compose_path_name(const char *dir_name,
    const char *file_name)
{
    if (dir_name != NULL && dir_name[0] != '\0') {
	expstring_t ret_val = mcopystr(dir_name);
	if (file_name != NULL && file_name[0] != '\0') {
	    /* neither dir_name nor file_name are empty */
	    size_t dir_name_len = strlen(dir_name);
	    /* do not add the separator slash if dir_name ends with a slash */
	    if (dir_name[dir_name_len - 1] != '/')
		ret_val = mputc(ret_val, '/');
	    ret_val = mputstr(ret_val, file_name);
	}
	return ret_val;
    } else return mcopystr(file_name);
}

expstring_t get_absolute_dir(const char *dir_name, const char *base_dir, const int with_error)
{
    expstring_t ret_val;
    /* save the working directory */
    expstring_t initial_dir = get_working_dir();
    if (base_dir != NULL && (dir_name == NULL || dir_name[0] != '/')) {
	/* go to base_dir first if it is given and dir_name is not an
	   absolute path */
	if (set_working_dir(base_dir)) {
	    Free(initial_dir);
	    return NULL;
	}
    }
    if (dir_name != NULL && with_error && set_working_dir(dir_name)) {
	/* there was an error: go back to initial_dir */
	set_working_dir(initial_dir);
	Free(initial_dir);
	return NULL;
    }
    if (dir_name != NULL && !with_error && chdir(dir_name)) {
        //No error sign
        errno = 0;
	Free(initial_dir);
	return NULL;
    }
    ret_val = get_working_dir();
    /* restore the working directory */
    set_working_dir(initial_dir);
    Free(initial_dir);
    if (ret_val != NULL &&
#if defined WIN32 && defined MINGW
	/* On native Windows the absolute path name shall begin with
	 * a drive letter, colon and backslash */
	(((ret_val[0] < 'A' || ret_val[0] > 'Z') &&
	  (ret_val[0] < 'a' || ret_val[0] > 'z')) ||
	 ret_val[1] != ':' || ret_val[2] != '\\')
#else
	/* On UNIX-like systems the absolute path name shall begin with
	 * a slash */
	ret_val[0] != '/'
#endif
	)
	path_error("Internal error: `%s' is not a valid absolute pathname.",
	    ret_val);
    return ret_val;
}

expstring_t get_relative_dir(const char *dir_name, const char *base_dir)
{
    expstring_t ret_val = NULL;
    /* canonize dir_name and the base directory */
    expstring_t canonized_dir_name = get_absolute_dir(dir_name, base_dir, 1);
    expstring_t canonized_base_dir = base_dir != NULL ?
	get_absolute_dir(base_dir, NULL, 1) : get_working_dir();
    size_t i, last_slash = 0;
    if (canonized_dir_name == NULL || canonized_base_dir == NULL) {
	/* an error occurred */
	Free(canonized_dir_name);
	Free(canonized_base_dir);
	return NULL;
    }
    /* skip over the common leading directory part of canonized_dir_name and
       canonized_base_dir */
    for (i = 1; ; i++) {
	char dir_c = canonized_dir_name[i];
	char base_c = canonized_base_dir[i];
	if (dir_c == '\0') {
	    /* we must update last_slash if dir_name is a parent of base_dir */
	    if (base_c == '/') last_slash = i;
	    /* we must stop anyway */
	    break;
	} else if (dir_c == '/') {
	    if (base_c == '/' || base_c == '\0') last_slash = i;
	    if (base_c != '/') break;
	} else {
	    if (dir_c != base_c) break;
	}
    }
    if (canonized_dir_name[i] == '\0' && canonized_base_dir[i] == '\0') {
	/* canonized_dir_name and canonized_base_dir are the same */
	ret_val = mcopystr(".");
    } else {
	if (canonized_base_dir[last_slash] == '/' &&
	    canonized_base_dir[last_slash + 1] != '\0') {
	    /* canonized_base_dir has some own additional components
	       (i.e. it is not the parent of canonized_dir_name) */
	    for (i = last_slash; canonized_base_dir[i] != '\0'; i++) {
		if (canonized_base_dir[i] == '/') {
		    /* go up one level in the relative path */
		    if (ret_val != NULL) ret_val = mputc(ret_val, '/');
		    ret_val = mputstr(ret_val, "..");
		}
	    }
	}
	if (canonized_dir_name[last_slash] == '/' &&
	    canonized_dir_name[last_slash + 1] != '\0') {
	    /* canonized_dir_name has some own additional components
	       (i.e. it is not the parent of canonized_base_dir) */
	    /* append the remaining parts of canonized_dir_name to the result */
	    if (ret_val != NULL) ret_val = mputc(ret_val, '/');
	    ret_val = mputstr(ret_val, canonized_dir_name + last_slash + 1);
	}
    }
    Free(canonized_dir_name);
    Free(canonized_base_dir);
    return ret_val;
}
