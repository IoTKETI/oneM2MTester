/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Bartha, Norbert
 *   Kovacs, Ferenc
 *
 ******************************************************************************/
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "Temp.hh"

namespace Temp {

BOOLEAN checkFile(const CHARSTRING& filename, const INTEGER& fileSizeInByte)
{
  FILE *fp;
  //checking if the file is exist
  fp = fopen(filename,"r");
  if (!fp) return false;

  //checking the file size if grater than expected
  //if the filesize is set to 0 means filesize is not matter
  //no check for that
  struct stat buf;
  fstat(fileno(fp), &buf);

  if(fileSizeInByte != 0)
  {
  	if(buf.st_size > (int)fileSizeInByte) return false;
  }


  fclose(fp);
  return true;

}

BOOLEAN check__script__out(const INTEGER& phase, const CHARSTRING& to_match)
{
  FILE *fp = fopen(phase > 0 ? "end_script.out" /* EndTestCase */ : "begin_script.out" /* BeginTestCase */, "rt");
  if (!fp) return false;
  char line[512];
  memset(line, 0, sizeof line);
  if (!fgets(line, sizeof line, fp)) return false;
  if (strcmp(line, (const char *)to_match) != 0) return false;
  fclose(fp);
  return true;
}

}
