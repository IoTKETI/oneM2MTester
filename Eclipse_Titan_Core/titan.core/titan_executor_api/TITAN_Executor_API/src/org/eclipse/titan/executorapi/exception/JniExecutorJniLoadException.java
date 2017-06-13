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
package org.eclipse.titan.executorapi.exception;

import org.eclipse.titan.executorapi.JniExecutor;

/**
 * This exception is thrown in {@link JniExecutor#init()} if C++ JNI library is failed to load.
 * The reason can be one of the following:
 * <ul>
 *   <li> The application was started on Windows, Windows is NOT supported 
 *   <li> Titan JNI library libmctrjninative.so does NOT exist
 *   <li> LD_LIBRARY_PATH is not set up, the path must contain the directory, where libmctrjninative.so is located
 * </ul> 
 * @see JniExecutor#init()
 */
public class JniExecutorJniLoadException extends JniExecutorException {
	
	/**
	 * Generated serial version ID
	 * (to avoid warning)
	 */
	private static final long serialVersionUID = -1887674537433032025L;

	public JniExecutorJniLoadException( final String aMsg ) {
		super( aMsg );
	}
	

	public JniExecutorJniLoadException(String aMsg, final Throwable aCause) {
		super( aMsg, aCause );
	}

}
