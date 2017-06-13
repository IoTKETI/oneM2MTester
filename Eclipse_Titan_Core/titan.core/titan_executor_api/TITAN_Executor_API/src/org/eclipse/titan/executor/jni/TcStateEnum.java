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
 * Possible states of a TC (MTC or PTC).
 * <p>
 * The original C++ structure can be found at TTCNv3\mctr2\mctr\MainController.h
 * */
public enum TcStateEnum {

	TC_INITIAL(0),
	TC_IDLE(1),
	TC_CREATE(2),
	TC_START(3),
	TC_STOP(4),

	TC_KILL(5),
	TC_CONNECT(6),
	TC_DISCONNECT(7),
	TC_MAP(8),
	TC_UNMAP(9),

	TC_STOPPING(10),
	TC_EXITING(11),
	TC_EXITED(12),
	MTC_CONTROLPART(13),
	MTC_TESTCASE(14),

	MTC_ALL_COMPONENT_STOP(15),
	MTC_ALL_COMPONENT_KILL(16),
	MTC_TERMINATING_TESTCASE(17),
	MTC_PAUSED(18),
	PTC_FUNCTION(19),

	PTC_STARTING(20),
	PTC_STOPPED(21),
	PTC_KILLING(22),
	PTC_STOPPING_KILLING(23),
	PTC_STALE(24),

	TC_SYSTEM(25);

	private int value;

	private TcStateEnum(final int aValue) {
		value = aValue;
	}

	public int getValue() {
		return value;
	}
}
