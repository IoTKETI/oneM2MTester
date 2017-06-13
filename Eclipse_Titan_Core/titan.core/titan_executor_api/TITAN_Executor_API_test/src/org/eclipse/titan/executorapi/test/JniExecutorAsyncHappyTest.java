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
 * Runs the tests of the same module in multiple ways 
 * async
 * happy day
 * Tests all the 2 state flow routes
 * Tests all the 3 execution methods
 */
public class JniExecutorAsyncHappyTest extends JniExecutorAsyncTest {

    /**
     * Route alternative in the MC state flow,
     * or in other words: the order of startHostControllers() and configure() operations
     */
	public enum McRoute {
		
		/**
		 * MC_LISTENING -> startHostControllers() -> MC_HC_CONNECTED -> configure() -> (MC_CONFIGURING) -> MC_ACTIVE
		 */
		ROUTE_1,
		
		/**
		 * MC_LISTENING -> configure() -> MC_LISTENING_CONFIGURED -> startHostControllers() -> (MC_CONFIGURING) -> MC_ACTIVE
		 */
		ROUTE_2
	};
	
	/**
	 * Determines how the tests are executed in when we get to MC_READY state
	 */
	public enum ExecutionMethod {
		
		/**
		 * Test is executed by test control name
		 */
		TEST_CONTROL,
		
		/**
		 * Test is executed by test case name, a module name and list of testcase name must be added
		 */
		TEST_CASE,
		
		/**
		 * Execution list of the cfg file is used for test execution
		 */
		CFG_FILE
	}
	
	/**
	 * Runs test of 1 module. 
	 * Sets up MC, executes tests, shuts down session. 
	 * using asynchronous functions
	 * Happy day scenario
	 * @param aHc host controller
	 * @param aCfgFileName cfg file name, a local file (not null, not empty, file must exists)
	 * @param aModule module name (can be null)
	 * @param aTestcases list of testcase names (can be null),
	 *                   ownership is NOT take, it's copied, because the list is modified
	 *                   (items are deleted one-by-one when they are executed)
	 * @param aRoute state route alternative (Route 1 or Route 2)
	 * @param aExecMethod execution method (test control, testcase or cfg execution list)
	 * @throws JniExecutorException
	 */
	private void testHappy(
			final HostController aHc,
			final String aCfgFileName,
			final String aModule,
			final List<String> aTestcases,
			final McRoute aRoute,
			final ExecutionMethod aExecMethod
			) throws JniExecutorException {
		Log.fi();
		final JniExecutor je = JniExecutor.getInstance();
		je.init();
		je.addHostController( aHc );
		je.setConfigFileName( aCfgFileName );
		final NormalTestObserver o = new NormalTestObserver( je, aRoute, aExecMethod );
		o.setModule(aModule);
		List<String> testcases = new ArrayList<>(aTestcases);
		o.setTestcases(testcases);
		je.setObserver(o);
		final Object lock = new Object();
		o.setLock( lock );
		TestUtil.assertState( McStateEnum.MC_INACTIVE );
		je.startSession();
		TestUtil.assertState( McStateEnum.MC_LISTENING );
		if ( aRoute == McRoute.ROUTE_1 ) {
			je.startHostControllers();
		} else { //if ( aRoute == McRoute.ROUTE_2 )
			je.configure();
		}
		o.waitForCompletion();
		assertTrue(o.getVerdict());
		assertTrue(!je.isConnected());
		Log.fo();
	}
	
	/**
	 * Runs test of 1 module. (It has less parameters, calls the other testHappy)
	 * Sets up MC, executes tests, shuts down session. 
	 * using asynchronous functions
	 * Happy day scenario
	 * @param aRoute state route alternative
	 * @param aExecMethod execution method (test control/testcase/cfg execution list)
	 * @throws JniExecutorException
	 */
	public void testHappy( final McRoute aRoute, final ExecutionMethod aExecMethod) throws JniExecutorException {
		Log.fi();
		final HostController hc1 = new HostController(null, TestConstants.WORKINGDIR, TestConstants.EXECUTABLE);
		final String cfgFileName = TestConstants.CFG_FILE;
		final String module = TestConstants.MODULE;
		final List<String> testcases = new ArrayList<String>();
		testcases.add(TestConstants.TESTCASE1);
		testcases.add(TestConstants.TESTCASE2);
		
		testHappy( hc1, cfgFileName, module, testcases, aRoute, aExecMethod );
		Log.fo();
	}
	
	@Test
	public void testExecutorAsyncHappyRoute1TestControl() throws JniExecutorException {
		testHappy(McRoute.ROUTE_1, ExecutionMethod.TEST_CONTROL);
	}
	
	@Test
	public void testExecutorAsyncHappyRoute1TestCasel() throws JniExecutorException {
		testHappy(McRoute.ROUTE_1, ExecutionMethod.TEST_CASE);
	}
	
	@Test
	public void testExecutorAsyncHappyRoute1CfgFilel() throws JniExecutorException {
		testHappy(McRoute.ROUTE_1, ExecutionMethod.CFG_FILE);
	}
	
	@Test
	public void testExecutorAsyncHappyRoute2TestControl() throws JniExecutorException {
		testHappy(McRoute.ROUTE_2, ExecutionMethod.TEST_CONTROL);
	}
	
	@Test
	public void testExecutorAsyncHappyRoute2TestCase() throws JniExecutorException {
		testHappy(McRoute.ROUTE_2, ExecutionMethod.TEST_CASE);
	}
	
	@Test
	public void testExecutorAsyncHappyRoute2CfgFile() throws JniExecutorException {
		testHappy(McRoute.ROUTE_2, ExecutionMethod.CFG_FILE);
	}
}
