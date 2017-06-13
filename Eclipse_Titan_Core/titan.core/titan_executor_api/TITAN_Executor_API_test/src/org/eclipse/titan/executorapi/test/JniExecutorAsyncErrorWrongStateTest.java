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
import static org.junit.Assert.fail;

import org.eclipse.titan.executor.jni.ComponentStruct;
import org.eclipse.titan.executor.jni.HostStruct;
import org.eclipse.titan.executor.jni.McStateEnum;
import org.eclipse.titan.executorapi.JniExecutor;
import org.eclipse.titan.executorapi.exception.JniExecutorIllegalArgumentException;
import org.eclipse.titan.executorapi.exception.JniExecutorJniLoadException;
import org.eclipse.titan.executorapi.exception.JniExecutorStartSessionException;
import org.eclipse.titan.executorapi.exception.JniExecutorWrongStateException;
import org.eclipse.titan.executorapi.util.Log;
import org.junit.Test;

/**
 * Testing all the JniExecutor methods in all the stable states if they can be called.
 * If method is not callable in the given state, JniExecutorWrongStateException is expected.
 */
public class JniExecutorAsyncErrorWrongStateTest extends JniExecutorAsyncErrorTest {
	
	// Method testers. Each method is tested in 2 ways:
	//  1. true: If it's callable in the given state, we expect NOT to throw JniExecutorWrongStateException and to get to the next state (if any).
	//             in this case it is recommended to check the state before and after this function using assertState 
	//  2. false: If it's NOT callable in the given state, we expect to throw JniExecutorWrongStateException
	// final boolean aCallable determines the expected behaviour for each testWrongState...() method
	
	// there is no separate method tester for the following methods, as they are called many times in the other method testers in all of the states:
	//   shutdownSession
	//   getState
	//   isConnected
	
	private void testWrongStateInit( final boolean aCallable, final JniExecutor je ) {
		if ( aCallable ) {
			try {
				je.init();
			} catch (JniExecutorWrongStateException | JniExecutorJniLoadException e) {
				fail();
			}
		} else {
			try {
				je.init();
				fail("JniExecutorWrongStateException expected");
			} catch (JniExecutorWrongStateException e) {
				//expected
			} catch (JniExecutorJniLoadException e) {
				fail();
			}
		}
	}
	
	private void testWrongStateAddHostControllers( final boolean aCallable, final JniExecutor je, final TestData td ) {
		if ( aCallable ) {
			try {
				je.addHostController( td.mHc );
			} catch (JniExecutorIllegalArgumentException | JniExecutorWrongStateException e) {
				fail();
			}			
		} else {
			try {
				je.addHostController(td.mHc);
				fail("JniExecutorWrongStateException expected");
			} catch (JniExecutorIllegalArgumentException e) {
				fail();
			} catch (JniExecutorWrongStateException e) {
				//expected
			}
		}
	}
	
	private void testWrongStateSetConfigFileName( final boolean aCallable, final JniExecutor je, final TestData td ) {
		if ( aCallable ) {
			try {
				je.setConfigFileName( td.mCfgFileName );
			} catch (JniExecutorIllegalArgumentException | JniExecutorWrongStateException e) {
				fail();
			}
		} else {
			try {
				je.setConfigFileName(td.mCfgFileName);
				fail("JniExecutorWrongStateException expected");
			} catch (JniExecutorIllegalArgumentException e) {
				fail();
			} catch (JniExecutorWrongStateException e) {
				//expected
			}
		}
	}
	
	private void testWrongStateSetObserver( final boolean aCallable, final JniExecutor je, final TestData td ) {
		if ( aCallable ) {
			try {
				je.setObserver(null);
			} catch (JniExecutorWrongStateException e) {
				fail();
			}
		} else {
			try {
				je.setObserver(td.mObserver);
				fail("JniExecutorWrongStateException expected");
			} catch (JniExecutorWrongStateException e) {
				//expected
			}
		}
	}

