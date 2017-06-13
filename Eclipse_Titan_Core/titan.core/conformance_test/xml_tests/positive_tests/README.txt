// README to positive xml conformance tests

How to run:
A. 
1. Create bin folder in positive_tests and copy XML_tests.cfg into it
2. Use the Makefile in positive tests folder
 
B. (Without the Makefile) 
1. Generate a Makefile from XML_tests.tpd in positive_tests folder:
		ttcn3_makefilegen -f -t XML_tests.tpd
2. Create bin folder in positive_tests and copy XML_tests.cfg into it
3. Compile Makefile in bin folder:
		make
4. Run tests in bin folder:
		ttcn3_start XML_tests XML_tests.cfg
5. Clean everything in bin folder:
		make clean
