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

import org.eclipse.titan.executorapi.JniExecutor;
import org.eclipse.titan.executorapi.exception.JniExecutorIllegalArgumentException;
import org.eclipse.titan.executorapi.exception.JniExecutorWrongStateException;
import org.junit.Test;

/**
 * Testing all the JniExecutor methods in all the stable states with illegal arguments.
 * If method is callable in the given state and 1 or more arguments are illegal (null, empty string, filename for a non-existent file, ...),
 * JniExecutorIllegalArgException is expected.
 */
public class JniExecutorAsyncErrorIllegalArgTest extends JniExecutorAsyncErrorTest {
	
	private void testIllegalArgAddHostController( final JniExecutor je ) {
		try {
			je.addHostController( null );
			fail("JniExecutorIllegalArgumentException expected");
		} catch (JniExecutorIllegalArgumentException e) {
			//expected
		} catch (JniExecutorWrongStateException e) {
			fail();
		}
	}
	
	private void testIllegalArgSetConfigFileName( final JniExecutor je ) {
		try {
			je.setConfigFileName( null );
			fail("JniExecutorIllegalArgumentException expected");
		} catch (JniExecutorIllegalArgumentException e) {
			//expected
		} catch (JniExecutorWrongStateException e) {
			fail();
		}
		
		try {
			je.setConfigFileName( "" );
			fail("JniExecutorIllegalArgumentException expected");
		} catch (JniExecutorIllegalArgumentException e) {
			//expected
		} catch (JniExecutorWrongStateException e) {
			fail();
		}
	}
	
	private void testIllegalArgSetObserver( final JniExecutor je, final TestData td ) {
		try {
			je.setObserver(td.mObserver);
			je.setObserver(null);
			// any observer including null is a valid argument
		} catch (JniExecutorWrongStateException e) {
			fail();
		}
	}
	
	private void testIllegalArgExecuteControl( final JniExecutor je ) {
		try {
			je.executeControl(null);
			fail("JniExecutorIllegalArgumentException expected");
		} catch (JniExecutorIllegalArgumentException e) {
			//expected
		} catch (JniExecutorWrongStateException e) {
			fail();
		}
		
		try {
			je.executeControl("");
			fail("JniExecutorIllegalArgumentException expected");
		} catch (JniExecutorIllegalArgumentException e) {
			//expected
		} catch (JniExecutorWrongStateException e) {
			fail();
		}
	}
	
	private void testIllegalArgExecuteTestcase( final JniExecutor je, final TestData td ) {
		try {
			je.executeTestcase( null, td.mTestcases.get(0));
			fail("JniExecutorIllegalArgumentException expected");
		} catch (JniExecutorIllegalArgumentException e) {
			//expected
		} catch (JniExecutorWrongStateException e) {
			fail();
		}
		try {
			je.executeTestcase( "", td.mTestcases.get(0));
			fail("JniExecutorIllegalArgumentException expected");
		} catch (JniExecutorIllegalArgumentException e) {
			//expected
		} catch (JniExecutorWrongStateException e) {
			fail();
		}
		try {
			je.executeTestcase( td.mModule, null);
			fail("JniExecutorIllegalArgumentException expected");
		} catch (JniExecutorIllegalArgumentException e) {
			//expected
		} catch (JniExecutorWrongStateException e) {
			fail();
		}
		try {
			je.executeTestcase( td.mModule, "");
			fail("JniExecutorIllegalArgumentException expected");
		} catch (JniExecutorIllegalArgumentException e) {
			//expected
		} catch (JniExecutorWrongStateException e) {
			fail();
		}
	}
	
	private void testIllegalArgExecuteCfg( final JniExecutor je ) {
		try {
			je.executeCfg( -1 );
			fail("JniExecutorIllegalArgumentException expected");
		} catch (JniExecutorIllegalArgumentException e) {
			//expected
		} catch (JniExecutorWrongStateException e) {
			fail();
		}
		
		try {
			final int executeCfgLen = je.getExecuteCfgLen();
			je.executeCfg( executeCfgLen );
			fail("JniExecutorIllegalArgumentException expected");
		} catch (JniExecutorIllegalArgumentException e) {
			//expected
		} catch (JniExecutorWrongStateException e) {
			fail();
		}
	}
	
	/**
	 * disconnected (before init())
	 */
	@Test
	public void testExecutorIllegalArgDisconnected() {
		//nothing to test in disconnected state
	}
	
	/**
	 * connected MC_INACTIVE state (after init())
	 */
	@Test
	public void testExecutorIllegalArgInactive() {
		final TestData td = createTestData1();
		final JniExecutor je = JniExecutor.getInstance();

		gotoInactive(je, td);

		// we are now in connected MC_INACTIVE, test all the functions, which has argument
		testIllegalArgAddHostController(je);
		testIllegalArgSetConfigFileName(je);
		testIllegalArgSetObserver(je, td);
		
		// --------------
		je.shutdownSession();
		je.waitForCompletion();
	}
	
	/**
	 * MC_LISTENING state (after startSession())
	 */
	@Test
	public void testExecutorIllegalArgListening() {
		final TestData td = createTestData1();
		final JniExecutor je = JniExecutor.getInstance();
		
		gotoListening(je, td);
		
		// we are now in MC_LISTENING, test all the functions, which has argument
		testIllegalArgAddHostController(je);
		testIllegalArgSetObserver(je, td);

		// --------------
		je.shutdownSession();
		je.waitForCompletion();
	}
	
	/**
	 * MC_HC_CONNECTED after startHostControllers()
	 */
	@Test
	public void testExecutorIllegalArgHcConnected() {
		final TestData td = createTestData1();
		final JniExecutor je = JniExecutor.getInstance();
		
		gotoHcConnected(je, td);
		
		// we are now in MC_HC_CONNECTED, test all the functions, which has argument
		testIllegalArgSetObserver(je, td);

		// --------------
		je.shutdownSession();
		je.waitForCompletion();
	}
	
	/**
	 * MC_LISTENING_CONFIGURED state (after configure())
	 */
	@Test
	public void testExecutorIllegalArgListeningConfigured() {
		final TestData td = createTestData1();
		final JniExecutor je = JniExecutor.getInstance();

		gotoListeningConfigured(je, td);
		
		// we are now in MC_LISTENING_CONFIGURED, test all the functions, which has argument
		testIllegalArgAddHostController(je);
		testIllegalArgSetObserver(je, td);

		// --------------
		je.shutdownSession();
		je.waitForCompletion();
	}
	
	/**
	 * MC_ACTIVE after configure()
	 */
	@Test
	public void testExecutorIllegalArgActive() {
		final TestData td = createTestData1();
		final JniExecutor je = JniExecutor.getInstance();

		gotoActive(je, td);
		
		// we are now in MC_ACTIVE, test all the functions, which has argument
		testIllegalArgSetObserver(je, td);

		// --------------
		je.shutdownSession();
		je.waitForCompletion();
	}
	
	/**
	 * MC_READY after createMTC()
	 */
	@Test
	public void testExecutorIllegalArgReady() {
		final TestData td = createTestData1();
		final JniExecutor je = JniExecutor.getInstance();
		
		gotoReady(je, td);
		
		// we are now in MC_READY, test all the functions, which has argument
		testIllegalArgSetObserver(je, td);
		testIllegalArgExecuteControl(je);
		testIllegalArgExecuteTestcase(je, td);
		testIllegalArgExecuteCfg(je);
		
		// --------------
		je.shutdownSession();
		je.waitForCompletion();
	}
}
