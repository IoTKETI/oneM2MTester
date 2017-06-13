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
package org.eclipse.titan.executor.jni;

import java.util.List;

/**
 * The callback interface that is used by the MainController to call back to the executor.
 * */
public interface IJNICallback {

	/**
	 * Notification about status change
	 * @param aState new MC state
	 * @see JNIMiddleWare#do_get_state()
	 */
	void statusChangeCallback( final McStateEnum aState );
	
	/**
	 * Error from MC. It also means, that the asynchronous request is finished unsuccessfully.
	 * @param aSeverity error severity
	 * @param aMsg error message
	 */
	void errorCallback( final int aSeverity, final String aMsg );
	
	/**
	 * Batch notification. Same as {@link #notifyCallback(Timeval, String, int, String)}, but more notification comes at once.
	 * @param aNotifications list of notifications
	 */
	void batchedInsertNotify( final List<String[]> aNotifications );
	
	/**
	 * Notification callback, information comes from MC
	 * @param aTime timestamp
	 * @param aSource source, the machine identifier of MC 
	 * @param aSeverity message severity
	 * @param aMsg message text
	 */
	void notifyCallback( final Timeval aTime, final String aSource, final int aSeverity, final String aMsg );
}
