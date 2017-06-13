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
 *   Lovassy, Arpad
 *   Raduly, Csaba
 *
 ******************************************************************************/
#include "tcov2lcov.hh"

#include <cstdlib>
#include <iostream>
#include <fstream>

#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <libxml/xmlschemastypes.h>
#include <libxml/xmlreader.h>

#include "../common/version_internal.h"
#include "../common/license.h"

// The new (T)itan(C)overage(D)ata is born!
const std::string tcd_ext(".tcd");
static size_t tcd_ext_len = tcd_ext.size();

#ifdef WIN32
#define P_SEP "\\"
#else
#define P_SEP "/"
#endif

void TcovData::inc_function(const std::string& function, int n)
{
  if (m_functions.count(function) == 0) m_functions[function] = 0;
  m_functions[function] += n;
}

void TcovData::inc_line(int line, int n)
{
  if (m_lines.count(line) == 0) m_lines[line] = 0;
  m_lines[line] += n;
}

Tcov2Lcov::Tcov2Lcov(const char *code_base, const char *input_dir, const char *output_file, const char *xsd_file)
  : m_code_base(code_base), m_input_dir(input_dir), m_output_file(output_file), m_xsd_file(xsd_file)
{
  m_ver_major = m_ver_minor = -1;
}

Tcov2Lcov::~Tcov2Lcov()
{
  std::map<std::string, TcovData *>::iterator i = m_data.begin();
  for (; i != m_data.end(); ++i) {
    delete (*i).second;
  }
  m_data.clear();
}

int Tcov2Lcov::collect_dir(std::string dir)
{
  DIR *d = NULL;
  if ((d = opendir(dir.c_str())) == NULL) {
    // Fatal for top-level directory.
    std::cerr << "Error while opening directory `" << dir << "'" << std::endl;
    return -1;
  }
  struct dirent *de = NULL;
  while ((de = readdir(d)) != NULL) {
    std::string file_name(de->d_name);
    std::string full_file_name = dir + P_SEP + file_name;
    // No `d_type' field on Solaris...
    struct stat st;
    int stat_ret_val = stat(de->d_name, &st);
    if (stat_ret_val != 0)
      continue;
    if (S_ISDIR(st.st_mode)) {
      if (file_name == ".." || file_name == ".")
        continue;
      collect_dir(full_file_name);
    } else {
      size_t file_name_len = file_name.size();
      if (file_name_len > tcd_ext_len
          && file_name.substr(file_name_len - tcd_ext_len) == tcd_ext) {
        m_files.push_back(full_file_name);
      }
    }
  }
  closedir(d);
  return 0;
}

int Tcov2Lcov::collect()
{
  if (collect_dir(m_input_dir) < 0)
    return -1;
  return static_cast<int>(m_files.size());
}

void Tcov2Lcov::d_print_files() const
{
  std::cout << "Collected XML/TCD files:" << std::endl;
  for (size_t i = 0; i < m_files.size(); ++i)
    std::cout << "#" << i << ": " << m_files[i] << std::endl;
}

int Tcov2Lcov::validate() const
{
  xmlInitParser();
  xmlLineNumbersDefault(1);

  xmlSchemaParserCtxtPtr ctxt = xmlSchemaNewParserCtxt(m_xsd_file.c_str());
  xmlSchemaSetParserErrors(ctxt, (xmlSchemaValidityErrorFunc)fprintf,
    (xmlSchemaValidityWarningFunc)fprintf, stderr);
  // valgrind reports `Conditional jump or move depends on uninitialised
  // value(s)' even for xmllint when using xmlSchemaParse().
  xmlSchemaPtr schema = xmlSchemaParse(ctxt);
  xmlSchemaFreeParserCtxt(ctxt);
  if (schema == NULL) {
    xmlSchemaCleanupTypes();
    xmlCleanupParser();
    xmlMemoryDump();
    return -1;
  }
  int ret_val = 0;
  for (size_t i = 0; i < m_files.size(); ++i) {
    const char *xml_file_name = m_files[i].c_str();
    xmlDocPtr doc = xmlReadFile(xml_file_name, NULL, 0);
    if (doc == NULL) {
      std::cerr << "Could not parse `" << xml_file_name << "'" << std::endl;
      ret_val = -1;
      continue;
    } else {
      xmlSchemaValidCtxtPtr xml_ctxt = xmlSchemaNewValidCtxt(schema);
      xmlSchemaSetValidErrors(xml_ctxt, (xmlSchemaValidityErrorFunc)fprintf,
        (xmlSchemaValidityWarningFunc)fprintf, stderr);
      int valid = xmlSchemaValidateDoc(xml_ctxt, doc);
      if (valid != 0) {
        std::cerr << "`" << xml_file_name << "' fails to validate" << std::endl;
        ret_val = -1;
      }
      xmlSchemaFreeValidCtxt(xml_ctxt);
      xmlFreeDoc(doc);
    }
  }
  xmlSchemaFree(schema);
  xmlSchemaCleanupTypes();
  xmlCleanupParser();
  xmlMemoryDump();
  return ret_val;
}

