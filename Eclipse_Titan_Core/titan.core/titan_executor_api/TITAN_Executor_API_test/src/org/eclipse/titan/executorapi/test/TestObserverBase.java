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

import java.util.List;
import java.util.Map;

import org.eclipse.titan.executor.jni.McStateEnum;
import org.eclipse.titan.executor.jni.Timeval;
import org.eclipse.titan.executor.jni.VerdictTypeEnum;
import org.eclipse.titan.executorapi.IJniExecutorObserver;
import org.eclipse.titan.executorapi.JniExecutor;
import org.eclipse.titan.executorapi.exception.JniExecutorException;
import org.eclipse.titan.executorapi.test.JniExecutorAsyncHappyTest.ExecutionMethod;
import org.eclipse.titan.executorapi.test.JniExecutorAsyncHappyTest.McRoute;
import org.eclipse.titan.executorapi.util.Log;

abstract class TestObserverBase implements IJniExecutorObserver {
	
	/** Executor instance, NOT OWNED */
	protected final JniExecutor mJe;
	
	/** TTCN-3 module name of the test control or test case(s), it can be null (it means execution list from cfg file is used) */
	protected String mModule = null;
	
	/** List of test cases, it can be null (it means test control is executed) */
	protected List<String> mTestcases = null;
	
	protected final McRoute mRoute;
	protected final ExecutionMethod mExecMethod;
	
	/**
	 * Lock object for waiting completion. NOT OWNED
	 */
	private Object mLockCompletion = null;

	/**
	 * OWNED
	 * verdict in the observer
	 *   true: PASS -> it can still be FAIL in the testcase!
	 *   false: FAIL
	 */
	private volatile boolean mVerdict = true;
	
	public void setLock( final Object aLockObject ) {
		mLockCompletion = aLockObject;
	}
	
	public boolean getVerdict() {
		return mVerdict;
	}
	
	public void setModule( final String aModule ) {
		mModule = aModule;
	}
	
	public void setTestcases( final List<String> aTestcases ) {
		mTestcases = aTestcases;
	}
	
	protected TestObserverBase( final JniExecutor aJe, final McRoute aRoute, final ExecutionMethod aExecMethod ) {
		mJe = aJe;
		mRoute = aRoute;
		mExecMethod = aExecMethod;
	}
	
	@Override
	public void statusChanged(McStateEnum aNewState) {
		Log.fi(aNewState);
		if ( aNewState == McStateEnum.MC_INACTIVE ) {
			// session is shut down
			// (It cannot be the initial MC_INACTIVE state, because we are at least
			// in MC_LISTENING state when the 1st asynchronous request is sent.)
			releaseLock();
		}
		try {
			statusChanged2( aNewState );
		} catch (JniExecutorException e) {
			Log.f( e.toString() );
			fail();
		}
		Log.fo();
	}
	
	@Override
	public void error(int aSeverity, String aMsg) {
		Log.fi( aSeverity, aMsg );
		fail();
		Log.fo();
	}

	@Override
	public void notify(Timeval aTime, String aSource, int aSeverity, String aMsg) {
		Log.fi( aTime, aSource, aSeverity, aMsg );
		Log.fo();
	}
	
	@Override
	public void verdict(String aTestcase, VerdictTypeEnum aVerdictType) {
		Log.fi( aTestcase, aVerdictType );
		Log.fo();
	}

	@Override
	public void verdictStats(Map<VerdictTypeEnum, Integer> aVerdictStats) {
		Log.fi( aVerdictStats );
		Log.fo();
	}
	
	/**
	 * Exception proof function instead of statusChanged(), exception is handled in statusChanged()
	 * @param aNewState
	 * @throws JniExecutorException
	 * @see #statusChanged(McStateEnum)
	 */
	abstract protected void statusChanged2( final McStateEnum aNewState ) throws JniExecutorException;
	
	/**
	 * releases the lock of the test client
	 */
	private void releaseLock() {
		if (mLockCompletion != null) {
			synchronized (mLockCompletion) {
				if (mLockCompletion != null) {
					mLockCompletion.notifyAll();
				}
			}
		}
	}
	
	/**
	 * Set verdict to FAIL on the observer, so to can be read by the test method
	 */
	private void fail() {
		mVerdict = false;
		releaseLock();
	}
	
	/**
	 * sets lock, waits until lock is released
	 */
	public void waitForCompletion() {
		if (mLockCompletion != null) {
			synchronized (mLockCompletion) {
				if (mLockCompletion != null) {
					try {
						mLockCompletion.wait();
					} catch (InterruptedException e) {
					}
				}
			}
		}
	}
}
