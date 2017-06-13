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
 *   Beres, Szabolcs
 *   Cserveni, Akos
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#ifndef MINGW
# include <pwd.h>
#endif
#include <errno.h>

#include <openssl/bn.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <openssl/dsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>

#ifdef WIN32
# include <windows.h>
# include <winreg.h>
# ifdef MINGW
#  include <lmcons.h>
# endif
# include <openssl/md5.h>
#endif

#ifdef INTERIX
# include <interix/interix.h>
# include <openssl/md5.h>
#endif

#include "memory.h"
#include "license.h"
#include "version.h"

#ifdef TTCN3_BUILDNUMBER
 /* In pre-release builds: */
#define TTCN3_VERSION_CHECK TTCN3_MAJOR * 1000000 + TTCN3_MINOR * 10000 + TTCN3_PATCHLEVEL * 100 + TTCN3_BUILDNUMBER
#else
 /* In official releases: */
#define TTCN3_VERSION_CHECK TTCN3_MAJOR * 10000   + TTCN3_MINOR * 100   + TTCN3_PATCHLEVEL
#endif

#if TTCN3_VERSION_CHECK != TTCN3_VERSION
#error TTCN3_VERSION in version.h does not match the value computed from TTCN3_MAJOR,TTCN3_MINOR,TTCN3_PATCHLEVEL,TTCN3_BUILDNUMBER
/* for debugging: these would appear in the preprocessed file */
static const int ttcn3_version       = TTCN3_VERSION;
static const int ttcn3_version_check = TTCN3_VERSION_CHECK;
#endif

