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
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef LICENSE_H
#define LICENSE_H

#include <time.h>

/* Initial features */
#define FEATURE_TTCN3		0x1u
#define FEATURE_CODEGEN		0x2u
#define FEATURE_TPGEN		0x4u
#define FEATURE_SINGLE		0x8u
#define FEATURE_MCTR		0x10u
#define FEATURE_HC		0x20u
#define FEATURE_LOGFORMAT	0x40u
/* Added in 1.2.pl0 */
#define FEATURE_ASN1		0x80u
#define FEATURE_RAW		0x100u
#define FEATURE_BER		0x200u
#define FEATURE_PER		0x400u
/* Added in 1.5.pl7 */
#define FEATURE_GUI		0x800u // deprecated  mctr GUI was removed
/* Added in 1.5.pl8 */
#define FEATURE_TEXT		0x1000u
/* Added in 1.8.pl0 */
#define FEATURE_XER             0x2000u

/* Limitation types */
#define LIMIT_HOST	0x1
#define LIMIT_USER	0x2

/* Limit for expiration warning in days */
#define EXPIRY_WARNING 14

/* Where to send the license.
 *
 * The values 1,2,3 work as either an enumeration or a bit field. */
#define SENDTO_LICENSEE 1
#define SENDTO_CONTACT  2
#define SENDTO_BOTH     3

/** "Cooked" license information suitable for processing. */
typedef struct {
    char *license_file;
    unsigned int unique_id;
    char *licensee_name, *licensee_email,
        *licensee_company, *licensee_department;
    char *contact_name, *contact_email;
    unsigned int  send_to;
    time_t valid_from, valid_until;
    unsigned long int host_id;
    char *login_name;
    unsigned int from_major, from_minor, from_patchlevel,
        to_major, to_minor, to_patchlevel;
    unsigned int feature_list, limitation_type;
    unsigned int max_ptcs;
} license_struct;

/** Raw license information.
 * 
 * The license file is a Base64-encode of a BER-encoded version of this structure.
 *
 * @note Modifying this structure will render it incompatible with
 * previous versions of Titan (the size of the structure is checked) */
typedef struct {
    unsigned char unique_id[4];
    char licensee_name[48], licensee_email[48],
        licensee_company[48], licensee_department[16];
    unsigned char valid_from[4], valid_until[4];
    unsigned char host_id[4];
    char login_name[8];
    unsigned char from_major[4], from_minor[4], from_patchlevel[4],
        to_major[4], to_minor[4], to_patchlevel[4];
    unsigned char feature_list[4], limitation_type[4];
    unsigned char max_ptcs[4];
    unsigned char dsa_signature[48];
} license_raw;

#ifdef __cplusplus
extern "C" {
#endif

/** Load license information.
 *
 * Loads license information from the file specified in the
 * \c TTCN3_LICENSE_FILE environment variable.
 *
 * Calls \c load_license_from_file()
 *
 * @param [out] lptr pointer to license data */
void load_license(license_struct *lptr);

/** Load license information from a specified file.
 * @param [out] lptr pointer to license data
 * @param [in] file_name string containing the license file name */
void load_license_from_file(license_struct *lptr, const char *file_name);

/** Free resources
 * @param [in] lptr pointer to license data */
void free_license(license_struct *lptr);

/** Check if the license allows the use of the product.
 *
 * Verifies that we are within the license period, that the host or user
 * restriction is satisfied, and the program version is within the limits
 * allowed by the license.
 *
 * If the license is about to expire in less than \c EXPIRY_WARNING days,
 * a warning is printed.
 *
 * If a check fails, function returns 0.
 *
 * @param [in] lptr pointer to license information
 * @return 1 if the license is valid, 0 if invalid. */
int verify_license(const license_struct *lptr);

/** Check if the license allows the use of a certain feature.
 *
 * @param lptr pointer to license information
 * @param feature feature to check. It should be one of the FEATURE_ macros.
 * @return nonzero if feature is enabled, 0 if disabled. */
unsigned int check_feature(const license_struct *lptr, unsigned int feature);

/** Print the details of the supplied license information
 *
 * @param [in] lptr pointer to license information */
void print_license(const license_struct *lptr);

/** Print information about the license used by the program.
 *
 * The license is loaded from the file specified by the \c TTCN3_LICENSE_FILE
 * environment variable.
 *
 * This function terminates the program if the license is not valid. */
void print_license_info(void);

void init_openssl(void);
void free_openssl(void);

#if defined(WIN32) || defined(INTERIX)
long gethostid(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
