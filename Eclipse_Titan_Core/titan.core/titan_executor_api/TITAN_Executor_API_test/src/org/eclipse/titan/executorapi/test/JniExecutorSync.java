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
 * Wrapper class for synchronous usage of JniExecutor. SINGLETON
 */
public final class JniExecutorSync implements IJniExecutorObserver {
	
	/** Default timeout of synchronous functions of the asynchronous requests in ms */
	private static final int DEFAULT_TIMEOUT = 0;
	
	/**
	 * lock for synchronous access of mRequestOngoing
	 */
	private final Object mLockObject = new Object();
	
	/**
	 * Flag for signaling the asynchronous requests, used for the synchronous versions of the functions
	 */
	private volatile boolean mRequestOngoing = false;
	
	/**
	 * true, if setObserver() was called
	 */
	private boolean mObserverSet = false;
	
	/**
	 * Timeout of synchronous functions of the asynchronous requests in ms
	 */
	private long mTimeout = DEFAULT_TIMEOUT;
	
	/**
	 * Private constructor, because it is a singleton.
	 */
	private JniExecutorSync() {
	}

	/**
	 * Lazy holder for the singleton (Bill Pugh solution)
	 * Until we need an instance, the holder class will not be initialized until required and you can still use other static members of the singleton class.
	 * @see "http://en.wikipedia.org/wiki/Singleton_pattern#Initialization_On_Demand_Holder_Idiom"
	 */
	private static class SingletonHolder {
		private static final JniExecutorSync mInstance = new JniExecutorSync();
	}
	
	/**
	 * @return the singleton instance
	 */
	public static JniExecutorSync getInstance() {
		return SingletonHolder.mInstance;
	}

	@Override
	public void statusChanged( final McStateEnum aNewState ) {
		Log.fi( aNewState );
		// in case of final states
		// synchronous function is signaled that function can be ended
	    if ( !isIntermediateState( aNewState ) ) {
	    	signalEndSync();
	    }
		Log.fo();
	}

	@Override
	public void error( final int aSeverity, final String aMsg ) {
		Log.fi( aSeverity, aMsg );
    	signalEndSync();
		Log.fo();
	}

	@Override
	public void notify(final Timeval aTime, final String aSource, final int aSeverity, final String aMsg) {
	}

	@Override
	public void verdict(String aTestcase, VerdictTypeEnum aVerdictType) {
	}

	@Override
	public void verdictStats(Map<VerdictTypeEnum, Integer> aVerdictStats) {
	}
	
	public void init() throws JniExecutorWrongStateException, JniExecutorJniLoadException {
		JniExecutor.getInstance().init();
		mRequestOngoing = false;
		mObserverSet = false;
	}

	public void addHostController( final HostController aHc ) throws JniExecutorIllegalArgumentException, JniExecutorWrongStateException {
		JniExecutor.getInstance().addHostController( aHc );
	}

	public void setConfigFileName( final String aConfigFileName ) throws JniExecutorWrongStateException, JniExecutorIllegalArgumentException {
		JniExecutor.getInstance().setConfigFileName( aConfigFileName );
	}
	
	public void startSession() throws JniExecutorWrongStateException, JniExecutorStartSessionException {
		JniExecutor.getInstance().startSession();
	}
	
	public int getExecuteCfgLen() throws JniExecutorWrongStateException {
		return JniExecutor.getInstance().getExecuteCfgLen();
	}
	
	/**
	 * Sets new timeout
	 * @param aTimeout the new timeout value in ms,
	 *                 or 0, if no timeout
	 */
	public void setTimeout( final long aTimeout ) {
		mTimeout = aTimeout;
	}
	
	/**
	 * Sets observer if it's not set yet.
	 * It must be called before the 1st asynchronous function call.
	 * @throws JniExecutorWrongStateException 
	 */
	private void setObserver() throws JniExecutorWrongStateException {
		if ( !mObserverSet ) {
			JniExecutor.getInstance().setObserver( this );
			mObserverSet = true;
		}
	}
	
