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
 *
 ******************************************************************************/
#include "JsonAST.hh"
#include "../../common/memory.h"
#include <cstddef>
#include <cstdio>

void JsonSchemaExtension::init(char* p_key, char* p_value)
{
  key = p_key;
  value = p_value;
}

JsonSchemaExtension::~JsonSchemaExtension()
{
  Free(key);
  Free(value);
}

void JsonAST::init_JsonAST()
{
  omit_as_null = false;
  alias = NULL;
  as_value = false;
  default_value = NULL;
  metainfo_unbound = false;
}

JsonAST::JsonAST(const JsonAST *other_val)
{
  init_JsonAST();
  if (NULL != other_val) {
    omit_as_null = other_val->omit_as_null;
    alias = (NULL != other_val->alias) ? mcopystr(other_val->alias) : NULL;
    as_value = other_val->as_value;
    default_value = (NULL != other_val->default_value) ? mcopystr(other_val->default_value) : NULL;
    for (size_t i = 0; i < other_val->schema_extensions.size(); ++i) {
      schema_extensions.add(new JsonSchemaExtension(*other_val->schema_extensions[i]));
    }
    metainfo_unbound = other_val->metainfo_unbound;
  }
}

JsonAST::~JsonAST()
{
  Free(alias);
  Free(default_value);
  for (size_t i = 0; i < schema_extensions.size(); ++i) {
    delete schema_extensions[i];
  }
  schema_extensions.clear();
}

void JsonAST::print_JsonAST() const
{
  printf("\n\rOmit encoding: ");
  if (omit_as_null) {
    printf("as null value\n\r");
  } else {
    printf("skip field\n\r");
  }
  if (alias) {
    printf("Name as %s\n\r", alias);
  }
  if (as_value) {
    printf("Encoding unions as JSON value\n\r");
  }
  if (default_value) {
    printf("Default value: %s\n\r", default_value);
  }
  if (0 != schema_extensions.size()) {
    printf("Extensions:");
    for (size_t i = 0; i < schema_extensions.size(); ++i) {
      printf(" \"%s\" : \"%s\"", schema_extensions[i]->key, schema_extensions[i]->value);
    }
  }
  if (metainfo_unbound) {
    printf("Metainfo for unbound field(s)\n\r");
  }
}
