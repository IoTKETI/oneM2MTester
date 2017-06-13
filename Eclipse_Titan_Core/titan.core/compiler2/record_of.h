/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Delic, Adam
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef RECORD_OF_H
#define RECORD_OF_H

#include "datatypes.h"
#include "ttcn3/compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

void defRecordOfClass(const struct_of_def *sdef, output_struct *output);
/* generates value class which uses less memory allocations by not allocating memory
 * for each individual element, but rather allocating one contiguous chunk */
void defRecordOfClassMemAllocOptimized(const struct_of_def *sdef, output_struct *output);
void defRecordOfTemplate(const struct_of_def *sdef, output_struct *output);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RECORD_OF_H */
