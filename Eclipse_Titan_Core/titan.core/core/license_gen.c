/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#include <stdio.h>

#include "license.h"

int main(int argc, char *argv[])
{
    license_struct lstr;
    license_raw lraw;
    struct tm tm_struct;
    
    init_openssl();
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s private_key.pem\n", argv[0]);
	return 1;
    }
    
    lstr.unique_id = 1;
    lstr.licensee_name = "Janos Zoltan Szabo";
    lstr.licensee_email = "Szabo.Janos@eth.ericsson.se";
    lstr.licensee_company = "Conformance Lab, "
        "Ericsson Hungary Ltd.";
    lstr.licensee_department = "ETH/RL/S";
    
    tm_struct.tm_year = 2001 - 1900;
    tm_struct.tm_mon = 1 - 1;
    tm_struct.tm_mday = 1;
    tm_struct.tm_hour = 0;
    tm_struct.tm_min = 0;
    tm_struct.tm_sec = 0;
    lstr.valid_from = mktime(&tm_struct);

    tm_struct.tm_year = 2001 - 1900;
    tm_struct.tm_mon = 12 - 1;
    tm_struct.tm_mday = 31;
    tm_struct.tm_hour = 23;
    tm_struct.tm_min = 59;
    tm_struct.tm_sec = 59;
    lstr.valid_until = mktime(&tm_struct);
    
    lstr.host_id = 0x80b33fd0;
    lstr.login_name = "tmpjsz";
    lstr.from_major = 1;
    lstr.from_minor = 1;
    lstr.from_patchlevel = 0;
    lstr.to_major = 1;
    lstr.to_minor = 1;
    lstr.to_patchlevel = 99;
    
    lstr.feature_list = TTCN3_PARSER |
        TTCN3_COMPILER |
	TTCN3_TPGEN |
	TTCN3_SINGLE |
	TTCN3_MCTR |
	TTCN3_HC |
	TTCN3_LOGFORMAT;
    
    lstr.limitation_type = LIMIT_HOST | LIMIT_USER;
    
    lstr.max_ptcs = 1000;
    
    encode_license(&lraw, &lstr);
    sign_license(&lraw, argv[1]);
    write_license("license.dat", &lraw);

    fprintf(stderr, "License file generated.\n");

    read_license("license.dat", &lraw);
    check_license_signature(&lraw);
    decode_license(&lstr, &lraw);
    print_license(&lstr);
    verify_license(&lstr, 1, 1, 8);
    free_license(&lstr);

    free_openssl();
    
    return 0;
}