const char *get_next_attr(const xmlTextReaderPtr& reader)
{
  xmlTextReaderMoveToNextAttribute(reader);
  return (const char *)xmlTextReaderConstValue(reader);
}

int Tcov2Lcov::merge()
{
  for (size_t i = 0; i < m_files.size(); ++i) {
    std::string& file_name = m_files[i];
    xmlInitParser();  // Avoid weird memory leaks...
    xmlTextReaderPtr reader = xmlReaderForFile(file_name.c_str(), NULL, 0);
    if (reader == NULL) {
      std::cerr << "Unable to open `" << file_name << "'" << std::endl;
      return -1;
    }
    std::string current_file_name;
    while (xmlTextReaderRead(reader)) {
      switch (xmlTextReaderNodeType(reader)) {
      case XML_READER_TYPE_ELEMENT: {
        std::string elem((const char *)xmlTextReaderConstName(reader));
        if (elem == "version") {
          int major = atoi(get_next_attr(reader));
          int minor = atoi(get_next_attr(reader));
          if (m_ver_major == -1) m_ver_major = major;
          if (m_ver_minor == -1) m_ver_minor = minor;
          if (m_ver_major != major || m_ver_minor != minor) {
            std::cerr << "Version mismatch in `" << file_name << "' ("
                      << major << "." << minor << " vs. " << m_ver_major
                      << "." << m_ver_minor << ")" << std::endl;
            xmlFreeTextReader(reader);
            xmlCleanupParser();
            xmlMemoryDump();
            return -1;
          }
        } else if (elem == "file") {
          current_file_name = get_next_attr(reader);
          if (m_data.count(current_file_name) == 0)
            m_data[current_file_name] = new TcovData();
        } else if (elem == "function") {
          std::string function_name(get_next_attr(reader));
          int function_count = atoi(get_next_attr(reader));
          m_data[current_file_name]->inc_function(function_name, function_count);
        } else if (elem == "line") {
          int line_no = atoi(get_next_attr(reader));
          int line_count = atoi(get_next_attr(reader));
          m_data[current_file_name]->inc_line(line_no, line_count);
        }
        xmlTextReaderMoveToElement(reader);
        continue;
      }
      default:
        // Ignore.
        break;
      }
    }
    xmlFreeTextReader(reader);
    xmlCleanupParser();
    xmlMemoryDump();
  }
  return 0;
}

int Tcov2Lcov::generate()
{
  struct stat st;
  if (!(stat(m_code_base.c_str(), &st) == 0 && S_ISDIR(st.st_mode))) {
	std::cerr << "Invalid code base `" << m_code_base << "'" << std::endl;
	return -1;
  }
  std::ofstream output(m_output_file.c_str());
  if (!output.is_open()) {
	std::cerr << "Output file `" << m_output_file << "' cannot be opened" << std::endl;
	return -1;
  }
  output << "TN:" << std::endl;
  std::map<std::string, TcovData *>::const_iterator i = m_data.begin();
  for (; i != m_data.end(); ++i) {
    output << "SF:" << m_code_base << P_SEP << (*i).first << std::endl;
    const TcovData *data = (*i).second;
    const std::map<std::string, int>& functions = data->get_functions();
    const std::map<int, int>& lines = data->get_lines();
    std::map<std::string, int>::const_iterator j = functions.begin();
    std::map<int, int>::const_iterator k = lines.begin();
    for (; j != functions.end(); ++j)  // All functions.
      output << "FN:0," << (*j).first << std::endl;
    for (j = functions.begin(); j != functions.end(); ++j)  // All functions counted.
      output << "FNDA:" << (*j).second << "," << (*j).first << std::endl;
    for (; k != lines.end(); ++k)  // All lines counted.
      output << "DA:" << (*k).first << "," << (*k).second << std::endl;
    output << "end_of_record" << std::endl;
  }
  output.close();
  return 0;
}

	static void print_product_info()
{
  std::cerr << "TCD to LCOV Converter for the TTCN-3 Test Executor, version "
            << PRODUCT_NUMBER << std::endl;
}

