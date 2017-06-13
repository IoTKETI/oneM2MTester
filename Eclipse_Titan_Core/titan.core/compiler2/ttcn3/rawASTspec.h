/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *
 ******************************************************************************/
#ifndef RAWASTSPEC_H
#define RAWASTSPEC_H

#define XDEFUNSIGNED 1
#define XDEFCOMPL 2
#define XDEFSIGNBIT 3
#define XDEFYES 2
#define XDEFNO 1
#define XDEFREVERSE 3
#define XDEFMSB 1
#define XDEFLSB 2
#define XDEFBITS 1
#define XDEFOCTETS 2
#define XDEFLEFT 1
#define XDEFRIGHT 2
#define XDEFFIRST 1
#define XDEFLAST 2
#define XDEFLOW 1
#define XDEFHIGH 2
#define XDEFDEFAULT -1

typedef struct {
    int bitorder;               /* Invert bitorder of the encoded data */
}rawAST_toplevel;

typedef enum { MANDATORY_FIELD, OPTIONAL_FIELD, UNION_FIELD
} rawAST_coding_field_type;

typedef struct{
    int nthfield;
    const char* nthfieldname;
    rawAST_coding_field_type fieldtype;
    const char* type;
    const char* typedescr;
}rawAST_coding_fields;

typedef struct{
   int nElements;
   rawAST_coding_fields* fields;
   const char *value;
   int start_pos;
   size_t temporal_variable_index;
}rawAST_coding_field_list;

typedef struct{
    const char* fieldName;
    int fieldnum;
    int nElements;
    rawAST_coding_field_list* fields;
}rawAST_coding_taglist;

typedef struct{
    int nElements;
    rawAST_coding_taglist* list;
}rawAST_coding_taglist_list;

typedef struct{
    int ext_bit;
    int from;
    int to;
}rawAST_coding_ext_group;

typedef struct{
    int fieldlength;
    int comp;
    int byteorder;              /* XDEFMSB, XDEFLSB */
    int align;                  /* XDEFLEFT, XDEFRIGHT */
    int bitorderinfield;        /* XDEFMSB, XDEFLSB */
    int bitorderinoctet;        /* XDEFMSB, XDEFLSB */
    int extension_bit;          /* XDEFYES, XDEFNO
                                   can be used for record fields:
                                   variant (field1) EXTENSION_BIT(use)*/
    int ext_bit_goup_num;
    rawAST_coding_ext_group* ext_bit_groups;
    int hexorder;               /* XDEFLOW, XDEFHIGH */
    int padding;                /* XDEFYES: next field starts at next octet */
    int fieldorder;             /* XDEFMSB, XDEFLSB */
    int lengthto_num;
    int *lengthto;              /* list of fields to generate length for */
    int pointerto;            /* pointer to the specified field is contained
                                   in this field */
    int ptrunit;                /* number of bits in pointerto value */
    int ptroffset;            /* offset to the pointer value in bits
                                   Actual position will be:
                                   pointerto*ptrunit + ptroffset */
    int pointerbase;
    int unit;                   /* number of bits in an unit */
    rawAST_coding_fields *lengthindex; /* stores subattribute of the lengthto
                                   attribute */
    rawAST_coding_taglist_list taglist;
    rawAST_coding_taglist_list crosstaglist;   /* field IDs in form of
                                   [unionField.sub]field_N,
                                   keyField.subfield_M = tagValue
                                   multiple tagValues may be specified */
    rawAST_coding_taglist presence; /* Presence indicator expressions for an
                                   optional field */
    int topleveleind;
    rawAST_toplevel toplevel;      /* Toplevel attributes */
    int union_member_num;
    const char **member_name;
    int repeatable;
    int length;			/* used only for record fields: the length of the
				   field measured in bits, it is set to -1 if
				   variable */
}raw_attrib_struct;
/* RAW enc types */


#endif

/*
 Local Variables:
 mode: C
 indent-tabs-mode: nil
 c-basic-offset: 4
 End:
*/
