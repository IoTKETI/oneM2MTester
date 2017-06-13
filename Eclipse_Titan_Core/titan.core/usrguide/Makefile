##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   
#   >
#   Baji, Laszlo
#   Balasko, Jeno
#   Forstner, Matyas
#   Gecse, Roland
#   Gillespie, Raymond
#   Kremer, Peter
#   Lovassy, Arpad
#   Ormandi, Matyas
#   Pandi, Krisztian
#   Raduly, Csaba
#   Szabados, Kristof
#   Szabo, Janos Zoltan – initial implementation
#   Szalai, Gabor
#   Tatarka, Gabor
#   Torpis, Zsolt
#   Zalanyi, Balazs Andor
#
##############################################################################
# Makefile for the TITAN User Guide

TOP  := ..
include ../Makefile.cfg

.SUFFIXES: .tex .labels .dvi .ps .pdf

DOC2PDF_JAR := ../../titan_playground/jar/doc2pdf.jar
WARNING_TXT := warning.txt
TITAN_DOCS := apiguide.doc userguide.doc installationguide.doc referenceguide.doc releasenotes.doc
TITAN_PDFS := $(TITAN_DOCS:.doc=.pdf)

ifeq ($(GEN_PDF), yes)

# Install with error handling: Tries to create PDFs and copy the available documents to $(DOCDIR). 
# On success (PDF files are successfully created), the PDF files are copied to the $(DOCDIR) directory.
# On failure (if doc2pdf.jar is not available or install_pdf exits with error, so PDFs are not created), the DOC files are copied to the $(DOCDIR) directory instead of the PDF files.
install:
	{ make test_doc2pdf_jar && make install_pdf; } || make install_error_handling

# Checks if $(DOC2PDF_JAR) is present, and exit with error if not.
test_doc2pdf_jar:
	@{ test -f $(DOC2PDF_JAR) && echo "$(DOC2PDF_JAR) is present .. OK"; } || { echo "ERROR: $(DOC2PDF_JAR) is missing" && false; };

# Install PDFs normal case: PDFs are created and copied to the $(DOCDIR) directory.
# PDF creation may fail if doc2pdf.jar fails, which can happen for example if the server, that converts from DOC to PDF, is not available. In this case the target exits with error.
install_pdf: $(TITAN_PDFS)
ifdef MINGW
$(info Skipped ${CURDIR} for MinGW)
else
	mkdir -p $(DOCDIR)
	cp $(TITAN_PDFS) $(DOCDIR)
endif

# Copies DOC files to $(DOCDIR) instead of the PDF files, because PDF creation failed. Also a $(WARNING_TXT) is created
install_error_handling:
	@echo "Error handling: DOC files are copied to $(DOCDIR) instead of PDF files"
ifdef MINGW
$(info Skipped ${CURDIR} for MinGW)
else
	mkdir -p $(DOCDIR)
	cp $(TITAN_DOCS) $(DOCDIR)
	@echo "Creating $(WARNING_TXT)"
	@echo "DOC files are copied to $(DOCDIR) instead of PDF files." >$(WARNING_TXT)
	@echo "It is because $(DOC2PDF_JAR) is missing or failed, which can happen for example if the server, that converts from DOC to PDF, is not available." >>$(WARNING_TXT)
	cp $(WARNING_TXT) $(DOCDIR)
endif

# Converts DOC to PDF
# $@ : target name, for example: apiguide.pdf
# $(basename $@) : target name without extension, for example: apiguide
%.pdf : %.doc
	@java -jar $(DOC2PDF_JAR) $(basename $@).doc $@ || { echo "ERROR: doc2pdf.jar failed with $(basename $@).doc" && false; }

else
# GEN_PDF == no
install:
endif

release:
	$(MAKE) install GEN_PDF=yes

all run:

clean:

distclean:clean

dep:

