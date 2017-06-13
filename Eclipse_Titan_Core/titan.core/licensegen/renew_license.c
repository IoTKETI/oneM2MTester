/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Gecse, Roland
 *
 ******************************************************************************/
#include <stdio.h>
#include "cgic.h"
#include <string.h>
#include <stdlib.h>

#include "mysql.h"

#define CGI_INPUT_UNIQUE_ID "unique_id"
#define CGI_INPUT_DEFAULT_UNIQUE_ID 0
#define CGI_INPUT_LICENSEE_EMAIL "licensee_email"
#define CGI_INPUT_LICENSEE_EMAIL_MAX_LENGTH 40+1

#define MYSQL_HOST "mwlx122"
#define MYSQL_DB_NAME "ttcn3"
#define MYSQL_USERID "ttcn3"
#define MYSQL_PASSWD "ttcn3"

#define COMMAND_LINE_BUFFER_LENGTH 80

/*
  The content of CGI_INPUT_UNIQUE_ID should be checked before submitting 
  the content to this program.  Missing CGI_INPUT_UNIQUE_ID defaults to 0.

  Do not forget to set mySQL database and user parameters below!

  Requires cgic.h and libcgic.a for compiling (http://boutell.com/cgic/).

  Requires mysql.h and libmysql.a for compiling.
*/

int cgiMain() {
  /* Send the content type, letting the browser know this is HTML */
  cgiHeaderContentType("text/html");
  /* Top of the page */
  fprintf(cgiOut, "<HTML><HEAD>\n");
  fprintf(cgiOut, "<TITLE>renew_license</TITLE></HEAD>\n");
  fprintf(cgiOut, "<BODY><H1>renew_license</H1>\n");

  int unique_id;
  if(cgiFormInteger(CGI_INPUT_UNIQUE_ID, &unique_id, 
    CGI_INPUT_DEFAULT_UNIQUE_ID) != cgiFormSuccess) {
    fprintf(cgiOut, 
      "<P>ERROR: Missing or invalid input field <code>%s</code>.", 
      CGI_INPUT_UNIQUE_ID);
    goto the_end;  
  };

  char licensee_email[CGI_INPUT_LICENSEE_EMAIL_MAX_LENGTH];
  if(cgiFormStringNoNewlines(CGI_INPUT_LICENSEE_EMAIL, licensee_email,
    CGI_INPUT_LICENSEE_EMAIL_MAX_LENGTH) 
    != cgiFormSuccess) {
    fprintf(cgiOut, 
      "<P>ERROR: Missing or invalid input field <code>%s</code>.", 
      CGI_INPUT_LICENSEE_EMAIL);
    goto the_end;  
  };

  /* Connect to mySQL server */
  MYSQL *mysql = mysql_init(NULL);
  if(!mysql) {
    fprintf(cgiOut, 
      "<P>ERROR: Could not initialize mySQL handle."); 
    goto the_end;
  }
  if(!(mysql_real_connect(mysql, MYSQL_HOST, MYSQL_USERID, MYSQL_PASSWD,
    MYSQL_DB_NAME, 0, NULL, CLIENT_ODBC))) {
    fprintf(cgiOut, 
      "<P>ERROR: Could not connect to database."); 
    goto the_end;
  }
  /* Execute query */
  char query[1024];  
  snprintf(query, 1024, 
    "SELECT * FROM licenses WHERE unique_id=%d AND "
    "licensee_email='%s' AND "
    "valid_until < DATE_ADD(CURDATE(), INTERVAL 1 MONTH);\0", 
    unique_id, licensee_email); 
  if(mysql_query(mysql, query)) {
    fprintf(cgiOut, 
      "<P>ERROR: Problem executing query."); 
    goto before_the_end;
  };
  MYSQL_RES *result;  
  result = mysql_store_result(mysql);
  if(result) {
    /* mysql_num_rows() returns my_ulonglong==unsigned long */
    my_ulonglong num_records = mysql_num_rows(result);
    if(num_records == 0) {
      fprintf(cgiOut, 
      "<P>ERROR: Did not find license with "
      "number <code>%d</code>, " 
      "e-mail <code>%s</code>, expiring within 1 month. "
      "Re-check data validity and try again.", 
      unique_id, 
      licensee_email); 
      goto before_the_end;
    } 
    MYSQL_ROW row = mysql_fetch_row(result);
    fprintf(cgiOut, "Debug info (remove it from final build):<br>");
    fprintf(cgiOut, "<p>unique_id: <code>%s</code>", row[0]);
    fprintf(cgiOut, "<p>licensee_email: <code>%s</code>", row[2]);
    fprintf(cgiOut, "<p>valid_until: <code>%s</code>", row[7]);
    fprintf(cgiOut, "<p>notes: <code>%s</code>", 
      row[33] ? row[33] : "NULL");
    /* Update mySQL database with new expiry date and add note */
    snprintf(query, 1024, 
      "UPDATE licenses SET "
      "valid_until=DATE_ADD(NOW(), INTERVAL 1 YEAR), "
      "notes=CONCAT(COALESCE(notes, ''), '\nProlonged on ', CURDATE(), '.') "
      "WHERE unique_id=%d;", unique_id);
    fprintf(cgiOut, 
      "<P>Should execute the followin query now "
      "(uncomment next line in source to do so): <BR>"
      "<code>%s</code>\n", query);
    /* if(mysql_query(mysql, query)) {
      fprintf(cgiOut, "<P>ERROR: Problem executing query."); 
      goto before_the_end;
    } */
    /* Generate and email license */
    char command_line[COMMAND_LINE_BUFFER_LENGTH];
    snprintf(command_line, COMMAND_LINE_BUFFER_LENGTH, 
      "license_gen -m %d", unique_id);
    if(!(system(command_line))) {
      fprintf(cgiOut, 
      "<P>Your new license key (%s) was generated and "
      "e-mailed to your filed e-mail address (%s).",
      row[0], 
      row[2]);
    } else {
      fprintf(cgiOut, "<P>ERROR: Could not launch subshell." );
    }
    mysql_free_result(result);
  } else {
    fprintf(cgiOut, 
      "<P>ERROR: Problem fetching results from database."); 
    goto before_the_end;
  }

  /* Terminate mysql session */
before_the_end:
  mysql_close(mysql);

  /* Finish up the page */
the_end:
  fprintf(cgiOut, "</BODY></HTML>\n");
  return 0;
}



