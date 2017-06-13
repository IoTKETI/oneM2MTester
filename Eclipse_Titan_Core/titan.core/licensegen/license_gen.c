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
 *   Gecse, Roland
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <mysql.h>

#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/dsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>

#include "../common/memory.h"
#include "../common/license.h"

#define MYSQL_HOST "mwlx122.eth.ericsson.se"
#define MYSQL_USER "ttcn3"
#define MYSQL_PASSWORD "ttcn3"
#define MYSQL_DATABASE "ttcn3"

#define LICENSE_DIR "/mnt/TTCN/license"
#define PRIVATE_KEY LICENSE_DIR "/key.pem"

#define SECS_IN_DAY 86400

__attribute__ ((__format__ (__printf__, 1, 2), __noreturn__))
static void error(const char *fmt, ...)
{
    va_list pvar;
    fputs("ERROR: ", stderr);
    va_start(pvar, fmt);
    vfprintf(stderr, fmt, pvar);
    va_end(pvar);
    fputc('\n', stderr);
    exit(EXIT_FAILURE);
}

/* Big-endian */
static void encode_int(unsigned char *to, unsigned int from)
{
    to[3] = from & 0xFF;
    from >>= 8;
    to[2] = from & 0xFF;
    from >>= 8;
    to[1] = from & 0xFF;
    from >>= 8;
    to[0] = from & 0xFF;
}

static void encode_string(char *to, const char *from, size_t length)
{
    strncpy(to, from, length);
}

static void encode_license(license_raw *to, const license_struct *from)
{
    memset(to, 0, sizeof(*to));
    encode_int(to->unique_id, from->unique_id);
    encode_string(to->licensee_name, from->licensee_name,
	sizeof(to->licensee_name));
    encode_string(to->licensee_email, from->licensee_email,
        sizeof(to->licensee_email));
    encode_string(to->licensee_company, from->licensee_company,
        sizeof(to->licensee_company));
    encode_string(to->licensee_department, from->licensee_department,
        sizeof(to->licensee_department));
    encode_int(to->valid_from, from->valid_from);
    encode_int(to->valid_until, from->valid_until);
    encode_int(to->host_id, from->host_id);
    encode_string(to->login_name, from->login_name, sizeof(to->login_name));
    encode_int(to->from_major, from->from_major);
    encode_int(to->from_minor, from->from_minor);
    encode_int(to->from_patchlevel, from->from_patchlevel);
    encode_int(to->to_major, from->to_major);
    encode_int(to->to_minor, from->to_minor);
    encode_int(to->to_patchlevel, from->to_patchlevel);
    encode_int(to->feature_list, from->feature_list);
    encode_int(to->limitation_type, from->limitation_type);
    encode_int(to->max_ptcs, from->max_ptcs);
}

static void sign_license(license_raw *lptr, const char *dsa_key_file)
{
    unsigned char message_digest[20];
    SHA_CTX sha_ctx;
    unsigned int signature_len = sizeof(lptr->dsa_signature);
    DSA *dsa;

    FILE *fp = fopen(dsa_key_file, "r");
    if (fp == NULL) {
        error("Cannot open DSA private key file `%s' for reading: %s",
	    dsa_key_file, strerror(errno));
    }

    dsa = PEM_read_DSAPrivateKey(fp, NULL, NULL, NULL);
    fclose(fp);

    if (dsa == NULL) {
        error("Cannot read DSA private key from `%s': %s",
	    dsa_key_file, ERR_error_string(ERR_get_error(), NULL));
    }

    SHA1_Init(&sha_ctx);
    SHA1_Update(&sha_ctx, lptr, sizeof(*lptr) - sizeof(lptr->dsa_signature));
    SHA1_Final(message_digest, &sha_ctx);

    if ((int)signature_len != DSA_size(dsa)) {
	error("Invalid DSA signature size: %d", DSA_size(dsa));
    }

    if (!DSA_sign(0, message_digest, sizeof(message_digest),
        lptr->dsa_signature, &signature_len, dsa)) {
        error("DSA signature generation failed: %s",
	    ERR_error_string(ERR_get_error(), NULL));
    }

    DSA_free(dsa);
}

