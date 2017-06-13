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
 * Verdict types.
 * <p>
 * The original C++ structure can be found at TTCNv3\core\Types.h
 * */
public final class VerdictTypeEnum {

	public static final VerdictTypeEnum NONE = new VerdictTypeEnum(0, "none");
	public static final VerdictTypeEnum PASS = new VerdictTypeEnum(1, "pass");
	public static final VerdictTypeEnum INCONC = new VerdictTypeEnum(2, "inconc");
	public static final VerdictTypeEnum FAIL = new VerdictTypeEnum(3, "fail");
	public static final VerdictTypeEnum ERROR = new VerdictTypeEnum(4, "error");

	private int enum_value;
	private final String name;

	private VerdictTypeEnum(final int value, final String name) {
		enum_value = value;
		this.name = name;
	}

	public int getValue() {
		return enum_value;
	}

	public String getName() {
		return name;
	}
	
	public String toString() {
		return name;
	}
}
