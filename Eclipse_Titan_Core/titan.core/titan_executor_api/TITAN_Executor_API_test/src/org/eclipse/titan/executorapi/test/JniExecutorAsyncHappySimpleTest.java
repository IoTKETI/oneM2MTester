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

import static org.junit.Assert.*;

import java.util.ArrayList;
import java.util.List;

import org.eclipse.titan.executor.jni.McStateEnum;
import org.eclipse.titan.executorapi.HostController;
import org.eclipse.titan.executorapi.JniExecutor;
import org.eclipse.titan.executorapi.exception.JniExecutorException;
import org.eclipse.titan.executorapi.util.Log;
import org.junit.Test;

/**
 * The purpose of the existence of this class is to show in one place (not through abstract functions and several methods, like it is done in the other happy test classes)
 * how to navigate throw the execution flow using the API functions going to each state. 
 */
public class JniExecutorAsyncHappySimpleTest extends JniExecutorAsyncTest {

	/**
	 * async
	 * route 1
	 * testcase by name
	 * happy day
	 * @throws JniExecutorException 
	 */
	@Test
	public void testExecutor_Async_Route1_TestByTestCaseName_Happy() throws JniExecutorException {
		Log.fi();
		final JniExecutor je = JniExecutor.getInstance();
		je.init();
		final HostController hc1 = new HostController(null, TestConstants.WORKINGDIR, TestConstants.EXECUTABLE);
		je.addHostController(hc1);
		je.setConfigFileName(TestConstants.CFG_FILE);
		final String module = TestConstants.MODULE;
		final List<String> testcases = new ArrayList<String>();
		testcases.add(TestConstants.TESTCASE1);
		testcases.add(TestConstants.TESTCASE2);
		final Test1Observer o = new Test1Observer( je, module, testcases );
		je.setObserver(o);
		final Object lock = new Object();
		o.setLock( lock );
		TestUtil.assertState( McStateEnum.MC_INACTIVE );
		je.startSession();
		TestUtil.assertState( McStateEnum.MC_LISTENING );
		je.startHostControllers();
		o.waitForCompletion();
		assertTrue(o.getVerdict());
		Log.fo();
	}

	/**
	 * async
	 * route 2
	 * testcase by name
	 * happy day
	 * @throws JniExecutorException 
	 */
	@Test
	public void testExecutor_Async_Route2_TestByTestCaseName_Happy() throws JniExecutorException {
		Log.fi();
		final JniExecutor je = JniExecutor.getInstance();
		je.init();
		final HostController hc1 = new HostController(null, TestConstants.WORKINGDIR, TestConstants.EXECUTABLE);
		je.addHostController(hc1);
		je.setConfigFileName(TestConstants.CFG_FILE);
		final String module = TestConstants.MODULE;
		final List<String> testcases = new ArrayList<String>();
		testcases.add(TestConstants.TESTCASE1);
		testcases.add(TestConstants.TESTCASE2);
		final Test2Observer o = new Test2Observer( je, module, testcases );
		je.setObserver(o);
		final Object lock = new Object();
		o.setLock( lock );
		TestUtil.assertState( McStateEnum.MC_INACTIVE );
		je.startSession();
		TestUtil.assertState( McStateEnum.MC_LISTENING );
		je.configure();
		o.waitForCompletion();
		assertTrue(o.getVerdict());
		Log.fo();
	}
}