static void write_license(const char *file_name, const license_raw *lptr)
{
    FILE *fp = fopen(file_name, "w");
    if (fp == NULL) {
        error("Cannot open license file `%s' for writing: %s", file_name,
	    strerror(errno));
    }
    if (!PEM_write(fp, "TTCN-3 LICENSE FILE", "", (unsigned char *)lptr,
        sizeof(*lptr))) {
        error("Writing to license file `%s' failed: %s", file_name,
	    ERR_error_string(ERR_get_error(), NULL));
    }
    fclose(fp);
}

static int privileged_user = 0;
static uid_t my_uid = -1;

static void check_user(void)
{
    if (geteuid() != 45719) {
        error("This program must have set-uid to user etccadmi1 (uid 45719)");
    }
    my_uid = getuid();
    switch (my_uid) {
    case 45719: /* etccadmi1 */
	privileged_user = 1;
    case 34217: /* ethjra */
    case 34385: /* ethgasz */
        break;
    default:
        error("You are not allowed to use this program");
    }
}

static void connect_database(MYSQL *mysql)
{
    if (!mysql_init(mysql)) {
	error("MySQL structure initialization failed");
    }
    if (!mysql_real_connect(mysql, MYSQL_HOST, MYSQL_USER, MYSQL_PASSWORD,
	MYSQL_DATABASE, 0, NULL, 0)) {
	error("MySQL database connection failed: %s", mysql_error(mysql));
    } else {
	fputs("Connected to MySQL database.\n", stderr);
    }
}

static void disconnect_database(MYSQL *mysql)
{
    mysql_close(mysql);
    fputs("Disconnected from MySQL database.\n", stderr);
}

