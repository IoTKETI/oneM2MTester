/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *
 ******************************************************************************/
#include "TCov.hh"

#include "Component.hh"
#include "Runtime.hh"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

expstring_t TCov::comp(boolean withname)
{
  enum { SINGLE, HC, MTC, PTC } whoami;
  if (TTCN_Runtime::is_single()) whoami = SINGLE;
  else if (TTCN_Runtime::is_hc()) whoami = HC;
  else if (TTCN_Runtime::is_mtc()) whoami = MTC;
  else whoami = PTC;
  expstring_t ret_val = NULL;
  switch (whoami) {
    case SINGLE: ret_val = mcopystr("single"); break;
    case HC: ret_val = mcopystr("hc"); break;
    case MTC: ret_val = mcopystr("mtc"); break;
    case PTC:
    default: {
	  const char *compname = TTCN_Runtime::get_component_name();
	  if (withname && compname != NULL) ret_val = mcopystr(compname);
	  else ret_val = mprintf("%d", self.is_bound() ? (component)self : (component)0);
      break;
    }
  }
  return ret_val;
}

int TCov::ver_major = 1;
int TCov::ver_minor = 0;
Vector<FileData *> TCov::m_file_data;
pid_t TCov::mypid = getpid();
expstring_t TCov::mycomp = TCov::comp();
expstring_t TCov::mycomp_name = TCov::comp(TRUE);

size_t TCov::has_file_name(const char *file_name)
{
  size_t i = 0;
  for (; i < m_file_data.size(); ++i) {
    if (!strcmp(file_name, m_file_data[i]->get_file_name())) {
      return i;
    }
  }
  return i;
}

void TCov::close_file()
{
  if (m_file_data.empty()) {
	Free(mycomp);
	Free(mycomp_name);
	mycomp = mycomp_name = NULL;
	return;
  }
  expstring_t file_name = mputprintf(NULL, "tcov-%s.tcd", mycomp);
  FILE *fp = fopen((const char *)file_name, "w");
  expstring_t output = mputprintf(NULL,
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" \
    "<?xml-stylesheet type=\"text/xsl\" href=\"tcov.xsl\"?>\n" \
    "<titan_coverage xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " \
    "xsi:schemaLocation=\"tcov.xsd\">\n" \
    "\t<version major=\"%d\" minor=\"%d\" />\n" \
    "\t<component id=\"%s\" name=\"%s\" />\n" \
    "\t<files>\n",
    ver_major, ver_minor, mycomp, mycomp_name);
  for (size_t i = 0; i < m_file_data.size(); ++i) {
    const FileData *file_data = m_file_data[i];
    output = mputprintf(output, "\t\t<file path=\"%s\">\n", file_data->get_file_name());
    const Vector<FunctionData *>& function_data = file_data->get_function_data();
    const Vector<LineData *>& line_data = file_data->get_line_data();
    output = mputstr(output, "\t\t\t<functions>\n");
    for (size_t j = 0; j < function_data.size(); ++j) {
      output = mputprintf(output, "\t\t\t\t<function name=\"%s\" count=\"%d\" />\n",
        function_data[j]->get_name(), function_data[j]->get_count());
    }
    output = mputstr(output, "\t\t\t</functions>\n");
    output = mputstr(output, "\t\t\t<lines>\n");
    for (size_t j = 0; j < line_data.size(); ++j) {
      output = mputprintf(output, "\t\t\t\t<line no=\"%d\" count=\"%d\" />\n",
        line_data[j]->get_no(), line_data[j]->get_count());
    }
    output = mputstr(output,
      "\t\t\t</lines>\n" \
      "\t\t</file>\n");
  }
  output = mputstr(output,
    "\t</files>\n" \
    "</titan_coverage>\n");
  fputs(output, fp);
  fclose(fp);
  Free(output);
  Free(file_name);
  for (size_t i = 0; i < m_file_data.size(); ++i)
	delete m_file_data[i];
  m_file_data.clear();
  Free(mycomp);
  Free(mycomp_name);
  mycomp = mycomp_name = NULL;
}

FileData::FileData(const char *file_name)
{
  m_file_name = mcopystr(file_name);
}

FileData::~FileData()
{
  Free(m_file_name);
  m_file_name = NULL;
  for (size_t i = 0; i < m_function_data.size(); ++i)
	delete m_function_data[i];
  for (size_t i = 0; i < m_line_data.size(); ++i)
  	delete m_line_data[i];
  m_function_data.clear();
  m_line_data.clear();
}

void FileData::reset()
{
  for (size_t i = 0; i < m_function_data.size(); ++i)
    m_function_data[i]->reset();
  for (size_t i = 0; i < m_line_data.size(); ++i)
  	m_line_data[i]->reset();
}

size_t FileData::has_function_name(const char *function_name)
{
  size_t i = 0;
  for (; i < m_function_data.size(); ++i) {
    if (!strcmp(function_name, m_function_data[i]->get_name())) {
      return i;
    }
  }

  return i;
}

size_t FileData::has_line_no(int line_no)
{
  size_t i = 0;
  for (; i < m_line_data.size(); ++i) {
    if (line_no == m_line_data[i]->get_no()) {
      return i;
    }
  }

  return i;
}

void FileData::inc_function(const char *function_name, int /*line_no*/)
{
  size_t i = has_function_name(function_name);
  if (i == m_function_data.size())
    m_function_data.push_back(new FunctionData(function_name));
  ++(*m_function_data[i]);
}

void FileData::inc_line(int line_no)
{
  size_t i = has_line_no(line_no);
  if (i == m_line_data.size())
    m_line_data.push_back(new LineData(line_no));
  ++(*m_line_data[i]);
}

void FileData::init_function(const char *function_name)
{
  size_t i = has_function_name(function_name);
  if (i == m_function_data.size()) m_function_data.push_back(new FunctionData(function_name));
}

void FileData::init_line(int line_no)
{
  size_t i = has_line_no(line_no);
  if (i == m_line_data.size()) m_line_data.push_back(new LineData(line_no));
}

void TCov::pid_check()
{
  pid_t p = getpid();
  if (mypid != p) {
	mypid = p;
	Free(mycomp);
	Free(mycomp_name);
	mycomp = NULL;
	mycomp_name = NULL;
	mycomp = comp();
	mycomp_name = comp(TRUE);
    for (size_t i = 0; i < m_file_data.size(); ++i)
      m_file_data[i]->reset();
  }
}

void TCov::hit(const char *file_name, int line_no, const char *function_name)
{
  pid_check();
  size_t i = has_file_name(file_name);
  if (i == m_file_data.size())
	m_file_data.push_back(new FileData(file_name));
  if (function_name)
	m_file_data[i]->inc_function(function_name, line_no);
  m_file_data[i]->inc_line(line_no);
}

void TCov::init_file_lines(const char *file_name, const int line_nos[], size_t line_nos_len)
{
  pid_check();
  size_t i = has_file_name(file_name);
  if (i == m_file_data.size())
	m_file_data.push_back(new FileData(file_name));
  for (size_t j = 0; j < line_nos_len; ++j)
    m_file_data[i]->init_line(line_nos[j]);
}

void TCov::init_file_functions(const char *file_name, const char *function_names[], size_t function_names_len)
{
  pid_check();
  size_t i = has_file_name(file_name);
  if (i == m_file_data.size())
	m_file_data.push_back(new FileData(file_name));
  for (size_t j = 0; j < function_names_len; ++j)
    m_file_data[i]->init_function(function_names[j]);
}
