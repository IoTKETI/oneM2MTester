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
package org.eclipse.titan.executorapi;

import java.util.List;
import java.util.Map;

import org.eclipse.titan.executor.jni.McStateEnum;
import org.eclipse.titan.executor.jni.Timeval;
import org.eclipse.titan.executor.jni.VerdictTypeEnum;

/**
 * Observer interface for asynchronous requests of {@link JniExecutor}.
 * @see JniExecutor
 */
public interface IJniExecutorObserver {
	
	/**
	 * Notification about status change. It also means, that the asynchronous request is finished successfully, if aNewState is the final state.
	 * @param aNewState the new MC state
	 */
	void statusChanged( final McStateEnum aNewState );

	/**
	 * Error from MC. It also means, that the asynchronous request is finished unsuccessfully.
	 * @param aSeverity error severity
	 * @param aMsg error message
	 */
	void error( final int aSeverity, final String aMsg );

	/**
	 * Notification callback, information comes from MC
	 * @param aTime timestamp
	 * @param aSource source, the machine identifier of MC 
	 * @param aSeverity message severity
	 * @param aMsg message text
	 */
	void notify(final Timeval aTime, final String aSource, final int aSeverity, final String aMsg);
        
	/**
	 * Verdict notification, that comes after execution of a testcase.
	 * If a test control is executed, this notification is sent multiple times after each testcase.
	 * @param aTestcase name of the testcase
	 * @param aVerdictType verdict type
	 */ 
	void verdict( final String aTestcase, final VerdictTypeEnum aVerdictType );
        
	/**
	 * Verdict statistics notification, that comes after executing all the testcases after exit MTC.
	 * This notification is sent only once for a MTC session.
	 * @param aVerdictStats verdict statistics
	 */
	void verdictStats( final Map<VerdictTypeEnum, Integer> aVerdictStats );
}
