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
package org.eclipse.titan.executorapi.util;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;

/**
 * Logger utility class.
 * <p>
 * Example usage:
 * <pre>
 * class A {
 * 
 *   void f1() {
 *     Log.fi();
 *     f2("abc");
 *     Log.fo();
 *   }
 *   
 *   void f2( String s ) {
 *     Log.fi(s);
 *     Log.f("Call doSomething()");
 *     doSomeThing();
 *     Log.fo();
 *   }
 *   
 *   ...
 * }
 * </pre>
 * 
 * <p>
 * Output:
 * <pre>
 * 2014-04-24 15:36:51.203   1 -> A.f1()
 * 2014-04-24 15:36:51.203   1 ->   A.f2( "abc" )
 * 2014-04-24 15:36:51.203   1        Call doSomething()
 * 2014-04-24 15:36:51.203   1 <-   A.f2()
 * 2014-04-24 15:36:51.204   1 <- A.f1()
 * </pre>
 */
public class Log {
	
	/**
	 * Severity of the log message, log level
	 * @see "http://logging.apache.org/log4j/1.2/apidocs/org/apache/log4j/Level.html"
	 */
	private enum Severity {
		/** The OFF has the highest possible rank and is intended to turn off logging. */
		OFF,
		
		/** The FATAL level designates very severe error events that will presumably lead the application to abort. */
		FATAL,
		
		/** The ERROR level designates error events that might still allow the application to continue running. */
		ERROR,
		
		/** The WARN level designates potentially harmful situations. */
		WARN,
		
		/** The INFO level designates informational messages that highlight the progress of the application at coarse-grained level. */
		INFO,
		
		/** The DEBUG Level designates fine-grained informational events that are most useful to debug an application. */
		DEBUG,
		
		/** The TRACE Level designates finer-grained informational events than the DEBUG */
		TRACE,
		
		/** The ALL has the lowest possible rank and is intended to turn on all logging. */
		ALL;
	};
	
	/**
	 * Global log level.
	 * It effects all the projects that use Log.
	 */
	private static Severity sLogLevel = Severity.OFF;
	
	/**
	 * Date format for function related logging functions: fi(), fo(), f()
	 */
	private static final SimpleDateFormat sFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS");
	
	/**
	 * Keeps track the call level for each thread.
	 * Call level is a number, that is increased, when a function entry point is logged (fi() is called),
	 *                                 decreased, when a function exit  point is logged (fo() is called).
	 * The indentation of the log message depends on the call level, higher call level means more indentation.
	 * Negative call level is treated as 0.
	 */
	private static Map<Long, Integer> sCallLevels = new HashMap<Long, Integer>();
	
	/**
	 * Logging states separately for each thread.
	 * Logging can be switched off separately for each thread. true: on, false: off. Default value: true.
	 */
	private static Map<Long, Boolean> sLoggingStates = new HashMap<Long, Boolean>();
	
	/**
	 * Function in.
	 * Logs the entry and the given parameter(s) of the method in DEBUG level.
	 * It must be placed at the entry point(s) of the method.
	 * @param aParams array of logged function parameters, it must be added one-by-one,
	 *                it doesn't be resolved automatically like class and method name
	 */
	public static void fi( final Object... aParams ) {
		if ( !checkLogLevel( Severity.DEBUG ) || isThreadLoggingOff() ) {
			return;
		}
		
		StringBuilder sb = new StringBuilder();
		logDateAndThreadId(sb);
		sb.append(" -> ");
		
		final int callLevel = getCallLevel();
		setCallLevel( callLevel + 1 );
		for (int i = 0; i < callLevel; i++) {
		   sb.append("  ");
		}
		
		// 0: getStackTrace(), 1: logMethodName(), 2: fi(), 3: this is the caller function we want to log
		logMethodName( sb, 3 );
		
		// log parameters
		sb.append("(");
		if ( aParams.length > 0 ) {
			sb.append(" ");
			for ( int i = 0; i < aParams.length; i++ ) {
				if ( i > 0 ) {
					sb.append(", ");
				}
				StringUtil.appendObject(sb, aParams[i]);
			}
			sb.append(" ");
		}
		sb.append(")");
		
		printLog( sb );
	}
	
	/**
	 * Function out.
	 * Logs the exit and the given return value of the method in DEBUG level.
	 * It must be placed at the exit point(s) of the method.
	 * @param aHasReturnValue true to log return value 
	 * @param aReturnValue the return value to log, it can be null if aHasReturnValue == false
	 */
	private static void fo( final boolean aHasReturnValue, final Object aReturnValue ) {
		if ( !checkLogLevel( Severity.DEBUG ) || isThreadLoggingOff() ) {
			return;
		}
		
		StringBuilder sb = new StringBuilder();
		logDateAndThreadId(sb);
		sb.append( " <- " );
		
		int callLevel = getCallLevel();
		setCallLevel( --callLevel );
		for (int i = 0; i < callLevel; i++) {
			sb.append("  ");
		}
		
		// 0: getStackTrace(), 1: logMethodName(), 2: fo (this private), 3: fo (one of the public ones), 4: this is caller the function we want to log
		logMethodName( sb, 4 );
		
		// log return value (if any)
		sb.append("()");
		if ( aHasReturnValue ) {
			sb.append(" ");
			StringUtil.appendObject(sb, aReturnValue);
		}
		printLog( sb );
	}
	
	/**
	 * Function out.
	 * Logs the exit of the method (but not the return value) in DEBUG level.
	 * It must be placed at the exit point(s) of the method.
	 */
	public static void fo() {
		fo( false, null );
	}
	
