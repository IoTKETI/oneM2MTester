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

import java.text.SimpleDateFormat;
import java.util.Date;

/**
 * Data structure for representing a time value.
 * <p>
 * The original C++ structure can be found at TTCNv3\mctr2\mctr\MainController.h
 */
public final class Timeval {
	
	/**
	 * Date format to display time with milliseconds precision, as java Date supports only milliseconds.
	 */
	private static final SimpleDateFormat sFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS");
	
	/**
	 * seconds since 1970-01-01 00:00:00.000
	 */
	public long tv_sec;
	
	/**
	 * microSeconds, 0 <= tv_usec < 1 000 000
	 */
	public long tv_usec;

	public Timeval(final long tv_sec, final long tv_usec) {
		this.tv_sec = tv_sec;
		this.tv_usec = tv_usec;
	}

	/**
	 * From milliseconds.
	 * <p>
	 * NOTE: last 3 digits of tv_usec are always 0 as Timeval is more precise (microseconds) 
	 * @param aMillis milliseconds since 1970-01-01 00:00:00.000
	 */
	public Timeval(final long aMillis) {
        tv_sec = aMillis / 1000;
        tv_usec = (aMillis - (tv_sec * 1000)) * 1000;
	}
	
	/**
	 * Create timestamp from current time
	 */
	public Timeval() {
		// current time in millis
		this(System.currentTimeMillis());
	}
	
	/**
	 * Converts to milliseconds
	 * <p>
	 * NOTE: it loses precision, microseconds -> milliseconds
	 * @return milliseconds since 1970-01-01 00:00:00.000
	 */
	public long toMillis() {
		return tv_sec * 1000 + tv_usec / 1000;
	}
	
	/**
	 * Converts to java Date
	 * <p>
	 * NOTE: it loses precision, microseconds -> milliseconds
	 * @return corresponding java Date object with milliseconds precision 
	 */
	public Date toDate() {
		return new Date(toMillis());
	}
	
	public String toString() {
		// use the last 3 digits of microseconds, not to lose precision
		return sFormat.format( toDate() ) + String.format("%03d", (tv_usec % 1000));
	}
}
