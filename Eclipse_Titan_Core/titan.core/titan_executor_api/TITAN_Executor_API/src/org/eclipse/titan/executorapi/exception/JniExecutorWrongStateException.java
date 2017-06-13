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
 * This exception is thrown if any method in {@link JniExecutor} is called in wrong state.
 * @see JniExecutor
 */
public class JniExecutorWrongStateException extends JniExecutorException {

	/**
	 * Generated serial version ID
	 * (to avoid warning)
	 */
	private static final long serialVersionUID = 2924842750312826131L;

	public JniExecutorWrongStateException( final String aMsg ) {
		super( aMsg );
	}
}