	private void testWrongStateStartSession( final boolean aCallable, final JniExecutor je ) {
		if ( aCallable ) {
			try {
				je.startSession();
			} catch (JniExecutorWrongStateException | JniExecutorStartSessionException e) {
				fail();
			}
		} else {
			try {
				je.startSession();
				fail("JniExecutorWrongStateException expected");
			} catch (JniExecutorWrongStateException e) {
				//expected
			} catch (JniExecutorStartSessionException e) {
				fail();
			}
		}
	}
	
	private void testWrongStateStartHostControllers( final boolean aCallable, final JniExecutor je ) {
		if ( aCallable ) {
			try {
				je.startHostControllers();
			} catch (JniExecutorWrongStateException e) {
				fail();
			}
		} else {
			try {
				je.startHostControllers();
				fail("JniExecutorWrongStateException expected");
			} catch (JniExecutorWrongStateException e) {
				//expected
			}
		}
	}
	
	private void testWrongStateConfigure( final boolean aCallable, final JniExecutor je ) {
		if ( aCallable ) {
			try {
				je.configure();
			} catch (JniExecutorWrongStateException e) {
				fail();
			}
		} else {
			try {
				je.configure();
				fail("JniExecutorWrongStateException expected");
			} catch (JniExecutorWrongStateException e) {
				//expected
			}
		}
	}

	private void testWrongStateCreateMTC( final boolean aCallable, final JniExecutor je ) {
		if ( aCallable ) {
			try {
				je.createMTC();
			} catch (JniExecutorWrongStateException e) {
				fail();
			}
		} else {
			try {
				je.createMTC();
				fail("JniExecutorWrongStateException expected");
			} catch (JniExecutorWrongStateException e) {
				//expected
			}
		}
	}
	
	private void testWrongStateExecuteControl( final boolean aCallable, final JniExecutor je, final TestData td ) {
		if ( aCallable ) {
			try {
				je.executeControl(td.mModule);
			} catch (JniExecutorWrongStateException | JniExecutorIllegalArgumentException e) {
				fail();
			}
		} else {
			try {
				je.executeControl(td.mModule);
				fail("JniExecutorWrongStateException expected");
			} catch (JniExecutorIllegalArgumentException e) {
				fail();
			} catch (JniExecutorWrongStateException e) {
				//expected
			}
		}
	}
	
	private void testWrongStateExecuteTextcase( final boolean aCallable, final JniExecutor je, final TestData td ) {
		if ( aCallable ) {
			try {
				je.executeTestcase(td.mModule, td.mTestcases.get(0));
			} catch (JniExecutorWrongStateException | JniExecutorIllegalArgumentException e) {
				fail();
			}
		} else {
			try {
				je.executeTestcase(td.mModule, td.mTestcases.get(0));
				fail("JniExecutorWrongStateException expected");
			} catch (JniExecutorIllegalArgumentException e) {
				fail();
			} catch (JniExecutorWrongStateException e) {
				//expected
			}
		}
	}

	private void testWrongStateGetExecuteCfglen( final boolean aCallable, final JniExecutor je ) {
		if ( aCallable ) {
			try {
				assertTrue(je.getExecuteCfgLen() > 0);
			} catch (JniExecutorWrongStateException e) {
				fail();
			}
		} else {
			try {
				je.getExecuteCfgLen();
				fail("JniExecutorWrongStateException expected");
			} catch (JniExecutorWrongStateException e) {
				//expected
			}
		}
	}
	
	private void testWrongStateExecuteCfg( final boolean aCallable, final JniExecutor je ) {
		if ( aCallable ) {
			try {
				je.executeCfg(0);
			} catch (JniExecutorWrongStateException | JniExecutorIllegalArgumentException e) {
				fail();
			}
		} else {
			try {
				je.executeCfg(0);
				fail("JniExecutorWrongStateException expected");
			} catch (JniExecutorWrongStateException e) {
				//expected
			} catch (JniExecutorIllegalArgumentException e) {
				fail();
			}
		}
	}
	
