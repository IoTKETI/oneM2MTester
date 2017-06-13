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

import static org.junit.Assert.assertEquals;

import org.eclipse.titan.executorapi.util.Log;
import org.eclipse.titan.executorapi.util.StringUtil;
import org.junit.Test;

public class StringUtilTest {

	@Test
	public void testAppendObjectArray() {
		Log.fi();
		final String EXPECTED = "[ false, true, false, true ], [ a, b, c, d ], [ 0, 1, 2, 3 ], [ 0, 1, 2, 3 ], [ 0, 1, 2, 3 ], [ 0, 1, 2, 3 ], [ 0.0, 1.0, 2.0, 3.0 ], [ 0.0, 1.0, 2.0, 3.0 ]";
		boolean[] booleanarray = {false, true, false, true};
		char[] chararray = {'a','b','c','d'};
		byte[] bytearray = {0,1,2,3};
		short[] shortarray = {0,1,2,3};
		int[] intarray = {0,1,2,3};
		long[] longarray = {0,1,2,3};
		float[] floatarray = {0,1,2,3};
		double[] doublearray = {0,1,2,3};
		StringBuilder sb = new StringBuilder();
		StringUtil.appendObject(sb, booleanarray );
		sb.append(", ");
		StringUtil.appendObject(sb, chararray );
		sb.append(", ");
		StringUtil.appendObject(sb, bytearray );
		sb.append(", ");
		StringUtil.appendObject(sb, shortarray );
		sb.append(", ");
		StringUtil.appendObject(sb, intarray );
		sb.append(", ");
		StringUtil.appendObject(sb, longarray );
		sb.append(", ");
		StringUtil.appendObject(sb, floatarray );
		sb.append(", ");
		StringUtil.appendObject(sb, doublearray );
		assertEquals(sb.toString(), EXPECTED);
		Log.f(sb.toString());
		Log.fo();
	}
}