static const unsigned char dsa_p[] = { 0x90, 0x63, 0x82, 0xae, 0x72, 0x71, 0xac, 0x14, 0xec, 0x9c, 0x71, 0x38, 0x9f, 0xda, 0x8d, 0x52, 0xbd, 0xb1, 0xa7, 0x1a, 0x19, 0xd4, 0x5f, 0xe7, 0x37, 0x3d, 0x44, 0xef, 0xce, 0xa1, 0x99, 0xfa, 0x85, 0xb6, 0x49, 0x77, 0xf1, 0x98, 0x39, 0x6b, 0x71, 0xce, 0x2, 0x42, 0x64, 0x4b, 0xd, 0xad, 0x83, 0xb0, 0x6b, 0x76, 0xba, 0xdc, 0x4f, 0xe0, 0x19, 0xf9, 0xc2, 0x79, 0x6e, 0xbb, 0xc, 0xab, 0x16, 0xae, 0xec, 0x56, 0x75, 0xff, 0x82, 0xb, 0x74, 0xd8, 0x96, 0x42, 0x23, 0x68, 0xf, 0xad, 0x27, 0xee, 0x4c, 0xbf, 0xf2, 0xd4, 0x49, 0x77, 0x8b, 0x1e, 0xf1, 0xdc, 0x5c, 0x4d, 0xfd, 0xa6, 0xd8, 0x5a, 0x70, 0x3f, 0x13, 0xd2, 0xed, 0x3f, 0x59, 0x9, 0x62, 0x2b, 0xb2, 0x8f, 0xcd, 0x7a, 0xa9, 0x3e, 0x6c, 0xb1, 0xe8, 0x80, 0x9d, 0xd2, 0x74, 0xc, 0xc8, 0xdf, 0xa, 0x40, 0xc9, 0xb3 },
    dsa_q[] = { 0xa3, 0xe2, 0x23, 0x73, 0xd3, 0x8a, 0x4b, 0x61, 0xd0, 0x60, 0x41, 0x21, 0x41, 0x6d, 0xc4, 0xf6, 0x8c, 0x5c, 0x89, 0x87 },
    dsa_g[] = { 0x40, 0x6d, 0xfb, 0x6d, 0xb6, 0x6, 0x32, 0xcc, 0xf0, 0xe9, 0x84, 0x16, 0x1e, 0xe1, 0x21, 0xcd, 0x34, 0xe7, 0xbb, 0x6c, 0x98, 0xff, 0xa9, 0xb9, 0xae, 0xe4, 0x6a, 0x61, 0x51, 0xf8, 0x66, 0x83, 0xa4, 0x34, 0x63, 0x81, 0xc4, 0x5f, 0xee, 0x85, 0x74, 0xee, 0x2a, 0x63, 0x9d, 0xcf, 0x97, 0x50, 0xb8, 0x9f, 0x76, 0xd9, 0xe, 0x58, 0xab, 0xac, 0x2e, 0x23, 0xac, 0x95, 0xc3, 0xb7, 0x14, 0xd6, 0x69, 0xff, 0x36, 0xef, 0xa4, 0xa9, 0xe1, 0xd6, 0x7a, 0xfd, 0x9d, 0x68, 0x91, 0xcf, 0x2d, 0xcd, 0x98, 0xc5, 0xe6, 0xf4, 0x1e, 0xde, 0xf8, 0x65, 0x6b, 0xeb, 0x80, 0x41, 0xab, 0xc7, 0x97, 0xcb, 0xbb, 0xc5, 0x5, 0x7, 0x22, 0x81, 0x58, 0x63, 0xf9, 0x67, 0xd4, 0x7c, 0xb6, 0x21, 0x17, 0xea, 0x62, 0xe3, 0xe8, 0x3f, 0x60, 0xb1, 0x51, 0x51, 0x4, 0xf2, 0x6f, 0x5c, 0x47, 0x69, 0x6b, 0xc1 },
    dsa_pub_key[] = { 0x76, 0xe, 0xf, 0x36, 0x77, 0x6, 0x9d, 0xb1, 0xf1, 0x4e, 0x9a, 0x95, 0xae, 0xc9, 0x39, 0xdf, 0x90, 0xd3, 0x94, 0x54, 0xf8, 0xf6, 0x89, 0xc8, 0x11, 0x8d, 0x2e, 0x92, 0x81, 0x6b, 0x2c, 0x37, 0x4, 0x9d, 0x97, 0x9a, 0x43, 0xa3, 0x2e, 0xed, 0x9a, 0x99, 0xb0, 0xcb, 0x9f, 0x4e, 0xea, 0x4f, 0xb7, 0xd5, 0x93, 0xc0, 0x86, 0x1b, 0xc7, 0x97, 0x5f, 0x5, 0xb4, 0xf1, 0xf9, 0xd6, 0x73, 0x97, 0x19, 0xe3, 0xc9, 0xda, 0xfc, 0x39, 0xe0, 0x37, 0xed, 0x7a, 0x62, 0xcb, 0xe3, 0x17, 0x9b, 0x64, 0x3c, 0x46, 0x86, 0x3f, 0x32, 0xec, 0x70, 0xab, 0x5b, 0x87, 0x5a, 0x6e, 0xc3, 0x37, 0xeb, 0x92, 0x58, 0x6c, 0x9e, 0x25, 0x1f, 0x37, 0x4b, 0xcd, 0xb5, 0x22, 0x62, 0x1d, 0x1b, 0x1c, 0xb1, 0x5d, 0xa1, 0xef, 0x50, 0xc3, 0x75, 0xff, 0x2, 0x24, 0x8c, 0xd7, 0x3b, 0x91, 0x77, 0xef, 0x94, 0x76 };



// Backward compatibility with older openssl versions
#if OPENSSL_VERSION_NUMBER < 0x10100000L

static int DSA_set0_pqg(DSA *d, BIGNUM *p, BIGNUM *q, BIGNUM *g)
{
   /* If the fields p, q and g in d are NULL, the corresponding input
    * parameters MUST be non-NULL.
    */
   if ((d->p == NULL && p == NULL)
       || (d->q == NULL && q == NULL)
       || (d->g == NULL && g == NULL))
       return 0;
   if (p != NULL) {
       BN_free(d->p);
       d->p = p;
   }
   if (q != NULL) {
       BN_free(d->q);
       d->q = q;
   }
   if (g != NULL) {
       BN_free(d->g);
       d->g = g;
   }
   return 1;
}

