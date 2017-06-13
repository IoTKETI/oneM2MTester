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

import static org.junit.Assert.assertTrue;

import org.eclipse.titan.executor.jni.McStateEnum;
import org.eclipse.titan.executorapi.JniExecutor;
import org.eclipse.titan.executorapi.util.Log;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;

/**
 * Base class of asynchronous JniExecutor tests.
 */
public abstract class JniExecutorAsyncTest extends JniExecutorTest {

	/**
	 * Common one-time initialization code for asynchronous JniExecutor tests.
	 * It runs only once for a test class.
	 */
    @BeforeClass
    public static void oneTimeSetUp() {
        // one-time initialization code
		Log.fi();
		// make sure, that session is shut down
		final JniExecutor je = JniExecutor.getInstance();
		je.shutdownSession();
		je.waitForCompletion();
		Log.fo();
    }
 
	/**
	 * Common one-time cleanup code for asynchronous JniExecutor tests.
	 * It runs only once for a test class.
	 */
    @AfterClass
    public static void oneTimeTearDown() {
        // one-time cleanup code
		Log.fi();
		Log.fo();
    }
 
	/**
	 * Common initialization code for asynchronous JniExecutor tests.
	 * It runs before each test method.
	 */
    @Before
    public void setUp() {
		Log.fi();
		// check if session is not started
		final JniExecutor je = JniExecutor.getInstance();
		assertTrue( !je.isConnected() );
		final McStateEnum state = je.getState();
		assertTrue( state == McStateEnum.MC_INACTIVE );
		Log.fo();
    }
 
	/**
	 * Common cleanup code for asynchronous JniExecutor tests.
	 * It runs before each test method.
	 */
    @After
    public void tearDown() {
		Log.fi();
		// make sure, that session is shut down
		final JniExecutor je = JniExecutor.getInstance();
		je.shutdownSession();
		je.waitForCompletion();
		Log.fo();
    }
}
