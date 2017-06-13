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

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import org.eclipse.titan.executor.jni.McStateEnum;
import org.eclipse.titan.executor.jni.Timeval;
import org.eclipse.titan.executor.jni.VerdictTypeEnum;
import org.eclipse.titan.executorapi.HostController;
import org.eclipse.titan.executorapi.IJniExecutorObserver;
import org.eclipse.titan.executorapi.JniExecutor;
import org.eclipse.titan.executorapi.exception.JniExecutorIllegalArgumentException;
import org.eclipse.titan.executorapi.exception.JniExecutorJniLoadException;
import org.eclipse.titan.executorapi.exception.JniExecutorStartSessionException;
import org.eclipse.titan.executorapi.exception.JniExecutorWrongStateException;
import org.eclipse.titan.executorapi.util.Log;

/**
 * Base class of asynchronous JniExecutor error tests.
 */
public abstract class JniExecutorAsyncErrorTest extends JniExecutorAsyncTest {
	
	/**
	 * structure to hold all the needed data to run tests
	 *
	 */
	protected class TestData {
		public HostController mHc = null;
		public String mCfgFileName = null;
		public String mModule = null;
		public List<String> mTestcases = null;
		public IJniExecutorObserver mObserver = null;
	}
	
	protected  TestData createTestData1() {
		HostController hc1 = null;
		try {
			hc1 = new HostController(null, TestConstants.WORKINGDIR, TestConstants.EXECUTABLE);
		} catch (JniExecutorIllegalArgumentException e) {
			fail();
		}
		final String cfgFileName = TestConstants.CFG_FILE;
		final String module = TestConstants.MODULE;
		final List<String> testcases = new ArrayList<String>();
		testcases.add(TestConstants.TESTCASE1);
		testcases.add(TestConstants.TESTCASE2);
		
		final JniExecutor je = JniExecutor.getInstance();
		IJniExecutorObserver o1 = new Test1Observer(je, module, testcases);
		
		TestData td = new TestData();
		td.mHc = hc1;
		td.mCfgFileName = cfgFileName;
		td.mModule = module;
		td.mTestcases = testcases;
		td.mObserver = o1;
		return td;
	}
	
	// functions to get to different states
	
	protected void gotoInactive( final JniExecutor je, final TestData td ) {
		Log.fi();
		// get to connected MC_INACTIVE
		try {
			je.init();
			TestUtil.assertState( McStateEnum.MC_INACTIVE );
		} catch (JniExecutorWrongStateException | JniExecutorJniLoadException e) {
			fail();
		}
		Log.fo();
	}
	
	protected void gotoListening( final JniExecutor je, final TestData td ) {
		Log.fi();
		// get to MC_LISTENING
		gotoInactive(je, td);
		
		try {
			TestUtil.assertState( McStateEnum.MC_INACTIVE );
			
			je.addHostController( td.mHc );
			TestUtil.assertState( McStateEnum.MC_INACTIVE );
			
			je.setConfigFileName( td.mCfgFileName );
			TestUtil.assertState( McStateEnum.MC_INACTIVE );
			
			je.setObserver(null);
			TestUtil.assertState( McStateEnum.MC_INACTIVE );
			
			je.startSession();
			TestUtil.assertState( McStateEnum.MC_LISTENING );
		} catch (JniExecutorIllegalArgumentException | JniExecutorWrongStateException | JniExecutorStartSessionException e) {
			fail();
		}
		Log.fo();
	}
	
	protected void gotoListeningConfigured( final JniExecutor je, final TestData td ) {
		Log.fi();
		// get to MC_LISTENING_CONFIGURED
		gotoListening(je, td);
		
		try {
			TestUtil.assertState( McStateEnum.MC_LISTENING );
			final Object lock = new Object();
			synchronized (lock) {
				waitBefore(lock, McStateEnum.MC_LISTENING_CONFIGURED );
				je.configure();
				waitAfter(lock);
			}
			TestUtil.assertState( McStateEnum.MC_LISTENING_CONFIGURED );
		} catch (JniExecutorWrongStateException e) {
			fail();
		}
		Log.fo();
	}
	
	protected void gotoListeningWithoutAddHostController( final JniExecutor je, final TestData td ) {
		Log.fi();
		// get to MC_LISTENING
		gotoInactive(je, td);
		
		try {
			TestUtil.assertState( McStateEnum.MC_INACTIVE );
			
			je.setConfigFileName( td.mCfgFileName );
			TestUtil.assertState( McStateEnum.MC_INACTIVE );
			
			je.setObserver(null);
			TestUtil.assertState( McStateEnum.MC_INACTIVE );
			
			je.startSession();
			TestUtil.assertState( McStateEnum.MC_LISTENING );
		} catch (JniExecutorIllegalArgumentException | JniExecutorWrongStateException | JniExecutorStartSessionException e) {
			fail();
		}
		Log.fo();
	}
	
	protected void gotoListeningConfiguredWithoutAddHostController( final JniExecutor je, final TestData td ) {
		Log.fi();
		// get to MC_LISTENING_CONFIGURED without addHostController()
		gotoListeningWithoutAddHostController(je, td);
		
		try {
			TestUtil.assertState( McStateEnum.MC_LISTENING );
			final Object lock = new Object();
			synchronized (lock) {
				waitBefore(lock, McStateEnum.MC_LISTENING_CONFIGURED );
				je.configure();
				waitAfter(lock);
			}
			TestUtil.assertState( McStateEnum.MC_LISTENING_CONFIGURED );
		} catch (JniExecutorWrongStateException e) {
			fail();
		}
		Log.fo();
	}
	