static int DSA_set0_key(DSA *d, BIGNUM *pub_key, BIGNUM *priv_key)
{
   /* If the field pub_key in d is NULL, the corresponding input
    * parameters MUST be non-NULL.  The priv_key field may
    * be left NULL.
    */
   if (d->pub_key == NULL && pub_key == NULL)
       return 0;
   if (pub_key != NULL) {
       BN_free(d->pub_key);
       d->pub_key = pub_key;
   }
   if (priv_key != NULL) {
       BN_free(d->priv_key);
       d->priv_key = priv_key;
   }
   return 1;
}

#endif


/** Big-endian decoding of a 4-byte integer */
static unsigned int decode_int(const unsigned char *from)
{
    return (unsigned int)from[3]
        | ((unsigned int)from[2] << 8)
        | ((unsigned int)from[1] << 16)
        | ((unsigned int)from[0] << 24);
}

/** Extract a string from a fixed-length field.
 *
 * @param from the beginning of the field
 * @param max_length the size of the field
 * @return a newly allocated string. The caller is responsible for
 * calling Free()
 *
 * Verifies that the unused portion of the field is properly zeroed out
 * (no non-NUL characters after the first NUL character). Terminates
 * the program with EXIT_FAILURE if there is trailing garbage
 * after the end of the string. */
static char *decode_string(const char *from, size_t max_length)
{
    size_t i, length;
    char *ptr;
    for (i = 0; i < max_length && from[i] != '\0'; i++);
    length = i;
    /* Verify that the tail is properly zeroed (no junk). */
    for (i++; i < max_length; i++)
	if (from[i] != '\0') {
            fputs("License file is corrupted: invalid string encoding\n",
		stderr);
	    exit(EXIT_FAILURE);
	}
    ptr = (char*)Malloc(length + 1);
    memcpy(ptr, from, length);
    ptr[length] = '\0';
    return ptr;
}

static void decode_license(license_struct *to, const license_raw *from)
{
    to->license_file = NULL;
    to->unique_id = decode_int(from->unique_id);
    to->licensee_name = decode_string(from->licensee_name,
        sizeof(from->licensee_name));
    to->licensee_email = decode_string(from->licensee_email,
        sizeof(from->licensee_email));
    to->licensee_company = decode_string(from->licensee_company,
        sizeof(from->licensee_company));
    to->licensee_department = decode_string(from->licensee_department,
        sizeof(from->licensee_department));
    to->valid_from = decode_int(from->valid_from);
    to->valid_until = decode_int(from->valid_until);
    to->host_id = decode_int(from->host_id);
    to->login_name = decode_string(from->login_name, sizeof(from->login_name));
    to->from_major = decode_int(from->from_major);
    to->from_minor = decode_int(from->from_minor);
    to->from_patchlevel = decode_int(from->from_patchlevel);
    to->to_major = decode_int(from->to_major);
    to->to_minor = decode_int(from->to_minor);
    to->to_patchlevel = decode_int(from->to_patchlevel);
    to->feature_list = decode_int(from->feature_list);

    /* Borrow the PER bit for this year */
    /* 1262300400 is Fri Jan  1 00:00:00 2010 */
    if((to->feature_list & FEATURE_PER) && (time(NULL) < 1262300400)) {
        to->feature_list |= FEATURE_XER;
    }

    to->limitation_type = decode_int(from->limitation_type);
    to->max_ptcs = decode_int(from->max_ptcs);
}