	private void testWrongStatePauseExecutionAndIsPaused( final boolean aCallable, final JniExecutor je ) {
		if ( aCallable ) {
			try {
				je.pauseExecution(true);
				assertTrue(je.isPaused());
				je.pauseExecution(false);
				assertTrue(!je.isPaused());
			} catch (JniExecutorWrongStateException e) {
				fail();
			}
		} else {
			try {
				je.pauseExecution(false);
				fail("JniExecutorWrongStateException expected");
			} catch (JniExecutorWrongStateException e) {
				//expected
			}
			
			try {
				je.isPaused();
				fail("JniExecutorWrongStateException expected");
			} catch (JniExecutorWrongStateException e) {
				//expected
			}
		}
	}

	private void testWrongStateContinueExecution( final boolean aCallable, final JniExecutor je ) {
		if ( aCallable ) {
			try {
				je.continueExecution();
			} catch (JniExecutorWrongStateException e) {
				fail();
			}
		} else {
			try {
				je.continueExecution();
				fail("JniExecutorWrongStateException expected");
			} catch (JniExecutorWrongStateException e) {
				//expected
			}
		}
	}
	
	private void testWrongStateStopExecution( final boolean aCallable, final JniExecutor je ) {
		if ( aCallable ) {
			try {
				je.stopExecution();
			} catch (JniExecutorWrongStateException e) {
				fail();
			}
		} else {
			try {
				je.stopExecution();
				fail("JniExecutorWrongStateException expected");
			} catch (JniExecutorWrongStateException e) {
				//expected
			}
		}
	}
	
	private void testWrongStateExitMTC( final boolean aCallable, final JniExecutor je ) {
		if ( aCallable ) {
			try {
				je.exitMTC();
			} catch (JniExecutorWrongStateException e) {
				fail();
			}
		} else {
			try {
				je.exitMTC();
				fail("JniExecutorWrongStateException expected");
			} catch (JniExecutorWrongStateException e) {
				//expected
			}
		}
	}
	
	/**
	 * Method tester for getNumberOfHosts(), getHostData() and getComponentData()
	 * @param aCallable true, if getNumberOfHosts() and getHostData() are callable
	 * @param aCallable2 true, if getComponentData() is callable
	 * @param je JniExecutor instance
	 */
	private void testWrongStateHostAndComponentData( final boolean aCallable, final boolean aCallable2, final JniExecutor je ) {
		if ( aCallable ) {
			try {
				Log.i("---------------------------------------------------");
				Log.i("state == " + je.getState() + ", " + ( je.isConnected() ? "connected" : "NOT connected"));
				final int nofHosts = je.getNumberOfHosts();
				Log.i("nofHosts == " + nofHosts);
				for ( int i = 0; i < nofHosts; i++ ) {
					final HostStruct host = je.getHostData( i );
					Log.i("host[ " + i + " ] == " + host);

					if ( aCallable2 ) {
						// number of compontents
						final int nofComponents = host.n_active_components;
						final int[] components = host.components;
						for (int componentIndex = 0; componentIndex < nofComponents; componentIndex++) {
							Log.i( "host.components[ " + i + " ] == " + components[ componentIndex ] );
							final ComponentStruct comp = je.getComponentData( components[ componentIndex ] );
							Log.i( "comp == " + comp );
						}
					} else {
						try {
							je.getComponentData(0);
							fail("JniExecutorWrongStateException expected");
						} catch ( JniExecutorWrongStateException e ) {
							//expected
						}
					}
				}
			} catch ( JniExecutorWrongStateException e ) {
				fail();
			}
		} else {
			try {
				je.getNumberOfHosts();
				fail("JniExecutorWrongStateException expected");
			} catch ( JniExecutorWrongStateException e ) {
				//expected
			}
			
			try {
				je.getHostData(0);
				fail("JniExecutorWrongStateException expected");
			} catch ( JniExecutorWrongStateException e ) {
				//expected
			}
			
			try {
				je.getComponentData(0);
				fail("JniExecutorWrongStateException expected");
			} catch ( JniExecutorWrongStateException e ) {
				//expected
			}
		}
	}
	