static void print_version()
{
  std::cerr << "Product number: " << PRODUCT_NUMBER << std::endl
            << "Build date: " << __DATE__ << " " << __TIME__ << std::endl
            << "Compiled with: " << C_COMPILER_VERSION << std::endl << std::endl
            << COPYRIGHT_STRING << std::endl << std::endl;
}

static void print_help(const char *name)
{
  std::cerr << "Usage: " << name << " [-hv] [-b dir] [-d dir] [-o file] [-x xsd] " << std::endl
		    << "  -b dir\tSet code base for source files" << std::endl
            << "  -d dir\tCollect XML/TCD files from `dir'" << std::endl
            << "  -h\t\tPrint help" << std::endl
            << "  -o file\tPlace of the output file" << std::endl
            << "  -v\t\tShow version" << std::endl
            << "  -x xsd\tValidate all XMLs/TCDs against this `xsd'" << std::endl;
}

int main(int argc, char *argv[])
{
  // Unfortunately, getopt_long() and :: for optional parameters are GNU extensions and not present on Solaris...
  // -b --dir: Set code base for source files (_optional_)
  // -d --dir: Collect XML/TCD files from, default is `.' the current directory (_optional_)
  // -o --output: Place of the output, default is `./output.info' (_optional_)
  // -x --xsd: Validate all XMLs/TCDs against this XSD (_optional_)
  // -h --help: Display help (_optional_)
  const char *code_base = NULL, *input_dir = NULL, *output_file = NULL, *xsd_file = NULL;
  int c;
  while ((c = getopt(argc, argv, "b:d:ho:vx:")) != -1) {
    switch (c) {
    case 'b':
      code_base = optarg;
      break;
    case 'd':
      input_dir = optarg;
      break;
    case 'h':
      print_product_info();
      print_help(argv[0]);
      return EXIT_SUCCESS;
    case 'o':
      output_file = optarg;
      break;
    case 'v':
      print_product_info();
      print_version();
#ifdef LICENSE
      print_license_info();
#endif
      return EXIT_SUCCESS;
    case 'x':
      xsd_file = optarg;
      break;
    default:
      print_help(argv[0]);
      return EXIT_FAILURE;
    }
  }
  bool param_error = false;
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif
  char cwd[PATH_MAX];
  memset(cwd, 0, PATH_MAX);
  if (!code_base) {
	if (!getcwd(cwd, PATH_MAX)) {
	  std::cerr << "Unable to get current working directory for `-b'" << std::endl;
	  param_error = true;
	} else code_base = cwd;
  }
  // Leave it relative.
  if (!input_dir) input_dir = ".";
  if (!output_file)	output_file = "output.info";
  if (!xsd_file) {
    const char *ttcn3_dir = getenv("TTCN3_DIR");
    if (!ttcn3_dir) {
      std::cerr << "$TTCN3_DIR environment variable is not set" << std::endl;
      param_error = true;
    } else {
      std::string xsd_file_str = std::string(ttcn3_dir) + P_SEP + std::string("include") + P_SEP + std::string("tcov.xsd");
      xsd_file = xsd_file_str.c_str();
    }
  }
  if (param_error) {
	print_help(argv[0]);
    return EXIT_FAILURE;
  }

#ifdef LICENSE
  {
    init_openssl();
    license_struct lstr;
    load_license  (&lstr);
    int license_valid = verify_license(&lstr);
    free_license  (&lstr);
    free_openssl();
    if (!license_valid) {
      exit(EXIT_FAILURE);
    }
  }
#endif

  Tcov2Lcov t2l(code_base, input_dir, output_file, xsd_file);
  int num_files = t2l.collect();
  if (num_files < 0) {
    std::cerr << "Directory `" << input_dir << "' doesn't exist" << std::endl;
    return EXIT_FAILURE;
  } else if (num_files == 0) {
    std::cerr << "No data files were found in the given directory `"
              << input_dir << "'" << std::endl;
    return EXIT_FAILURE;
  }
  // Initialize the library and check potential ABI mismatches between
  // the between the version it was compiled for and the actual shared
  // library used.
  LIBXML_TEST_VERSION
  if (t2l.validate() < 0) {
    std::cerr << "Error while validating XML/TCD files" << std::endl;
    return EXIT_FAILURE;
  }
  if (t2l.merge() < 0) {
    std::cerr << "Error while merging XML/TCD files" << std::endl;
    return EXIT_FAILURE;
  }
  if (t2l.generate() < 0) {
    std::cerr << "Error while generating output file `" << output_file << "'"
              << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

reffer::reffer(const char*) {}