static void check_license_signature(license_raw *lptr)
{
    unsigned char message_digest[SHA_DIGEST_LENGTH];
    SHA_CTX sha_ctx;
    DSA *dsa = DSA_new();

    SHA1_Init(&sha_ctx);
    SHA1_Update(&sha_ctx, lptr, sizeof(*lptr) - sizeof(lptr->dsa_signature));
    SHA1_Final(message_digest, &sha_ctx);

    DSA_set0_pqg(dsa, BN_bin2bn(dsa_p, sizeof(dsa_p), NULL), BN_bin2bn(dsa_q, sizeof(dsa_q), NULL), BN_bin2bn(dsa_g, sizeof(dsa_g), NULL));
    DSA_set0_key(dsa, BN_bin2bn(dsa_pub_key, sizeof(dsa_pub_key), NULL), NULL);

  // calculate the right len of the signiture
  DSA_SIG *temp_sig=DSA_SIG_new();
  int siglen = -1;
  const unsigned char *data =lptr->dsa_signature; 
  if (temp_sig == NULL || d2i_DSA_SIG(&temp_sig,&data,sizeof(lptr->dsa_signature)) == NULL){
  	fprintf(stderr, "License signature verification failed: %s\n",
	    ERR_error_string(ERR_get_error(), NULL));
	  exit(EXIT_FAILURE);
  }
  unsigned char *tmp_buff= NULL;
  siglen = i2d_DSA_SIG(temp_sig, &tmp_buff);
  OPENSSL_cleanse(tmp_buff, siglen);
	OPENSSL_free(tmp_buff);
	DSA_SIG_free(temp_sig);

    switch(DSA_verify(0, message_digest, sizeof(message_digest),
        lptr->dsa_signature, siglen, dsa)) {
    case 0:
	fputs("License file is corrupted: invalid DSA signature.\n", stderr);
	exit(EXIT_FAILURE);
    case 1:
	break; /* valid signature */
    default:
	fprintf(stderr, "License signature verification failed: %s\n",
	    ERR_error_string(ERR_get_error(), NULL));
	exit(EXIT_FAILURE);
    }

    DSA_free(dsa);
}

/** Read a PEM-encoded license from a file.
 *
 * @param [in] file_name string containing license file name
 * @param [out] lptr filled with the decoded license information
 *
 * If the license information cannot be obtained (bad file name or wrong content),
 * terminates the program with EXIT_FAILURE. */
static void read_license(const char *file_name, license_raw *lptr)
{
    char *name = NULL;
    char *header = NULL;
    unsigned char *data = NULL;
    long len = 0;
    struct stat buf;
    FILE *fp;

    if (stat(file_name, &buf) != 0) {
        fprintf(stderr, "Cannot access license file `%s'. (%s)\n",
	    file_name, strerror(errno));
	exit(EXIT_FAILURE);
    }
    if (buf.st_mode & S_IFDIR) {
        fprintf(stderr, "The environment variable TTCN3_LICENSE_FILE was set "
	    "to `%s', which is a directory. It must be set to the file that "
	    "contains the license key.\n", file_name);
	exit(EXIT_FAILURE);
    }
    fp = fopen(file_name, "r");
    if (fp == NULL) {
        fprintf(stderr, "Cannot open license file `%s' for reading. (%s)\n",
	    file_name, strerror(errno));
	exit(EXIT_FAILURE);
    }
    if (!PEM_read(fp, &name, &header, &data, &len)) {
        fprintf(stderr, "License file is corrupted: %s\n",
	    ERR_error_string(ERR_get_error(), NULL));
	exit(EXIT_FAILURE);
    }
    fclose(fp);
    if (strcmp(name, "TTCN-3 LICENSE FILE") || strcmp(header, "")) {
        fputs("License file is corrupted: invalid header.\n", stderr);
	exit(EXIT_FAILURE);
    }
    if (len != sizeof(*lptr)) {
        fputs("License file is corrupted: invalid length.\n", stderr);
	exit(EXIT_FAILURE);
    }
    memcpy(lptr, data, sizeof(*lptr));
    OPENSSL_free(name);
    OPENSSL_free(header);
    OPENSSL_free(data);
}

void load_license(license_struct *lptr)
{
    const char *file_name = getenv("TTCN3_LICENSE_FILE");
    if (file_name == NULL) {
        fputs("TTCN3_LICENSE_FILE environment variable is not set.\n",
	    stderr);
	exit(EXIT_FAILURE);
    }
    load_license_from_file(lptr, file_name);
}

void load_license_from_file(license_struct *lptr, const char *file_name)
{
    license_raw lraw;
    read_license(file_name, &lraw);
    check_license_signature(&lraw);
    decode_license(lptr, &lraw);
    lptr->license_file = mcopystr(file_name);
}

