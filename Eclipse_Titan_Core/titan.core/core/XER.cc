/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Baranyi, Botond
 *   Ormandi, Matyas
 *   Raduly, Csaba
 *   Szabo, Bence Janos
 *
 ******************************************************************************/
#define DEFINE_XER_STRUCT
#include "XER.hh"

//#include <libxml/xmlreader.h>
#include "XmlReader.hh"
#include "Module_list.hh"

// FIXME: there ought to be a better way than this
static const char indent_buffer[] =
  // --------------------------- 32 ----------------------------- ||
  "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t" //1
  "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t" //2
  "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t" //3
  "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t" //4
  "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t" //5
  "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t" //6
  "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t" //7
  "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t" //8
  "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t" //9
  "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t" //10
  "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t" //11
  "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t" //12
  "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t" //13
  "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t" //14
  "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t" //15
  "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";//16

// FIXME do_indent's level should be size_t, but there were plans of passing negative values
int do_indent(TTCN_Buffer& buf, int level)
{
  if (level > 0 && level < (int)sizeof(indent_buffer)) {
    buf.put_s((size_t)level, (const unsigned char*)indent_buffer);
  }
  return level;
}

const char* verify_name(XmlReaderWrap& reader, const XERdescriptor_t& p_td, boolean exer)
{
  const char *name = (const char*)reader.LocalName();
  const char *nsuri= (const char*)reader.NamespaceUri(); // NULL if no ns

  const namespace_t *expected_ns = 0;
  if (p_td.my_module != 0 && p_td.ns_index != -1) {
    expected_ns = p_td.my_module->get_ns((size_t)p_td.ns_index);
  }

  if (0 == name) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TAG,
      "NULL XML name instead of `%.*s'",
      p_td.namelens[exer]-2, p_td.names[exer]);
  }
  if (!check_name(name, p_td, exer)) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TAG,
      "Bad XML tag `%s' instead of `%.*s'",
      name, p_td.namelens[exer]-2, p_td.names[exer]);
  }

  if (exer) { // XML namespaces only apply to EXER
    const char *prefix = (const char*)reader.Prefix(); // may be NULL
    if (expected_ns == 0) { // get_ns was not called
      if (nsuri != 0) {
        TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TAG,
          "Unexpected namespace '%s' (%s)", nsuri, prefix ? prefix : "");
      }
    }
    else { // a namespace was expected
      if (p_td.xer_bits & FORM_UNQUALIFIED) {
        if (prefix && *prefix) TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TAG,
          "Unexpected prefix '%s'", prefix);
      }
      else {
        if (nsuri == 0) { // XML node has no namespace
          if (p_td.my_module->get_ns((size_t)p_td.ns_index)->px[0] != 0) { // and module has namespace prefix
            TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TAG,
              "Missing namespace '%s'", expected_ns->ns);
          }
        }
        else { // and there is one, but is it the right one ?
          if (strcmp(nsuri, expected_ns->ns)) {
            TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TAG,
              "Bad XML namespace `%s' instead of `%s'", nsuri, expected_ns->ns);
          }
        }
      }
    }
  } // if exer

  return name;
}

void verify_end(XmlReaderWrap& reader, const XERdescriptor_t& p_td, const int depth, boolean exer)
{
  TTCN_EncDec_ErrorContext endcontext("While checking end tag: ");
  verify_name(reader, p_td, exer);
  const int currdepth = reader.Depth();
  if (currdepth!=depth) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TAG,
      "Bad depth in XML, %d instead of %d", currdepth, depth);
  }
}

// This should be called for EXER only
boolean check_namespace(const char *ns_uri, const XERdescriptor_t& p_td)
{
  if (p_td.my_module==0 || p_td.ns_index==-1) {// no namespace in XER descriptor
    return ns_uri==0 || *ns_uri=='\0'; // there should be no ns
  }
  else {
    const namespace_t *expected_ns = p_td.my_module->get_ns((size_t)p_td.ns_index);
    if (ns_uri!=0) return strcmp(ns_uri, expected_ns->ns)==0;
    else return TRUE; // if no namespace we presume it's the expected one
  }
}

void write_ns_prefix(const XERdescriptor_t& p_td, TTCN_Buffer& p_buf)
{
  if (p_td.my_module != 0 && p_td.ns_index != -1
    && !(p_td.xer_bits & FORM_UNQUALIFIED)) {
    const namespace_t *my_ns = p_td.my_module->get_ns((size_t)p_td.ns_index);
    if (my_ns->px[0] != 0) { // not an empty prefix
      p_buf.put_s(strlen(my_ns->px), (cbyte*)my_ns->px);
      p_buf.put_c(':');
    }
  }
}

const char* get_ns_uri_from_prefix(const char *prefix, const XERdescriptor_t& p_td)
{
  if (p_td.my_module != 0 && prefix != NULL && prefix[0] != 0) {
    for (size_t i = 0; i < p_td.my_module->get_num_ns(); ++i) {
      const namespace_t *ns = p_td.my_module->get_ns(i);
      if (ns->px != NULL && strcmp(ns->px, prefix) == 0) {
        return ns->ns;
      }
    }
  }
  return NULL;
}

void check_namespace_restrictions(const XERdescriptor_t& p_td, const char* p_xmlns)
{
  // In case of "anyElement from ..." matching namespaces is good
  // in case of "anyElement except ..." matching namespaces is bad
  boolean ns_match_allowed = (p_td.xer_bits & ANY_FROM) ? TRUE : FALSE;

  boolean ns_error = ns_match_allowed;
  for (unsigned short idx = 0; idx < p_td.nof_ns_uris; ++idx) {
    if ((p_xmlns == 0 && strlen(p_td.ns_uris[idx]) == 0) ||
        (p_xmlns != 0 && strcmp(p_td.ns_uris[idx], p_xmlns) == 0))
    {
      ns_error = !ns_match_allowed;
      break;
    }
  }

  if (ns_error) {
    if (p_xmlns) {
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG, 
        "XML namespace \"%s\" is %s namespace list.", p_xmlns, ns_match_allowed ? "not in the allowed" : "in the excluded");
    }
    else {
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG, 
        "The unqualified XML namespace is %s namespace list.", ns_match_allowed ? "not in the allowed" : "in the excluded");
    }
  }
}
