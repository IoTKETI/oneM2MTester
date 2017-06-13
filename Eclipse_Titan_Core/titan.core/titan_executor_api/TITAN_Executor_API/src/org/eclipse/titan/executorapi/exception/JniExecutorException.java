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
 * Base class of all the exceptions which can be thrown in {@link JniExecutor}.
 * @see JniExecutor
 */
public abstract class JniExecutorException extends Exception {

	/**
	 * Generated serial version ID
	 * (to avoid warning)
	 */
	private static final long serialVersionUID = -6723702347475073808L;

	public JniExecutorException( final String aMsg ) {
		super( aMsg );
	}

	public JniExecutorException( final String aMsg, final Throwable aCause ) {
		super( aMsg, aCause );
	}

}