	/**
	 * disconnected (before init())
	 */
	@Test
	public void testExecutorWrongStateDisconnected() {
		Log.fi();
		final TestData td = createTestData1();
		final JniExecutor je = JniExecutor.getInstance();
		
		testWrongStateHostAndComponentData(false, false, je);
		
		testWrongStateAddHostControllers(false, je, td);
		testWrongStateSetConfigFileName(false, je, td);
		testWrongStateSetObserver(false, je, td);
		testWrongStateStartSession(false, je);
		testWrongStateStartHostControllers(false, je);
		testWrongStateConfigure(false, je);
		testWrongStateCreateMTC(false, je);
		testWrongStateExecuteControl(false, je, td);
		testWrongStateExecuteTextcase(false, je, td);
		testWrongStateGetExecuteCfglen(false, je);
		testWrongStateExecuteCfg(false, je);
		testWrongStatePauseExecutionAndIsPaused(false, je);
		testWrongStateContinueExecution(false, je);
		testWrongStateStopExecution(false, je);
		testWrongStateExitMTC(false, je);
		
		assertTrue( !je.isConnected() );
		assertTrue( je.getState() == McStateEnum.MC_INACTIVE );
		
		// it can be called in any state, it has no effect if we are in disconnected state
		je.shutdownSession();
		
		assertTrue( !je.isConnected() );
		assertTrue( je.getState() == McStateEnum.MC_INACTIVE );
		
		testWrongStateInit(true, je);
				
		TestUtil.assertState( McStateEnum.MC_INACTIVE );
		
		// in this case shutdownSession() is synchronous
		je.shutdownSession();
		assertTrue( !je.isConnected() );
		assertTrue( je.getState() == McStateEnum.MC_INACTIVE );
		Log.fo();
	}
	
	/**
	 * connected MC_INACTIVE state (after init())
	 */
	@Test
	public void testExecutorWrongStateInactive() {
		Log.fi();
		final TestData td = createTestData1();
		final JniExecutor je = JniExecutor.getInstance();

		gotoInactive(je, td);

		// we are now in connected MC_INACTIVE, test all the functions, first the ones, that throw exception
		testWrongStateHostAndComponentData(false, false, je);
		
		testWrongStateInit(false, je);
		testWrongStateStartHostControllers(false, je);
		testWrongStateConfigure(false, je);
		testWrongStateCreateMTC(false, je);
		testWrongStateExecuteControl(false, je, td);
		testWrongStateExecuteTextcase(false, je, td);
		testWrongStateGetExecuteCfglen(false, je);
		testWrongStateExecuteCfg(false, je);
		testWrongStatePauseExecutionAndIsPaused(true, je);
		testWrongStateContinueExecution(false, je);
		testWrongStateStopExecution(false, je);
		testWrongStateExitMTC(false, je);

		TestUtil.assertState( McStateEnum.MC_INACTIVE );
		testWrongStateAddHostControllers(true, je, td);
		TestUtil.assertState( McStateEnum.MC_INACTIVE );
		testWrongStateSetConfigFileName(true, je, td);
		TestUtil.assertState( McStateEnum.MC_INACTIVE );
		testWrongStateSetObserver(true, je, td);
		TestUtil.assertState( McStateEnum.MC_INACTIVE );
		testWrongStateStartSession(true, je);
		TestUtil.assertState( McStateEnum.MC_LISTENING );
		
		// --------------
		je.shutdownSession();
		je.waitForCompletion();
		Log.fo();
	}
	