	/**
	 * Function out.
	 * Logs the exit and the given return value of the method in DEBUG level.
	 * It must be placed at the exit point(s) of the method.
	 * @param aReturnValue return value to log
	 */
	public static void fo( final Object aReturnValue ) {
		fo( true, aReturnValue );
	}
	
	/**
	 * function log (just a general log within the function)
	 * @param aMsg log message
	 */
	public static void f( final String aMsg ) {
		if ( !checkLogLevel( Severity.TRACE ) || isThreadLoggingOff() ) {
			return;
		}
		
		StringBuilder sb = new StringBuilder();
		logDateAndThreadId(sb);
		sb.append( "    " );
		
		final int callLevel = getCallLevel();
		for (int i = 0; i < callLevel; i++) {
		   sb.append("  ");
		}
		
		if ( aMsg != null && aMsg.contains( "\n" ) ) {
			// in case of multiline message other lines are also indented with the same number of spaces
			final int len = sb.length();
			StringBuilder sbMsg = new StringBuilder( aMsg );
			//replacement instead of \n
			StringBuilder sbSpaces = new StringBuilder( len );
			sbSpaces.append("\n");
			for (int i = 0; i < len; i++) {
				   sbSpaces.append(" ");
			}
			
			// replace \n -> sbSpaces in sbMsg
			StringUtil.replaceString( sbMsg, "\n", sbSpaces.toString() );
			sb.append( sbMsg );
		}
		else {
			sb.append( aMsg );
		}
		
		printLog( sb );
	}

	/**
	 * info
	 * @param aMsg log message
	 */
	public static void i( final String aMsg ) {
		if ( !checkLogLevel( Severity.INFO ) || isThreadLoggingOff() ) {
			return;
		}
		StringBuilder sb = new StringBuilder();
		logDateAndThreadId(sb);
		sb.append( " " + aMsg );
		printLog( sb );
	}
	
	/**
	 * info unformatted (without datetime)
	 * @param aMsg log message
	 */
	public static void iu( final String aMsg ) {
		if ( !checkLogLevel( Severity.INFO ) || isThreadLoggingOff() ) {
			return;
		}
		StringBuilder sb = new StringBuilder( aMsg );
		printLog( sb );
	}
	
	/**
	 * Switch on logging for the current thread
	 */
	public static void on() {
		final long threadId = Thread.currentThread().getId();
		sLoggingStates.put(threadId, true);
	}

	/**
	 * Switch off logging for the current thread
	 */
	public static void off() {
		final long threadId = Thread.currentThread().getId();
		sLoggingStates.put(threadId, false);
	}
	
	/**
	 * Adds full datetime and thread id to the log string. 
	 * @param aSb [in/out] the log string, where new strings are added
	 */
	private static void logDateAndThreadId( final StringBuilder aSb ) {
		final long threadId = Thread.currentThread().getId();
		aSb.append( sFormat.format( new Date() ) + " " + String.format( "%3d", threadId ) );
	}
	
	/**
	 * Checks if the global static log level reaches the minimum required log level,
	 * @param aMinRequired minimum required log level
	 * @return true, if the global log level >= required log level 
	 */
	private static boolean checkLogLevel( final Severity aMinRequired ) {
		return sLogLevel.ordinal() >= aMinRequired.ordinal();
	}
	
	/**
	 * Adds class and method name to the log string.
	 * Short class name is used without full qualification.
	 * @param aSb [in/out] the log string, where new strings are added
	 * @param aStackTraceElementIndex [in] the call stack index, where we get the class and method name.
	 *                                + 1 must be added, because logMethodName() increases the call stack size by 1
	 */
	private static void logMethodName( final StringBuilder aSb, final int aStackTraceElementIndex ) {
		final StackTraceElement ste = Thread.currentThread().getStackTrace()[ aStackTraceElementIndex ];
		final String className = ste.getClassName();
		final String shortClassName = className.substring( className.lastIndexOf('.') + 1 );
		final String methodName = ste.getMethodName();
		aSb.append( shortClassName + "." + methodName );
	}
	
	/**
	 * @return true if logging for the current thread switched off.
	 *              If there is no info for this thread, a new (<current thread>, true) item is added to the map 
	 */
	private static boolean isThreadLoggingOff() {
		final long threadId = Thread.currentThread().getId();
		
		if(!sLoggingStates.containsKey(threadId)) {
			sLoggingStates.put(threadId, true);
		}
		
		return !sLoggingStates.get( threadId );
	}
	
	/**
	 * @return Call level for the current thread.
	 *         If there is no info for this thread, a new (<current thread>, 0) item is added to the map 
	 */
	private static int getCallLevel() {
		final long threadId = Thread.currentThread().getId();
		if(!sCallLevels.containsKey( threadId ) ) {
			sCallLevels.put( threadId, 0 );
		}
		final int callLevel = sCallLevels.get( threadId );
		return callLevel;
	}
	
	/**
	 * Sets the call level of the current thread. 
	 * If sCallLevels does NOT contain current thread as key, it creates it.
	 * @param aNewValue new value of the logging level for the thread, in can be negative
	 */
	private static void setCallLevel( final int aNewValue ) {
		final long threadId = Thread.currentThread().getId();
		sCallLevels.put( threadId, aNewValue );
	}
	
	/**
	 * prints the log string to the output
	 * @param aLogString the log string, it can be multiline
	 */
	private static void printLog( final StringBuilder aLogString ) {
		System.out.println( aLogString );
	}
}