static void fill_from_database_c(MYSQL *mysql, license_struct *plic, int unique_id)
{
    MYSQL_RES *result;
    my_ulonglong num_rows;
    unsigned int i, num_fields;
    MYSQL_FIELD *fields;
    MYSQL_ROW row;
    time_t current_time = time(NULL);
    char *query = mprintf("SELECT * FROM licenses WHERE unique_id = %d", unique_id);
    if (mysql_query(mysql, query)) {
	error("Execution of MySQL query `%s' failed: %s", query,
	    mysql_error(mysql));
    }
    Free(query);

    result = mysql_store_result(mysql);
    if (result == NULL) {
	error("MySQL query result fetching failed: %s", mysql_error(mysql));
    }

    num_rows = mysql_num_rows(result);
    if (num_rows <= 0) {
	error("There is no license in the database with unique id %d",
	    unique_id);
    } else if (num_rows > 1) {
	error("There are %llu licenses in the database with unique id %d",
	    num_rows, unique_id);
    }

    num_fields = mysql_num_fields(result);
    fields = mysql_fetch_fields(result);
    row = mysql_fetch_row(result);

    fprintf(stderr, "License data was fetched for unique id %d.\n", unique_id);

    for (i = 0; i < num_fields; i++) {
        const char *field_name = fields[i].name;
        if (!strcmp(field_name, "unique_id")) {
	    plic->unique_id = atoi(row[i]);
	} else if (!strcmp(field_name, "licensee_name")) {
	    plic->licensee_name = row[i] != NULL ? row[i] : "";
	} else if (!strcmp(field_name, "licensee_email")) {
	    plic->licensee_email = row[i] != NULL ? row[i] : "";
	} else if (!strcmp(field_name, "contact_name")) {
	    plic->contact_name = row[i] != NULL ? row[i] : "";
	} else if (!strcmp(field_name, "contact_email")) {
	    plic->contact_email = row[i] != NULL ? row[i] : "";
	} else if (!strcmp(field_name, "send_to")) {
            if (!strcmp(row[i], "Contact")) plic->send_to = SENDTO_CONTACT;
            else if (!strcmp(row[i], "Both")) plic->send_to = SENDTO_BOTH;
            else /*if (!strcmp(row[i], "Licensee"))*/ plic->send_to = SENDTO_LICENSEE;
	    /* SENDTO_LICENSEE is the default */
	} else if (!strcmp(field_name, "licensee_company")) {
	    plic->licensee_company = row[i] != NULL ? row[i] : "";
	} else if (!strcmp(field_name, "licensee_department")) {
	    plic->licensee_department = row[i] != NULL ? row[i] : "";
	} else if (!strcmp(field_name, "valid_from")) {
	    int year, month, day;
	    struct tm tm_struct;
	    if (sscanf(row[i], "%4d-%2d-%2d", &year, &month, &day) != 3) {
		error("Invalid date format: `%s'", row[i]);
	    }
	    tm_struct.tm_year = year - 1900;
	    tm_struct.tm_mon = month - 1;
	    tm_struct.tm_mday = day;
	    tm_struct.tm_hour = 0;
	    tm_struct.tm_min = 0;
	    tm_struct.tm_sec = 0;
	    tm_struct.tm_isdst = -1;
	    plic->valid_from = mktime(&tm_struct);
	} else if (!strcmp(field_name, "valid_until")) {
	    int year, month, day;
	    struct tm tm_struct;
	    if (sscanf(row[i], "%4d-%2d-%2d", &year, &month, &day) != 3) {
		error("Invalid date format: `%s'", row[i]);
	    }
	    tm_struct.tm_year = year - 1900;
	    tm_struct.tm_mon = month - 1;
	    tm_struct.tm_mday = day;
	    tm_struct.tm_hour = 23;
	    tm_struct.tm_min = 59;
	    tm_struct.tm_sec = 59;
	    tm_struct.tm_isdst = -1;
	    plic->valid_until = mktime(&tm_struct);
	} else if (!strcmp(field_name, "host_id")) {
	    if (row[i] == NULL || sscanf(row[i], "%lx", &plic->host_id) != 1)
		plic->host_id = 0;
	} else if (!strcmp(field_name, "login_name")) {
	    plic->login_name = row[i] != NULL ? row[i] : "";
	} else if (!strcmp(field_name, "from_major")) {
	    plic->from_major = atoi(row[i]);
	} else if (!strcmp(field_name, "from_minor")) {
	    plic->from_minor = atoi(row[i]);
	} else if (!strcmp(field_name, "from_patchlevel")) {
	    plic->from_patchlevel = atoi(row[i]);
	} else if (!strcmp(field_name, "to_major")) {
	    plic->to_major = atoi(row[i]);
	} else if (!strcmp(field_name, "to_minor")) {
	    plic->to_minor = atoi(row[i]);
	} else if (!strcmp(field_name, "to_patchlevel")) {
	    plic->to_patchlevel = atoi(row[i]);
	} else if (!strcmp(field_name, "feature_ttcn3")) {
	    if (!strcmp(row[i], "Yes")) plic->feature_list |= FEATURE_TTCN3;
	} else if (!strcmp(field_name, "feature_asn1")) {
	    if (!strcmp(row[i], "Yes")) plic->feature_list |= FEATURE_ASN1;
	} else if (!strcmp(field_name, "feature_codegen")) {
	    if (!strcmp(row[i], "Yes")) plic->feature_list |= FEATURE_CODEGEN;
	} else if (!strcmp(field_name, "feature_raw")) {
	    if (!strcmp(row[i], "Yes")) plic->feature_list |= FEATURE_RAW;
	} else if (!strcmp(field_name, "feature_text")) {
	    if (!strcmp(row[i], "Yes")) plic->feature_list |= FEATURE_TEXT;
	} else if (!strcmp(field_name, "feature_ber")) {
	    if (!strcmp(row[i], "Yes")) plic->feature_list |= FEATURE_BER;
	} else if (!strcmp(field_name, "feature_per")) {
	    if (!strcmp(row[i], "Yes")) plic->feature_list |= FEATURE_PER;
	} else if (!strcmp(field_name, "feature_xer")) {
	    if (!strcmp(row[i], "Yes")) plic->feature_list |= FEATURE_XER;
	} else if (!strcmp(field_name, "feature_tpgen")) {
	    if (!strcmp(row[i], "Yes")) plic->feature_list |= FEATURE_TPGEN;
	} else if (!strcmp(field_name, "feature_single")) {
	    if (!strcmp(row[i], "Yes")) plic->feature_list |= FEATURE_SINGLE;
	} else if (!strcmp(field_name, "feature_mctr")) {
	    if (!strcmp(row[i], "Yes")) plic->feature_list |= FEATURE_MCTR;
	} else if (!strcmp(field_name, "feature_hc")) {
	    if (!strcmp(row[i], "Yes")) plic->feature_list |= FEATURE_HC;
	} else if (!strcmp(field_name, "feature_logformat")) {
	    if (!strcmp(row[i], "Yes")) plic->feature_list |= FEATURE_LOGFORMAT;
	} else if (!strcmp(field_name, "limit_host")) {
	    if (!strcmp(row[i], "Yes")) plic->limitation_type |= LIMIT_HOST;
	} else if (!strcmp(field_name, "limit_user")) {
	    if (!strcmp(row[i], "Yes")) plic->limitation_type |= LIMIT_USER;
	} else if (!strcmp(field_name, "max_ptcs")) {
	    plic->max_ptcs = atoi(row[i]);
	}
    } /* next */

    if (plic->valid_from > current_time) {
        error("The beginning of validity is in the future: %s",
	    ctime(&plic->valid_from));
    }

    if (plic->valid_until < current_time) {
        error("The license has already expired on %s",
	    ctime(&plic->valid_until));
    } else if (plic->valid_until < current_time + SECS_IN_DAY) {
	error("The license is valid for less than one day");
    } else if (plic->valid_until > current_time + 380 * SECS_IN_DAY &&
	       !privileged_user) {
        error("You are not authorized to generate a license that is valid for "
	    "more than one year (%ld days)",
	    (plic->valid_until - current_time) / SECS_IN_DAY);
    }

    if (plic->limitation_type & LIMIT_HOST && plic->host_id == 0) {
        error("Cannot generate HOST limited license for zero host ID");
    }

    if (plic->limitation_type & LIMIT_USER && !strcmp(plic->login_name, "")) {
        error("Cannot generate USER limited license for empty login name");
    }

    if (plic->limitation_type == 0 && !privileged_user) {
        error("You are not authorized to generate unrestricted licenses");
    }
    mysql_free_result(result);
}