void free_license(license_struct *lptr)
{
    Free(lptr->license_file);
    lptr->license_file = NULL;
    Free(lptr->licensee_name);
    lptr->licensee_name = NULL;
    Free(lptr->licensee_email);
    lptr->licensee_email = NULL;
    Free(lptr->licensee_company);
    lptr->licensee_company = NULL;
    Free(lptr->licensee_department);
    lptr->licensee_department = NULL;
    Free(lptr->login_name);
    lptr->login_name = NULL;
}

int verify_license(const license_struct *lptr)
{
    time_t current_time = time(NULL);
    int errflag = 0;

    if (current_time < lptr->valid_from) {
        fputs("The license key is not yet valid.\n", stderr);
	errflag = 1;
    }

    if (current_time > lptr->valid_until) {
        fputs("The license key has already expired.\n", stderr);
	errflag = 1;
    }

    if (lptr->limitation_type & LIMIT_HOST) {
	/* broken libc call gethostid() performs sign extension on some 64-bit
	 * Linux platforms, thus only the lowest 32 bits are considered */
	unsigned long host_id = (unsigned long)gethostid() & 0xffffffffUL;
	if (host_id != lptr->host_id) {
	    fprintf(stderr, "The license key is not valid for this host "
		"(%08lx).\n", host_id);
	    errflag = 1;
        }
    }

    if (lptr->limitation_type & LIMIT_USER) {
#ifdef MINGW
	TCHAR user_name[UNLEN + 1];
	DWORD buffer_size = sizeof(user_name);
	if (GetUserName(user_name, &buffer_size)) {
	    if (strcmp(user_name, lptr->login_name)) {
		fprintf(stderr, "The license key is not valid for this user "
		    "name (%s).\n", user_name);
		errflag = 1;
	    }
	} else {
	    fprintf(stderr, "Getting the current user name failed when "
		"verifying the license key. Windows error code: %d.\n",
		(int)GetLastError());
	    errflag = 1;
	}
#else
	uid_t process_uid = getuid();
        struct passwd *p;
	setpwent();
	p = getpwuid(process_uid);
	if (p == NULL) {
	    fprintf(stderr, "The current user ID (%d) does not have login "
		"name.\n", (int)process_uid);
	    errflag = 1;
	} else if (strcmp(p->pw_name, lptr->login_name)) {
	    /* First making a backup copy of the current login name because
	     * the subsequent getpwnam() call will overwrite it. */
	    char *login_name = mcopystr(p->pw_name);
	    /* Another chance: Trying to map the login name of the license key
	     * to a valid UID. Note that it is possible to associate several
	     * login names with the same UID. */
	    p = getpwnam(lptr->login_name);
	    if (p == NULL || p->pw_uid != process_uid) {
		fprintf(stderr, "The license key is not valid for this login "
		    "name (%s).\n", login_name);
		errflag = 1;
	    }
	    Free(login_name);
	}
	endpwent();
#endif
    }

    if (TTCN3_MAJOR < lptr->from_major ||
        (TTCN3_MAJOR == lptr->from_major && (TTCN3_MINOR < lptr->from_minor ||
	(TTCN3_MINOR == lptr->from_minor &&
	TTCN3_PATCHLEVEL < lptr->from_patchlevel)))) {
        /* Checking of to_{major,minor,patchlevel} removed when Titan moved
         * to major version 2 (licenses were valid up to 1.99pl99 only) */
            fputs("The license key is not valid for this version.\n", stderr);
	    errflag = 1;
    }
    
    if (errflag) {
      return 0;
    }

    if (lptr->valid_until - current_time < EXPIRY_WARNING * 86400) {
	time_t expiry_days = (lptr->valid_until - current_time) / 86400 + 1;
	fprintf(stderr, "Warning: The license key will expire within %ld "
	    "day%s.\n", (long)expiry_days, expiry_days > 1 ? "s" : "");
    }
    /* setpwent and getpwuid calls may use some system calls that fail.
     * We should ignore these error codes in future error messages. */
    errno = 0;
    return 1;
}

