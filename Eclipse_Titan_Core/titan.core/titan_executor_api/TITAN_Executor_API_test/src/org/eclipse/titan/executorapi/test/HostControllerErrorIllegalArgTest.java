/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Lovassy, Arpad
 *
 ******************************************************************************/
package org.eclipse.titan.executorapi.test;

import static org.junit.Assert.fail;

import org.eclipse.titan.executorapi.HostController;
import org.eclipse.titan.executorapi.exception.JniExecutorIllegalArgumentException;
import org.junit.Test;

/**
 * Testing all the HostController methods with illegal arguments.
 * If 1 or more arguments are illegal (null, empty string, filename for a non-existent file, ...),
 * JniExecutorIllegalArgException is expected.
 */
public class HostControllerErrorIllegalArgTest {
	
	@Test
	public void testHostControllerConstructor() {
		
		// working dir null
		try {
			new HostController(null, null, TestConstants.EXECUTABLE);
			fail("JniExecutorIllegalArgumentException expected");
		} catch (JniExecutorIllegalArgumentException e) {
			//expected
		}
		
		// executable null
		try {
			new HostController(null, TestConstants.WORKINGDIR, null);
			fail("JniExecutorIllegalArgumentException expected");
		} catch (JniExecutorIllegalArgumentException e) {
			//expected
		}
		
		// working dir does NOT exist
		try {
			new HostController(null, TestConstants.WORKINGDIR + TestConstants.NONEXISTENTFILENAMEEXT, TestConstants.EXECUTABLE);
			fail("JniExecutorIllegalArgumentException expected");
		} catch (JniExecutorIllegalArgumentException e) {
			//expected
		}
		
		// working dir exists, but not a directory
		try {
			new HostController(null, TestConstants.EXISTS_BUT_NOT_DIR_WORKINGDIR, TestConstants.EXECUTABLE);
			fail("JniExecutorIllegalArgumentException expected");
		} catch (JniExecutorIllegalArgumentException e) {
			//expected
		}
		
		// executable does NOT exist
		try {
			new HostController(null, TestConstants.WORKINGDIR, TestConstants.EXECUTABLE + TestConstants.NONEXISTENTFILENAMEEXT);
			fail("JniExecutorIllegalArgumentException expected");
		} catch (JniExecutorIllegalArgumentException e) {
			//expected
		}
		
		// executable exists, but not a file (it's a directory)
		try {
			new HostController(null, TestConstants.EXISTS_BUT_NOT_FILE_WORKINGDIR, TestConstants.EXISTS_BUT_NOT_FILE_EXECUTABLE);
			fail("JniExecutorIllegalArgumentException expected");
		} catch (JniExecutorIllegalArgumentException e) {
			//expected
		}

		// happy day, it must not throw exception
		try {
			new HostController(null, TestConstants.WORKINGDIR, TestConstants.EXECUTABLE);
		} catch (JniExecutorIllegalArgumentException e) {
			fail();
		}
	}
}