static void fill_from_database(MYSQL *mysql, license_raw *lraw, int unique_id)
{

    license_struct lstr;
    memset(&lstr, 0, sizeof(lstr));

    fill_from_database_c(mysql, &lstr, unique_id);

    encode_license(lraw, &lstr);

}

static const char *get_email_address(void)
{
    switch (my_uid) {
    case 34217: /* ethjra */
	return "Julianna.Rozsa@ericsson.com";
    case 34385: /* ethgasz */
	return "Gabor.Szalai@ericsson.com";
    case 45719: /* etccadmi1 */
	return "Julianna.Rozsa@ericsson.com";
    default:
	return "???";
    }
}

static const char *get_first_name(void)
{
    switch (my_uid) {
    case 34217: /* ethjra */
	return "Julianna";
    case 34385: /* ethgasz */
	return "Gábor";
    case 45719: /* etccadmi1 */
	return "Admin";
    default:
	return "???";
    }
}

static const char *get_email_signature(void)
{
    switch (my_uid) {
    case 34217: /* ethjra */
	return
"Julianna Rózsa                           Ericsson Hungary Ltd.\n"
"Configuration Manager                    H-1117 Budapest, Irinyi József u.4-20.\n"
"Test Competence Center                   Phone: +36 1 437 7895\n"
"Julianna.Rozsa@ericsson.com              Fax:   +36 1 439 5176\n";
    case 34385: /* ethgasz */
	return
"Gábor Szalai                             Gabor.Szalai@ericsson.com\n"
"Test Competence Center                   Tel: +36-1-437-7591\n"
"Ericsson Telecommunications Ltd.         Fax: +36-1-439-5176\n"
"H-1037 Budapest, Laborc u. 1., Hungary   Mob: +36-30-743-7669\n"
"Intranet: http://ttcn.ericsson.se/       ECN: 831-7591\n";
    case 45719: /* etccadmi1 */
	return
"TCC License Administrator                Julianna.Rozsa@ericsson.com\n"
"Test Competence Center                   \n"
"Ericsson Telecommunications Ltd.         Fax: +36-1-439-5176\n"
"H-1037 Budapest, Laborc u. 1., Hungary   \n"
"Intranet: http://ttcn.ericsson.se/       \n";
    default:
	return "";
    }
}