unsigned int check_feature(const license_struct *lptr, unsigned int feature)
{
    return (lptr->feature_list & feature) != 0;
}

void print_license(const license_struct *lptr)
{
    fprintf(stderr,
	"---------------------------------------------------------------\n"
	"License file : %s\n"
	"Unique ID    : %d\n"
	"Licensee     : %s\n"
	"E-mail       : %s\n"
	"Company      : %s\n"
	"Department   : %s\n"
	"Valid from   : %s",
	lptr->license_file != NULL ? lptr->license_file : "",
	lptr->unique_id,
	lptr->licensee_name,
	lptr->licensee_email,
	lptr->licensee_company,
	lptr->licensee_department,
	ctime(&lptr->valid_from));

    fprintf(stderr,
	"Valid until  : %s"
	"Limitation   :%s%s\n"
	"Host ID      : %08lx\n"
	"Login name   : %s\n"
	"Versions     : from %d.%d.pl%d until %d.%d.pl%d\n"
	"Languages    :%s%s\n"
	"Encoders     :%s%s%s%s%s\n"
	"Applications :%s%s%s%s%s%s\n"
	"Max PTCs     : %d\n"
	"---------------------------------------------------------------\n",
	ctime(&lptr->valid_until),
	lptr->limitation_type & LIMIT_HOST ? " HOST" : "",
	lptr->limitation_type & LIMIT_USER ? " USER" : "",
	lptr->host_id,
	lptr->login_name,
	lptr->from_major, lptr->from_minor, lptr->from_patchlevel,
	lptr->to_major, lptr->to_minor, lptr->to_patchlevel,
	lptr->feature_list & FEATURE_TTCN3 ? " TTCN3" : "",
	lptr->feature_list & FEATURE_ASN1 ? " ASN1" : "",
	lptr->feature_list & FEATURE_RAW ? " RAW" : "",
	lptr->feature_list & FEATURE_TEXT ? " TEXT" : "",
	lptr->feature_list & FEATURE_BER ? " BER" : "",
	lptr->feature_list & FEATURE_PER ? " PER" : "",
	lptr->feature_list & FEATURE_XER ? " XER" : "",
	lptr->feature_list & FEATURE_CODEGEN ? " CODEGEN" : "",
	lptr->feature_list & FEATURE_TPGEN ? " TPGEN" : "",
	lptr->feature_list & FEATURE_SINGLE ? " SINGLE" : "",
	lptr->feature_list & FEATURE_MCTR ? " MCTR" : "",
	lptr->feature_list & FEATURE_HC ? " HC" : "",
	lptr->feature_list & FEATURE_LOGFORMAT ? " LOGFORMAT" : "",
	lptr->max_ptcs);
}

void print_license_info()
{
    license_struct lstr;
    int license_valid;
    fputs("License information:\n", stderr);
    init_openssl();
    load_license(&lstr);
    print_license(&lstr);
    license_valid = verify_license(&lstr);
    free_license(&lstr);
    free_openssl();

    if (!license_valid) {
      exit(EXIT_FAILURE);
    }

    fputs("The license key is valid.\n", stderr);
}

void init_openssl()
{
    if (!RAND_status()) {
	time_t time_sec = time(NULL);
	if (time_sec == (time_t)-1) {
            perror("time() system call failed");
	    exit(EXIT_FAILURE);
	}
	RAND_seed(&time_sec, sizeof(time_sec));
    }
    while (!RAND_status()) {
#ifdef MINGW
	FILETIME filetime;
	GetSystemTimeAsFileTime(&filetime);
	RAND_seed(&filetime.dwLowDateTime, sizeof(filetime.dwLowDateTime));
#else
        struct timeval tv;
	if (gettimeofday(&tv, NULL) == -1) {
	    perror("gettimeofday() system call failed");
	    exit(EXIT_FAILURE);
        }
	RAND_seed(&tv.tv_usec, sizeof(tv.tv_usec));
#endif
    }
    ERR_load_crypto_strings();
    /* Random seeding in OpenSSL may use some system calls that fail.
     * We should ignore these error codes in future error messages. */
    errno = 0;
}

