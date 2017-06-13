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

import org.eclipse.titan.executor.jni.McStateEnum;
import org.eclipse.titan.executorapi.JniExecutor;
import org.eclipse.titan.executorapi.exception.JniExecutorException;
import org.eclipse.titan.executorapi.exception.JniExecutorIllegalArgumentException;
import org.eclipse.titan.executorapi.exception.JniExecutorWrongStateException;
import org.eclipse.titan.executorapi.test.JniExecutorAsyncHappyTest.ExecutionMethod;
import org.eclipse.titan.executorapi.test.JniExecutorAsyncHappyTest.McRoute;
import org.eclipse.titan.executorapi.util.Log;

/**
 * General observer for normal asynchronous MC setup, test execution and shutdown
 * It can be parametrized by
 *   MC state route
 *   execution method
 */
class NormalTestObserver extends TestObserverBase {

	/**
	 * The length of the execute list of the cfg file,
	 * or -1 if it's not known yet. It is read in MC_READY state
	 */
	private int mExecuteCfgLen = -1;
	
	/**
	 * The index of the current item, which is currently executed
	 * Used for CFG_FILE and TEST_CONTROL
	 */
	private int mExecuteIndex = -1;
	
	protected NormalTestObserver(JniExecutor aJe, McRoute aRoute, ExecutionMethod aExecMethod) {
		super(aJe, aRoute, aExecMethod);
	}

	@Override
	public void statusChanged2( final McStateEnum aNewState ) throws JniExecutorException {
		Log.fi(aNewState);
		Log.i(""+aNewState);
		switch (aNewState) {
		case MC_LISTENING_CONFIGURED:
			mJe.startHostControllers();
			break;
		case MC_HC_CONNECTED:
			mJe.configure();
			break;
		case MC_ACTIVE:
			if ( executionReady() ) {
				mJe.shutdownSession();
			}
			else {
				mJe.createMTC();
			}
			break;
		case MC_READY:
			if ( executionReady() ) {
				mJe.exitMTC();
			}
			else {
				executeNext();
			}
			break;
		default:
			break;
		}
		Log.fo();
	}
	
	private boolean executionReady() {
		switch ( mExecMethod ) {
		case TEST_CONTROL:
			return mExecuteIndex == 0;
		case TEST_CASE:
			return mTestcases.size() == 0;
		case CFG_FILE:
			return mExecuteCfgLen != -1 && mExecuteIndex + 1 >= mExecuteCfgLen;
		default:
			return true;
		}
	}
	
	private void executeNext() throws JniExecutorWrongStateException, JniExecutorIllegalArgumentException {
		switch ( mExecMethod ) {
		case TEST_CONTROL:
			if ( mExecuteIndex == -1) {
				mJe.executeControl(mModule);
				mExecuteIndex = 0;
			}
			break;
		case TEST_CASE:
			mJe.executeTestcase( mModule, mTestcases.remove( 0 ) );
			break;
		case CFG_FILE:
			if ( mExecuteCfgLen == -1 ) {
				mExecuteCfgLen = mJe.getExecuteCfgLen();
			}
			mJe.executeCfg( ++mExecuteIndex );
			break;
		default:
			break;
		}
	}
}