static void mail_license(const license_struct *lstr, const license_raw *lptr)
{
    /* administrator's email address */
    const char *email_addr;
    /* licensee's email address, or empty if sent only to contact */
    const char *maybe_licensee_email = (SENDTO_CONTACT != lstr->send_to) ? lstr->licensee_email : "";
    /* contact's email address, or empty if sent only to licensee */
    const char *maybe_contact_email = (SENDTO_LICENSEE != lstr->send_to) ? lstr->contact_email : "";
    char *sendmail_cmd, *first_name, *mime_boundary;
    FILE *fp;
    size_t i;
    time_t current_time;
    struct tm *formatted_time;
    int return_status;

    email_addr = get_email_address();

    sendmail_cmd = mprintf("/usr/lib/sendmail %s %s %s", maybe_licensee_email, maybe_contact_email,
	email_addr);
    fp = popen(sendmail_cmd, "w");
    Free(sendmail_cmd);
    if (fp == NULL) error("Cannot mail license: %s", strerror(errno));

    first_name = memptystr();
    for (i = 0; lstr->licensee_name[i] != '\0'; i++) {
	if (lstr->licensee_name[i] == ' ') {
	    if (first_name[0] != '\0') break;
	} else first_name = mputc(first_name, lstr->licensee_name[i]);
    }
    mime_boundary = mcopystr("__");
    for (i = 0; i < 16; i++) {
	unsigned char uc;
	if (!RAND_pseudo_bytes(&uc, 1)) error("Random number generation "
	    "failed: %s", ERR_error_string(ERR_get_error(), NULL));
	mime_boundary = mputprintf(mime_boundary, "%02X", uc);
    }
    mime_boundary = mputstr(mime_boundary, "__");

    fprintf(fp, "From: TTCN-3 License Generator <%s>\n"
	"To: %s <%s>\n"
	"Subject: TTCN-3 license key\n"
	"MIME-Version: 1.0\n"
	"Content-Type: multipart/mixed; boundary=\"%s\"\n"
	"\n"
	"This is a multi-part message in MIME format.\n"
	"--%s\n"
	"Content-Type: text/plain; charset=ISO-8859-1\n"
	"\n"
        "Dear %s,\n"
	"\n", email_addr, lstr->licensee_name, lstr->licensee_email,
	mime_boundary, mime_boundary, first_name);
    Free(first_name);

    current_time = time(NULL);
    formatted_time = localtime(&lstr->valid_until);
    if (formatted_time == NULL) {
	error("Cannot format time: %s", strerror(errno));
    }
    if (current_time - lstr->valid_from < SECS_IN_DAY) {
	fprintf(fp, "this is your license key for the TTCN-3 "
	    "Test Executor. Please save the entire attachment "
	    "(including the header and trailing lines) and "
	    "follow the instructions of the Installation Guide "
	    "(Section 4.1.1) to proceed with the installation.\n"
	    "\n"
	    "The Unique ID of your license key is %d. Please refer to "
	    "this number if you have any licensing-related problems.\n"
	    "\n"
	    "The license key is valid till %04d-%02d-%02d beginning from today.\n"
	    "\n"
	    "Do not hesitate to contact us if you have any questions about "
	    "the TTCN-3 language or the test executor.\n", lstr->unique_id,
	    formatted_time->tm_year + 1900, formatted_time->tm_mon + 1,
	    formatted_time->tm_mday);
    } else {
	fprintf(fp, "your existing license key with Unique ID %d has been "
	    "prolonged so that it will expire on %04d-%02d-%02d. "
	    "Please find the updated license file in the attachment. "
	    "The only thing you have to do is to replace your old "
	    "license file with this one.\n", lstr->unique_id,
	    formatted_time->tm_year + 1900, formatted_time->tm_mon + 1,
	    formatted_time->tm_mday);
    }
    fprintf(fp, "\n"
	"With best regards,\n"
	"%s\n"
	"\n"
	"PS: This is an automatically generated e-mail, please reply to its "
	"sender in case of problems!\n"
	"\n", get_first_name());
    if (SENDTO_CONTACT == lstr->send_to) {
      fprintf (stderr, "%s, please forward this email to %s (%s)\n\n",
        lstr->contact_name, lstr->licensee_name, lstr->licensee_email);
    }
    fprintf(fp, "--\n"
	"%s"
        "--%s\n"
	"Content-Type: application/octet-stream\n"
	"Content-Transfer-Encoding: 7bit\n"
	"Content-Disposition: attachment; filename=\"license_%d.dat\"\n"
	"\n", get_email_signature(), mime_boundary,
	lstr->unique_id);
    if (!PEM_write(fp, "TTCN-3 LICENSE FILE", "", (unsigned char*)lptr,
	sizeof(*lptr))) {
	error("Attaching license file failed: %s",
	    ERR_error_string(ERR_get_error(), NULL));
    }
    fprintf(fp, "\n"
	"--%s--\n", mime_boundary);
    Free(mime_boundary);
    return_status = pclose(fp);
    if (!WIFEXITED(return_status) ||
	WEXITSTATUS(return_status) != EXIT_SUCCESS) {
	error("Sendmail invocation failed");
    }
}