void free_openssl()
{
    RAND_cleanup();
    ERR_free_strings();
}

#if defined(WIN32) || defined(INTERIX)

#ifdef INTERIX
#define INTERIX_PREFIX "\\Registry\\Machine\\"
#else
#define INTERIX_PREFIX
#endif

static const char * const keys[] = {
    INTERIX_PREFIX "Software\\Microsoft\\Windows\\CurrentVersion\\",
    INTERIX_PREFIX "Software\\Microsoft\\Windows NT\\CurrentVersion\\",
    NULL
};

static const char * const values[] = {
/* Product specific info */
    "ProductName",
    "Version",
    "CurrentVersion",
    "VersionNumber",
    "CurrentBuildNumber",
/* Unique identifiers */
    "ProductKey",
    "ProductId",
    "DigitalProductId",
/* User specific info */
    "RegisteredOwner",
    "RegisteredOrganization",
/* Installation specific info */
    "InstallDate",
    "FirstInstallDateTime",
    "SystemRoot",
    NULL
};
#endif

#ifdef WIN32

long gethostid(void)
{
    const char * const *subKey;
    MD5_CTX context;
    unsigned char digest[MD5_DIGEST_LENGTH];
    long hostid = 0;
    unsigned int i;

    MD5_Init(&context);

    for (subKey = keys; *subKey != NULL; subKey++) {
	HKEY hKey;
	const char * const *valueName;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, *subKey,
	    0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
	    continue;

	for (valueName = values; *valueName != NULL; valueName++) {
	    DWORD length = 0;
	    unsigned char *buffer;

	    if (RegQueryValueEx(hKey, *valueName, NULL, NULL, NULL, &length)
		!= ERROR_SUCCESS) continue;
	    buffer = (unsigned char*)Malloc(length);
	    if (RegQueryValueEx(hKey, *valueName, NULL, NULL, buffer, &length)
		== ERROR_SUCCESS) {
		MD5_Update(&context, buffer, length);
	    }
	    Free(buffer);
	}
	RegCloseKey(hKey);
    }
    MD5_Final(digest, &context);
    for (i = 0; i < sizeof(hostid); i++) hostid |= digest[i] << i * 8;
    return hostid;
}

#endif

#ifdef INTERIX

long gethostid(void)
{
    long hostid = 0;
    const char * const *subKey;
    size_t i;
    MD5_CTX context;
    unsigned char digest[MD5_DIGEST_LENGTH];

    MD5_Init(&context);

    for (subKey = keys; *subKey != NULL; subKey++) {
	const char * const *valueName;
	for (valueName = values; *valueName != NULL; valueName++) {
	    int ret = -13, type = -42;
	    size_t size = 0;
	    unsigned char *buffer;
            char * key = mcopystr(*subKey);
	    key = mputstr(key, *valueName);

	    ret = getreg(key, &type, 0, &size); /* query the size; ret always -1 */
	    if (size > 0) {
		buffer = Malloc(size);
		ret = getreg(key, &type, buffer, &size);
		if (ret == 0) {
		    switch (type) {
		    case WIN_REG_SZ:
		    case WIN_REG_EXPAND_SZ:
		    case WIN_REG_MULTI_SZ: {
		        /* getreg gave use _wide_ strings. Make them narrow.
			FIXME this assumes everybody is american and uses US-ASCII.
			It would be more correct to use iconv()
			but we don't know which codepage to convert to.
			*/
		        const unsigned char *from = buffer, *end = buffer + size;
			unsigned char *to = buffer;
			for (; from < end; ++from) { /* Yes, from is incremented twice */
			  *to++ = *from++;
			}
			size /= 2;
		    	break; }
		    default:
		    	break;
		    }
		    MD5_Update(&context, buffer, size);
		}
		Free(buffer);
	    }
	    Free(key);
	}
    }

    MD5_Final(digest, &context);
    for (i = 0; i < sizeof(hostid); i++) hostid |= digest[i] << (i * 8);
    return hostid;
}

#endif