	// ------------ SYNCHRONOUS VERSION OF ASYNCHRONOUS FUNCTIONS --------------------
	
	
	public void startHostControllersSync() throws JniExecutorWrongStateException {
		// observer is set before the 1st async request
		setObserver();
		startSync();
		JniExecutor.getInstance().startHostControllers();
		endSync();
	}
	
	public void configureSync() throws JniExecutorWrongStateException {
		// observer is set before the 1st async request
		setObserver();
		startSync();
		JniExecutor.getInstance().configure();
		endSync();
	}
	
	public void createMTCSync() throws JniExecutorWrongStateException {
		startSync();
		JniExecutor.getInstance().createMTC();
		endSync();
	}
	
	public void executeControlSync( final String aModule ) throws JniExecutorWrongStateException, JniExecutorIllegalArgumentException {
		startSync();
		JniExecutor.getInstance().executeControl( aModule );
		endSync();
	}
	
	public void executeTestcaseSync( final String aModule, final String aTestcase ) throws JniExecutorWrongStateException, JniExecutorIllegalArgumentException {
		startSync();
		JniExecutor.getInstance().executeTestcase( aModule, aTestcase );
		endSync();
	}
	
	public void executeCfgSync( final int aIndex ) throws JniExecutorWrongStateException, JniExecutorIllegalArgumentException {
		startSync();
		JniExecutor.getInstance().executeCfg( aIndex );
		endSync();
	}
	
	public void continueExecutionSync() throws JniExecutorWrongStateException {
		startSync();
		JniExecutor.getInstance().continueExecution();
		endSync();
	}
	
	public void exitMTCSync() throws JniExecutorWrongStateException {
		startSync();
		JniExecutor.getInstance().exitMTC();
		endSync();
	}
	
	public void shutdownSessionSync() {
		startSync();
		JniExecutor.getInstance().shutdownSession();
		endSync();
	}

	private void startSync() {
		Log.fi();
		synchronized (mLockObject) {
			Log.i("startSync()");
			// Make sure, that ...Sync() functions do NOT call each other
			if ( mRequestOngoing ) {
				//This should not happen, another request is ongoing
				Log.i("  startSync() mRequestOngoing == true --- This should not happen, another request is ongoing");
			}
			mRequestOngoing = true;
		}
		Log.fo();
	}
	
	private void endSync() {
		Log.fi();
		synchronized (mLockObject) {
			if ( !mRequestOngoing ) {
				// The request is already finished, faster than the ...Sync() function
				Log.i("  endSync() mRequestOngoing == false --- The request is already finished");
			}
			else {
				try {
					if ( mTimeout == 0 ) {
						// no timeout
						mLockObject.wait();
					} else {
						mLockObject.wait( mTimeout );
					}
				} catch (InterruptedException e) {
					Log.f(e.toString());
				}
			}
			Log.i("endSync()");
		}
		Log.fo();
	}
	
	private void signalEndSync() {
		Log.fi();
		synchronized (mLockObject) {
			Log.i("signalEndSync()");
			if ( !mRequestOngoing ) {
				// There is no synchronous function called
				Log.i("  signalEndSync() There is no synchronous function called");
			}
			mLockObject.notifyAll();
			mRequestOngoing = false;
		}
		Log.fo();
	}

	/**
	 * Checks if state is intermediate state. Intermediate state is a state, when asynchronous request is ongoing.
	 * When it is finished, it will switched to some final state automatically.
	 * @param aState the state to check
	 * @return if state is intermediate
	 */
	private static boolean isIntermediateState( final McStateEnum aState ) {
		return
				aState == McStateEnum.MC_LISTENING            || // it is stable, but cannot be a result state of any async request
				aState == McStateEnum.MC_CONFIGURING          ||
				aState == McStateEnum.MC_CREATING_MTC         ||
				aState == McStateEnum.MC_TERMINATING_MTC      ||
				aState == McStateEnum.MC_EXECUTING_CONTROL    ||
				aState == McStateEnum.MC_EXECUTING_TESTCASE   ||
				aState == McStateEnum.MC_TERMINATING_TESTCASE ||
				aState == McStateEnum.MC_SHUTDOWN;
	}
}
