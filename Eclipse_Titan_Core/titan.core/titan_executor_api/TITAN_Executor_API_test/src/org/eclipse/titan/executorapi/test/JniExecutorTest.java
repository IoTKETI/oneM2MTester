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

import org.junit.Rule;
import org.junit.rules.Timeout;

/**
 * Base test class of all the classes, that tests JniExecutor
 * @see org.eclipse.titan.executorapi.JniExecutor 
 */
public abstract class JniExecutorTest {
	
	/** The time in milliseconds max per method tested */
	@Rule
	public Timeout globalTimeout = new Timeout(60000);
}
