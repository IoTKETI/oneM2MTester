// README to positive conformance tests

How to run:
1. Generate a Makefile from pos_conf_test.tpd in positive_tests folder:
		ttcn3_makefilegen -f -t pos_conf_tests.tpd
2. Compile Makefile in bin folder:
		make
3. Run tests in bin folder:
		ttcn3_start pos_conf_tests
4. Clean everything in bin folder:
		make clean
