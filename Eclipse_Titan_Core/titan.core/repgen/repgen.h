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
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#ifndef REPGEN_H
#define REPGEN_H

#define MAXLEN 2048

struct	listentry
	{
	 char   	        tcname[MAXLEN];
	 struct listentry	*next;
	};


int	WriteCode ( struct listentry *first, char *code_srcdir, char *code_dstdir, int tablen, int fillcol );
int	WriteLog ( struct listentry *first, char *log_srcdir, char *log_dstdir );
int	WriteDump ( struct listentry *first, char *dump_srcdir, char *dump_dstdir, int tablen, int fillcol );
int	WriteDump ( struct listentry *first, char *dump_srcdir, char *dump_dstdir, int tablen, int fillcol );
int	Genhtml ( struct listentry *first, char *title, char *data_dstdir );


#endif /* REPGEN_H */