	/**
	 * MC_LISTENING state (after startSession())
	 */
	@Test
	public void testExecutorWrongStateListening() {
		Log.fi();
		final TestData td = createTestData1();
		final JniExecutor je = JniExecutor.getInstance();
		
		gotoListening(je, td);
		
		// we are now in MC_LISTENING, test all the functions, first the ones, that throw exception
		
		testWrongStateHostAndComponentData(false, false, je);
		
		testWrongStateInit(false, je);
		testWrongStateSetConfigFileName(false, je, td);
		testWrongStateSetObserver(true, je, td);
		testWrongStateStartSession(false, je);
		testWrongStateCreateMTC(false, je);
		testWrongStateExecuteControl(false, je, td);
		testWrongStateExecuteTextcase(false, je, td);
		testWrongStateGetExecuteCfglen(false, je);
		testWrongStateExecuteCfg(false, je);
		testWrongStatePauseExecutionAndIsPaused(true, je);
		testWrongStateContinueExecution(false, je);
		testWrongStateStopExecution(false, je);
		testWrongStateExitMTC(false, je);
		
		TestUtil.assertState( McStateEnum.MC_LISTENING );
		final Object lock = new Object();
		synchronized (lock) {
			waitBefore(lock, McStateEnum.MC_HC_CONNECTED );
			testWrongStateStartHostControllers(true, je);
			waitAfter(lock);
		}
		TestUtil.assertState( McStateEnum.MC_HC_CONNECTED );

		// --------------
		je.shutdownSession();
		je.waitForCompletion();
		Log.fo();
	}
	
	/**
	 * MC_HC_CONNECTED after startHostControllers()
	 */
	@Test
	public void testExecutorWrongStateHcConnected() {
		Log.fi();
		final TestData td = createTestData1();
		final JniExecutor je = JniExecutor.getInstance();
		
		gotoHcConnected(je, td);
		
		// we are now in MC_HC_CONNECTED, test all the functions, first the ones, that throw exception
		
		testWrongStateHostAndComponentData(true, false, je);
		
		testWrongStateInit(false, je);
		testWrongStateAddHostControllers(false, je, td);
		testWrongStateSetConfigFileName(false, je, td);
		testWrongStateSetObserver(true, je, td);
		testWrongStateStartSession(false, je);
		testWrongStateStartHostControllers(false, je);
		testWrongStateCreateMTC(false, je);
		testWrongStateExecuteControl(false, je, td);
		testWrongStateExecuteTextcase(false, je, td);
		testWrongStateGetExecuteCfglen(false, je);
		testWrongStateExecuteCfg(false, je);
		testWrongStatePauseExecutionAndIsPaused(true, je);
		testWrongStateContinueExecution(false, je);
		testWrongStateStopExecution(false, je);
		testWrongStateExitMTC(false, je);
		
		TestUtil.assertState( McStateEnum.MC_HC_CONNECTED );
		final Object lock = new Object();
		synchronized (lock) {
			waitBefore(lock, McStateEnum.MC_ACTIVE );
			testWrongStateConfigure(true, je);		
			waitAfter(lock);
		}
		TestUtil.assertState( McStateEnum.MC_ACTIVE );
		
		// --------------
		je.shutdownSession();
		je.waitForCompletion();
		Log.fo();
	}
	
	/**
	 * MC_LISTENING state (after startSession()), addHostController() is called
	 */
	@Test
	public void testExecutorWrongStateListening2() {
		Log.fi();
		final TestData td = createTestData1();
		final JniExecutor je = JniExecutor.getInstance();
		
		// get to MC_LISTENING without calling addHostController()
		gotoListeningWithoutAddHostController(je, td);
		
		testWrongStateHostAndComponentData(false, false, je);
		
		TestUtil.assertState( McStateEnum.MC_LISTENING );
		testWrongStateAddHostControllers(true, je, td);
		TestUtil.assertState( McStateEnum.MC_LISTENING );
		
		// --------------
		je.shutdownSession();
		je.waitForCompletion();
		Log.fo();
	}
	
