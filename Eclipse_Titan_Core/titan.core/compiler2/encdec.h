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
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *
 ******************************************************************************/
#ifndef ENCDEC_H
#define ENCDEC_H

#include "datatypes.h"

#ifdef __cplusplus
extern "C" {
#endif

  /** Generate code for enc-dec.
   *
   * @param[in]  p_classname
   * @param[out] p_classdef
   * @param[out] p_classsrc
   * @param[in]  ber  BER  codec needed
   * @param[in]  raw  RAW  codec needed
   * @param[in]  text TEXT codec needed
   * @param[in]  xer  XER  codec needed
   * @param[in]  is_leaf
   *  */
  void def_encdec(const char *p_classname,
                  char **p_classdef, char **p_classsrc,
                  boolean ber, boolean raw, boolean text, boolean xer,
                  boolean json, boolean is_leaf);
  char *genRawFieldChecker(char *src,
                  const rawAST_coding_taglist *taglist, boolean is_equal);
  char *genRawTagChecker(char *src, const rawAST_coding_taglist *taglist);

#ifdef __cplusplus
}; /* extern "C" */
#endif

#endif /* ENCDEC_H */
