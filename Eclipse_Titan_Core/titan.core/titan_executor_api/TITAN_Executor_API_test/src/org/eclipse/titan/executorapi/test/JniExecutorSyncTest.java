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

import java.util.ArrayList;
import java.util.List;

import org.eclipse.titan.executor.jni.McStateEnum;
import org.eclipse.titan.executorapi.HostController;
import org.eclipse.titan.executorapi.JniExecutor;
import org.eclipse.titan.executorapi.exception.JniExecutorException;
import org.eclipse.titan.executorapi.util.Log;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

/**
 * Tests that use JniExecutorSync. So JniExecutor is tested via JniExecutorSync in a synchronous way. 
 */
public class JniExecutorSyncTest extends JniExecutorTest {

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
 
    @AfterClass
    public static void oneTimeTearDown() {
        // one-time cleanup code
		Log.fi();
		Log.fo();
    }
 
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
 
    @After
    public void tearDown() {
		Log.fi();
		// make sure, that session is shut down
		final JniExecutor je = JniExecutor.getInstance();
		je.shutdownSession();
		je.waitForCompletion();
		Log.fo();
    }

	/**
	 * sync
	 * route 1
	 * testcase by name
	 * happy day
	 * @throws JniExecutorException 
	 */
	@Test
	public void testExecutor_Sync_Route1_TestByTestCaseName_Happy() throws JniExecutorException {
		Log.fi();
		final JniExecutorSync je = JniExecutorSync.getInstance();
		je.init();
		final HostController hc1 = new HostController(null, TestConstants.WORKINGDIR, TestConstants.EXECUTABLE);
		je.addHostController(hc1);
		je.setConfigFileName(TestConstants.CFG_FILE);
		TestUtil.assertState( McStateEnum.MC_INACTIVE );
		
		Log.i("startSession()");
		je.startSession();
		TestUtil.assertState( McStateEnum.MC_LISTENING );

		Log.i("startHostControllersSync()");
		je.startHostControllersSync();
		TestUtil.assertState( McStateEnum.MC_HC_CONNECTED );
		
		Log.i("configureSync()");
		je.configureSync();
		TestUtil.assertState( McStateEnum.MC_ACTIVE );
		
		Log.i("createMTCSync()");
		je.createMTCSync();
		TestUtil.assertState( McStateEnum.MC_READY );
		
		final String module = TestConstants.MODULE;
		final List<String> testcases = new ArrayList<String>();
		testcases.add(TestConstants.TESTCASE1);
		testcases.add(TestConstants.TESTCASE2);
		while ( testcases.size() > 0 ) {
			Log.i("executeTestcaseSync()");
			je.executeTestcaseSync(module, testcases.remove(0));
			TestUtil.assertState( McStateEnum.MC_READY );
		}

		Log.i("exitMTCSync()");
		je.exitMTCSync();
		TestUtil.assertState( McStateEnum.MC_ACTIVE );
		
		Log.i("shutdownSessionSync()");
		je.shutdownSessionSync();
		Log.fo();
	}
	
	/**
	 * sync
	 * route 2
	 * testcase by name
	 * happy day
	 * @throws JniExecutorException 
	 */
	@Test
	public void testExecutor_Sync_Route2_TestByTestCaseName_Happy() throws JniExecutorException {
		Log.fi();
		final JniExecutorSync je = JniExecutorSync.getInstance();
		je.init();
		final HostController hc1 = new HostController(null, TestConstants.WORKINGDIR, TestConstants.EXECUTABLE);
		je.addHostController(hc1);
		je.setConfigFileName(TestConstants.CFG_FILE);
		TestUtil.assertState( McStateEnum.MC_INACTIVE );
		
		Log.i("startSession()");
		je.startSession();
		TestUtil.assertState( McStateEnum.MC_LISTENING );

		Log.iu("configureSync()");
		je.configureSync();
		TestUtil.assertState( McStateEnum.MC_LISTENING_CONFIGURED );
		
		Log.iu("startHostControllersSync()");
		je.startHostControllersSync();
		TestUtil.assertState( McStateEnum.MC_ACTIVE );
		
		Log.i("createMTCSync()");
		je.createMTCSync();
		TestUtil.assertState( McStateEnum.MC_READY );
		
		final String module = TestConstants.MODULE;
		final List<String> testcases = new ArrayList<String>();
		testcases.add(TestConstants.TESTCASE1);
		testcases.add(TestConstants.TESTCASE2);
		while ( testcases.size() > 0 ) {
			Log.i("executeTestcaseSync()");
			je.executeTestcaseSync(module, testcases.remove(0));
			TestUtil.assertState( McStateEnum.MC_READY );
		}

		Log.i("exitMTCSync()");
		je.exitMTCSync();
		TestUtil.assertState( McStateEnum.MC_ACTIVE );
		
		Log.i("shutdownSessionSync()");
		je.shutdownSessionSync();
		Log.fo();
	}
	
	/**
	 * sync
	 * route 1
	 * test by cfg file execution list
	 * happy day
	 * @throws JniExecutorException 
	 */
	@Test
	public void testExecutor_Sync_Route1_TestByCfg_Happy() throws JniExecutorException {
		Log.fi();
		final JniExecutorSync je = JniExecutorSync.getInstance();
		je.init();
		final HostController hc1 = new HostController(null, TestConstants.WORKINGDIR, TestConstants.EXECUTABLE);
		je.addHostController( hc1 );
		je.setConfigFileName(TestConstants.CFG_FILE);
		TestUtil.assertState( McStateEnum.MC_INACTIVE );
		
		Log.i("startSession()");
		je.startSession();
		TestUtil.assertState( McStateEnum.MC_LISTENING );

		Log.i("startHostControllersSync()");
		je.startHostControllersSync();
		TestUtil.assertState( McStateEnum.MC_HC_CONNECTED );
		
		Log.i("configureSync()");
		je.configureSync();
		TestUtil.assertState( McStateEnum.MC_ACTIVE );
		
		Log.i("createMTCSync()");
		je.createMTCSync();
		TestUtil.assertState( McStateEnum.MC_READY );

		final int executeListLen = je.getExecuteCfgLen();
		assertTrue( executeListLen > 0 );
		for ( int i = 0; i < executeListLen; i++ ) {
			Log.i("executeCfgSync()");
			je.executeCfgSync( i );
			TestUtil.assertState( McStateEnum.MC_READY );
		}

		Log.i("exitMTCSync()");
		je.exitMTCSync();
		TestUtil.assertState( McStateEnum.MC_ACTIVE );
		
		Log.i("shutdownSessionSync()");
		je.shutdownSessionSync();
		Log.fo();
	}
}