	/**
	 * MC_LISTENING_CONFIGURED state (after configure())
	 */
	@Test
	public void testExecutorWrongStateListeningConfigured() {
		Log.fi();
		final TestData td = createTestData1();
		final JniExecutor je = JniExecutor.getInstance();

		gotoListeningConfigured(je, td);
		
		// we are now in MC_LISTENING_CONFIGURED, test all the functions, first the ones, that throw exception
		
		testWrongStateHostAndComponentData(false, false, je);
		
		testWrongStateInit(false, je);
		testWrongStateSetConfigFileName(false, je, td);
		testWrongStateSetObserver(true, je, td);
		testWrongStateStartSession(false, je);
		testWrongStateCreateMTC(false, je);
		testWrongStateExecuteControl(false, je, td);
		testWrongStateExecuteTextcase(false, je, td);
		testWrongStateGetExecuteCfglen(false, je);
		testWrongStateExecuteCfg(false, je);
		testWrongStatePauseExecutionAndIsPaused(true, je);
		testWrongStateContinueExecution(false, je);
		testWrongStateStopExecution(false, je);
		testWrongStateExitMTC(false, je);
		
		TestUtil.assertState( McStateEnum.MC_LISTENING_CONFIGURED );
		final Object lock = new Object();
		synchronized (lock) {
			waitBefore(lock, McStateEnum.MC_ACTIVE );
			testWrongStateStartHostControllers(true, je);
			waitAfter(lock);
		}
		TestUtil.assertState( McStateEnum.MC_ACTIVE );

		// --------------
		je.shutdownSession();
		je.waitForCompletion();
		Log.fo();
	}
	
	/**
	 * MC_LISTENING_CONFIGURED state (after configure()) to call addHostController()
	 */
	@Test
	public void testExecutorWrongStateListeningConfigured2() {
		Log.fi();
		final TestData td = createTestData1();
		final JniExecutor je = JniExecutor.getInstance();

		gotoListeningConfiguredWithoutAddHostController(je, td);
		
		// we are now in MC_LISTENING_CONFIGURED, test all the functions, first the ones, that throw exception
		
		testWrongStateHostAndComponentData(false, false, je);
		
		testWrongStateInit(false, je);
		testWrongStateAddHostControllers(true, je, td);
		testWrongStateSetConfigFileName(false, je, td);
		testWrongStateSetObserver(true, je, td);
		testWrongStateStartSession(false, je);
		testWrongStateCreateMTC(false, je);
		testWrongStateExecuteControl(false, je, td);
		testWrongStateExecuteTextcase(false, je, td);
		testWrongStateGetExecuteCfglen(false, je);
		testWrongStateExecuteCfg(false, je);
		testWrongStatePauseExecutionAndIsPaused(true, je);
		testWrongStateContinueExecution(false, je);
		testWrongStateStopExecution(false, je);
		testWrongStateExitMTC(false, je);
		
		TestUtil.assertState( McStateEnum.MC_LISTENING_CONFIGURED );
		final Object lock = new Object();
		synchronized (lock) {
			waitBefore(lock, McStateEnum.MC_ACTIVE );
			testWrongStateStartHostControllers(true, je);
			waitAfter(lock);
		}
		TestUtil.assertState( McStateEnum.MC_ACTIVE );

		// --------------
		je.shutdownSession();
		je.waitForCompletion();
		Log.fo();
	}
	
	/**
	 * MC_ACTIVE after configure()
	 */
	@Test
	public void testExecutorWrongStateActive() {
		Log.fi();
		final TestData td = createTestData1();
		final JniExecutor je = JniExecutor.getInstance();

		gotoActive(je, td);
		
		// we are now in MC_ACTIVE, test all the functions, first the ones, that throw exception
		
		testWrongStateHostAndComponentData(true, false, je);
		
		testWrongStateInit(false, je);
		testWrongStateAddHostControllers(false, je, td);
		testWrongStateSetConfigFileName(false, je, td);
		testWrongStateSetObserver(true, je, td);
		testWrongStateStartSession(false, je);
		testWrongStateStartHostControllers(false, je);
		testWrongStateConfigure(false, je);
		testWrongStateExecuteControl(false, je, td);
		testWrongStateExecuteTextcase(false, je, td);
		testWrongStateGetExecuteCfglen(false, je);
		testWrongStateExecuteCfg(false, je);
		testWrongStatePauseExecutionAndIsPaused(true, je);
		testWrongStateContinueExecution(false, je);
		testWrongStateStopExecution(false, je);
		testWrongStateExitMTC(false, je);
		
		TestUtil.assertState( McStateEnum.MC_ACTIVE );
		final Object lock = new Object();
		synchronized (lock) {
			waitBefore(lock, McStateEnum.MC_READY );
			testWrongStateCreateMTC(true, je);
			waitAfter(lock);
		}
		TestUtil.assertState( McStateEnum.MC_READY );
		
		// --------------
		je.shutdownSession();
		je.waitForCompletion();
		Log.fo();
	}
	
