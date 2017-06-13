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

import org.eclipse.titan.executor.jni.McStateEnum;
import org.eclipse.titan.executorapi.JniExecutor;
import org.eclipse.titan.executorapi.exception.JniExecutorException;
import org.eclipse.titan.executorapi.test.JniExecutorAsyncHappyTest.ExecutionMethod;
import org.eclipse.titan.executorapi.test.JniExecutorAsyncHappyTest.McRoute;
import org.eclipse.titan.executorapi.util.Log;

/**
 * observer
 *  async
 *  route 1: MC_LISTENING -> startHostControllers() -> MC_HC_CONNECTED -> configure() ->(MC_CONFIGURING) -> MC_ACTIVE
 *  testcase by name 
 */
class Test1Observer extends TestObserverBase {
	
	public Test1Observer( final JniExecutor aJe, final String aModule, final List<String> aTestcases ) {
		super( aJe, McRoute.ROUTE_1, ExecutionMethod.TEST_CASE );
		setModule(aModule);
		setTestcases(aTestcases);
	}
	
	@Override
	public void statusChanged2( final McStateEnum aNewState ) throws JniExecutorException {
		Log.fi(aNewState);
		switch (aNewState) {
		case MC_HC_CONNECTED:
			mJe.configure();
			break;
		case MC_ACTIVE:
			if ( mTestcases.size() > 0 ) {
				mJe.createMTC();
			}
			else {
				mJe.shutdownSession();
			}
			break;
		case MC_READY:
			if ( mTestcases.size() > 0 ) {
				mJe.executeTestcase( mModule, mTestcases.remove( 0 ) );
			}
			else {
				mJe.exitMTC();
			}
			break;
		default:
			break;
		}
		Log.fo();
	}
}