	protected void gotoHcConnected( final JniExecutor je, final TestData td ) {
		Log.fi();
		// get to MC_HC_CONNECTED
		gotoListening(je, td);
		
		try {
			TestUtil.assertState( McStateEnum.MC_LISTENING );
			final Object lock = new Object();
			synchronized (lock) {
				waitBefore(lock, McStateEnum.MC_HC_CONNECTED );
				je.startHostControllers();
				waitAfter(lock);
			}
			TestUtil.assertState( McStateEnum.MC_HC_CONNECTED );
		} catch (JniExecutorWrongStateException e) {
			fail();
		}
		Log.fo();
	}
	
	protected void gotoActive( final JniExecutor je, final TestData td ) {
		Log.fi();
		// get to MC_ACTIVE
		gotoHcConnected(je, td);
		
		try {
			TestUtil.assertState( McStateEnum.MC_HC_CONNECTED );
			final Object lock = new Object();
			synchronized (lock) {
				waitBefore(lock, McStateEnum.MC_ACTIVE );
				je.configure();
				waitAfter(lock);
			}
			TestUtil.assertState( McStateEnum.MC_ACTIVE );
		} catch (JniExecutorWrongStateException e) {
			fail();
		}
		Log.fo();
	}
	
	protected void gotoReady( final JniExecutor je, final TestData td ) {
		Log.fi();
		// get to MC_READY
		gotoActive(je, td);
		
		try {
			TestUtil.assertState( McStateEnum.MC_ACTIVE );
			final Object lock = new Object();
			synchronized (lock) {
				waitBefore(lock, McStateEnum.MC_READY );
				je.createMTC();
				waitAfter(lock);
			}
			TestUtil.assertState( McStateEnum.MC_READY );
		} catch (JniExecutorWrongStateException e) {
			fail();
		}
		Log.fo();
	}
	
	protected void gotoPaused( final JniExecutor je, final TestData td ) {
		Log.fi();
		// get to MC_PAUSED
		gotoReady(je, td);
		
		try {
			TestUtil.assertState( McStateEnum.MC_READY );
			je.pauseExecution(true);
			final Object lock = new Object();
			synchronized (lock) {
				waitBefore(lock, McStateEnum.MC_PAUSED );
				je.executeControl(td.mModule);
				waitAfter(lock);
			}
			TestUtil.assertState( McStateEnum.MC_PAUSED );
		} catch (JniExecutorWrongStateException | JniExecutorIllegalArgumentException e) {
			fail();
		}
		Log.fo();
	}
	
	/**
	 * First part of waiting until asynchronous function gets to the expected state.
	 * <p>
	 * This must be called BEFORE calling the asynchronous function to make sure that observer
	 * does the right action, if statusChange() is called back before asynchronous function returns.
	 * <br>
	 * Example usage:
	 * <pre>
	 * final Object lock = new Object();
	 * synchronized ( lock ) {
	 *     waitBefore( McStateEnum.MC_READY );
	 *     je.createMTC();
	 *     waitAfter( lock );
	 * }
	 * </pre>
	 * @param aLock lock for synchronization
	 * @param aExpectedState the expected state we are waiting for
	 * @see #waitAfter(Object)
	 */
	protected static void waitBefore( final Object aLock, final McStateEnum aExpectedState ) {
		Log.fi( aExpectedState );
		final JniExecutor je = JniExecutor.getInstance();
		IJniExecutorObserver o = new IJniExecutorObserver() {
			@Override
			public void statusChanged( final McStateEnum aNewState ) {
				Log.fi(aNewState);
				if ( aNewState == aExpectedState ) {
					synchronized (aLock) {
						aLock.notifyAll();
					}
				}
				Log.fo();
			}
			
			@Override
			public void error(int aSeverity, String aMsg) {
				Log.fi(aSeverity, aMsg);
				synchronized (aLock) {
					aLock.notifyAll();
				}
				Log.fo();
			}

			@Override
			public void notify(Timeval aTime, String aSource, int aSeverity, String aMsg) {
			}
			
			@Override
			public void verdict(String aTestcase, VerdictTypeEnum aVerdictType) {
			}

			@Override
			public void verdictStats(Map<VerdictTypeEnum, Integer> aVerdictStats) {
			}
		};
		try {
			je.setObserver(o);
		} catch (JniExecutorWrongStateException e) {
			Log.f(e.toString());
			//it happens only in disconnected state, but it cannot happen,
			//we get here only if we are connected
			fail();
		}
		Log.fo();
	}
	
	/**
	 * Pair of {@link #waitBefore(Object, McStateEnum)}
	 * @param aLock lock for synchronization
	 * @see #waitBefore(Object, McStateEnum)
	 */
	protected static void waitAfter( final Object aLock ) {
		Log.fi();
		try {
			Log.f("aLock.wait() BEFORE");
			aLock.wait();
			Log.f("aLock.wait() AFTER");
		} catch (InterruptedException e) {
			Log.f(e.toString());
			fail();
		}
		Log.fo();
	}
}