	/**
	 * MC_READY after createMTC()
	 */
	@Test
	public void testExecutorWrongStateReady() {
		Log.fi();
		final TestData td = createTestData1();
		final JniExecutor je = JniExecutor.getInstance();
		
		gotoReady(je, td);
		
		// we are now in MC_READY, test all the functions, first the ones, that throw exception
		
		testWrongStateHostAndComponentData(true, true, je);

		testWrongStateInit(false, je);
		testWrongStateAddHostControllers(false, je, td);
		testWrongStateSetConfigFileName(false, je, td);
		testWrongStateSetObserver(true, je, td);
		testWrongStateStartSession(false, je);
		testWrongStateStartHostControllers(false, je);
		testWrongStateConfigure(false, je);
		testWrongStateCreateMTC(false, je);
		testWrongStatePauseExecutionAndIsPaused(true, je);
		testWrongStateContinueExecution(false, je);
		testWrongStateStopExecution(true, je); // do nothing
		
		TestUtil.assertState( McStateEnum.MC_READY );
		final Object lock1 = new Object();
		synchronized (lock1) {
			waitBefore(lock1, McStateEnum.MC_READY );
			testWrongStateExecuteControl(true, je, td);
			waitAfter(lock1);
		}
		TestUtil.assertState( McStateEnum.MC_READY );
		
		final Object lock2 = new Object();
		synchronized (lock2) {
			waitBefore(lock2, McStateEnum.MC_READY );
			testWrongStateExecuteTextcase(true, je, td);
			waitAfter(lock2);
		}
		TestUtil.assertState( McStateEnum.MC_READY );
		
		testWrongStateGetExecuteCfglen(true, je);
		TestUtil.assertState( McStateEnum.MC_READY );
		
		final Object lock3 = new Object();
		synchronized (lock3) {
			waitBefore(lock3, McStateEnum.MC_READY );
			testWrongStateExecuteCfg(true, je);
			waitAfter(lock3);
		}
		TestUtil.assertState( McStateEnum.MC_READY );
		
		testWrongStateHostAndComponentData( true, true, je );
		
		// --------------
		je.shutdownSession();
		je.waitForCompletion();
		Log.fo();
	}
	
	/**
	 * MC_READY after createMTC() to call exitMTC()
	 */
	@Test
	public void testExecutorWrongStateReady2() {
		Log.fi();
		final TestData td = createTestData1();
		final JniExecutor je = JniExecutor.getInstance();
		
		gotoReady(je, td);
		
		TestUtil.assertState( McStateEnum.MC_READY );
		final Object lock = new Object();
		synchronized (lock) {
			waitBefore(lock, McStateEnum.MC_ACTIVE );
			testWrongStateExitMTC(true, je);
			waitAfter(lock);
		}
		TestUtil.assertState( McStateEnum.MC_ACTIVE );
		
		// --------------
		je.shutdownSession();
		je.waitForCompletion();
		Log.fo();
	}
	
	/**
	 * MC_READY after createMTC() to call stopExecution()
	 */
	@Test
	public void testExecutorWrongStateReady3() {
		Log.fi();
		final TestData td = createTestData1();
		final JniExecutor je = JniExecutor.getInstance();
		
		gotoReady(je, td);
		
		TestUtil.assertState( McStateEnum.MC_READY );
		final Object lock = new Object();
		synchronized (lock) {
			waitBefore(lock, McStateEnum.MC_READY );
			try {
				je.executeTestcase(td.mModule, td.mTestcases.get(1)); // long running testcase
				// Immediately after the method call we are in MC_EXECUTING_CONTROL state until the testcase finishes,
				// when it gets to MC_READY.
				// So we could write this:
				//TestUtil.assertState( McStateEnum.MC_EXECUTING_CONTROL );
				// But MC_EXECUTING_CONTROL is an unstable state, and if the system is slow and the thread in MC runs
				// the testcase faster, than we get here or java side MC lock is blocked (JniExecutor.getState() is synchronized),
				// we are already in MC_READY, so the testcase fails.
				// It really happened sometimes during the automatic tests, so assert was removed.
				
				// If executeTestcase() is called and then stopExecution() is also called without any delay,
				// the behaviour of MainController is unpredictable:
				// Sometimes "Dynamic test case error" sent by MainController to the JniExecutor through the pipe.
				try {
				    Thread.sleep(100); // in milliseconds
				} catch(InterruptedException ex) {
				    Thread.currentThread().interrupt();
				}
				
				je.stopExecution();
			} catch (JniExecutorWrongStateException | JniExecutorIllegalArgumentException e ) {
				fail();
			}
			waitAfter(lock);
		}
		TestUtil.assertState( McStateEnum.MC_READY );
		
		// --------------
		je.shutdownSession();
		je.waitForCompletion();
		Log.fo();
	}
	
