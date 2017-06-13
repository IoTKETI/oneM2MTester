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

/**
 * Possible states of a HC.
 * <p>
 * The original C++ structure can be found at TTCNv3\mctr2\mctr\MainController.h
 */
public enum HcStateEnum {

	HC_IDLE(0),
	HC_CONFIGURING(1),
	HC_ACTIVE(2),
	HC_OVERLOADED(3),
	HC_CONFIGURING_OVERLOADED(4),

	HC_EXITING(5),
	HC_DOWN(6);

	private int value;

	private HcStateEnum(final int aValue) {
		value = aValue;
	}

	public int getValue() {
		return value;
	}
}