static int get_unique_id(const char *str)
{
    int unique_id = atoi(str);
    if (unique_id <= 0) {
	error("Invalid unique id: %s", str);
    }
    return unique_id;
}

int main(int argc, char *argv[])
{
    license_struct lstr;
    license_raw lraw;
    char *file_name;
    int unique_id;
    MYSQL mysql;
    int send_email;

    fputs("License generator for the TTCN-3 Test Executor.\n", stderr);
    check_user();

    switch (argc) {
    case 2:
	unique_id = get_unique_id(argv[1]);
	send_email = 0;
	break;
    case 3:
	if (!strcmp(argv[1], "-m")) {
	    unique_id = get_unique_id(argv[2]);
	    send_email = 1;
	    break;
	}
	/* no break */
    default:
        error("Usage: %s [-m] unique_id", argv[0]);
    }

    init_openssl();

    connect_database(&mysql);
    fill_from_database(&mysql, &lraw, unique_id);
    disconnect_database(&mysql);

    sign_license(&lraw, PRIVATE_KEY);

    file_name = mprintf(LICENSE_DIR "/license_%d.dat", unique_id);
    write_license(file_name, &lraw);
    fprintf(stderr, "License file %s was generated.\n", file_name);

    fputs("License information:\n", stderr);
    load_license_from_file(&lstr, file_name);
    Free(file_name);
    print_license(&lstr);
    if (send_email) {
        license_struct lstr2;
        connect_database(&mysql);
        fill_from_database_c(&mysql, &lstr2, unique_id);
        disconnect_database(&mysql);
	mail_license(&lstr2, &lraw);
	fputs("E-mail was sent.\n", stderr);
        /* do not free_license(lstr2) */
    }
    free_openssl();
    free_license(&lstr);
    check_mem_leak(argv[0]);

    return EXIT_SUCCESS;
}