	/**
	 * MC_PAUSED, call continueExecution
	 */
	@Test
	public void testExecutorWrongStatePaused() {
		Log.fi();
		final TestData td = createTestData1();
		final JniExecutor je = JniExecutor.getInstance();
		
		gotoPaused(je, td);
		
		// we are now in MC_PAUSED, test all the functions, first the ones, that throw exception
		
		testWrongStateHostAndComponentData(false, false, je);

		testWrongStateInit(false, je);
		testWrongStateAddHostControllers(false, je, td);
		testWrongStateSetConfigFileName(false, je, td);
		testWrongStateSetObserver(true, je, td);
		testWrongStateStartSession(false, je);
		testWrongStateStartHostControllers(false, je);
		testWrongStateConfigure(false, je);
		testWrongStateCreateMTC(false, je);
		testWrongStateExecuteControl(false, je, td);
		testWrongStateExecuteTextcase(false, je, td);
		testWrongStateGetExecuteCfglen(false, je);
		testWrongStateExecuteCfg(false, je);
		
		TestUtil.assertState( McStateEnum.MC_PAUSED );
		
		final Object lock = new Object();
		synchronized (lock) {
			waitBefore(lock, McStateEnum.MC_PAUSED );
			testWrongStateContinueExecution(true, je);
			waitAfter(lock);
		}
		TestUtil.assertState( McStateEnum.MC_PAUSED );
		
		// --------------
		je.shutdownSession();
		je.waitForCompletion();
		Log.fo();
	}
	
	/**
	 * MC_PAUSED, call stopExecution
	 */
	@Test
	public void testExecutorWrongStatePaused2() {
		Log.fi();
		final TestData td = createTestData1();
		final JniExecutor je = JniExecutor.getInstance();
		
		gotoPaused(je, td);
		
		TestUtil.assertState( McStateEnum.MC_PAUSED );
		
		final Object lock = new Object();
		synchronized (lock) {
			waitBefore(lock, McStateEnum.MC_READY );
			testWrongStateStopExecution(true, je);
			waitAfter(lock);
		}
		TestUtil.assertState( McStateEnum.MC_READY );
		
		// --------------
		je.shutdownSession();
		je.waitForCompletion();
		Log.fo();
	}
	
	/**
	 * MC_PAUSED switch paused state to false -> test control continues
	 */
	@Test
	public void testExecutorWrongStatePaused3() {
		Log.fi();
		final TestData td = createTestData1();
		final JniExecutor je = JniExecutor.getInstance();
		
		gotoPaused(je, td);
		
		TestUtil.assertState( McStateEnum.MC_PAUSED );
		final Object lock = new Object();
		synchronized (lock) {
			waitBefore(lock, McStateEnum.MC_READY );
			//If we switch paused from on to off in MC_PAUSED state, execution of the next testcase will be automatically continued.
			try {
				assertTrue(je.isPaused());
				je.pauseExecution(false);
			} catch( JniExecutorWrongStateException e ) {
				fail();
			}
			waitAfter(lock);
		}
		TestUtil.assertState( McStateEnum.MC_READY );
		
		// --------------
		je.shutdownSession();
		je.waitForCompletion();
		Log.fo();
	}
	
}
