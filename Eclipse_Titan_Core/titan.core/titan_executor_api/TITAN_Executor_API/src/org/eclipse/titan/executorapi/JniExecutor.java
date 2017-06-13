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
package org.eclipse.titan.executorapi;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;
import java.util.HashMap;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.eclipse.titan.executor.jni.ComponentStruct;
import org.eclipse.titan.executor.jni.HostStruct;
import org.eclipse.titan.executor.jni.IJNICallback;
import org.eclipse.titan.executor.jni.JNIMiddleWare;
import org.eclipse.titan.executor.jni.McStateEnum;
import org.eclipse.titan.executor.jni.Timeval;
import org.eclipse.titan.executor.jni.VerdictTypeEnum;
import org.eclipse.titan.executorapi.exception.JniExecutorException;
import org.eclipse.titan.executorapi.exception.JniExecutorIllegalArgumentException;
import org.eclipse.titan.executorapi.exception.JniExecutorJniLoadException;
import org.eclipse.titan.executorapi.exception.JniExecutorStartSessionException;
import org.eclipse.titan.executorapi.exception.JniExecutorWrongStateException;
import org.eclipse.titan.executorapi.util.Log;


/**
 * SINGLETON
 * <p>
 * This executor handles the execution of tests compiled in a parallel mode, via directly connecting to the MainController (MC) written in C++.
 * It controls MC with commands, most of methods in this class represents one or more MC commands.
 * These methods can be called in certain states.
 * They can be synchronous or asynchronous.
 * <p>
 * Recommended order of method calls during a normal test execution:
 * <ol>
 *   <li> {@link #init()}
 *   <li> {@link #addHostController(HostController)}
 *   <li> {@link #setConfigFileName(String)}
 *   <li> {@link #setObserver(IJniExecutorObserver)}
 *   <li> {@link #startSession()}
 *   <li> {@link #startHostControllers()} (asynchronous)
 *   <li> {@link #configure()} (asynchronous)
 *   <li> {@link #createMTC()} (asynchronous)
 *   <li> One or more of the following methods:
 *     <ul>
 *       <li> {@link #executeTestcase(String, String)} (asynchronous)
 *       <li> {@link #executeControl(String)} (asynchronous)
 *       <li> {@link #getExecuteCfgLen()} and then {@link #executeCfg(int)} (asynchronous)
 *     </ul>
 *   <li> {@link #exitMTC()} (asynchronous)
 *   <li> {@link #shutdownSession()} (asynchronous)
 * </ol> 
 * <p>
 * This is singleton, because there is only one MainController instance.
 * <p>
 * When an asynchronous operation is finished, status changed (intermediate state, but operation is still running) or error happens or notification arrives from MC,
 * callback methods of {@link IJniExecutorObserver} will be called.
 * This observer is set by calling {@link #setObserver(IJniExecutorObserver)}.
 * <p>
 * If problem occurs synchronously (in synchronous functions or in asynchronous functions before request is sent to MC),
 * a {@link JniExecutorException} is thrown.
 * <br>
 * These problems can be one of the following cases:
 * <ul>
 *   <li> method is called in invalid state
 *   <li> one of the method parameters are invalid
 * </ul>
 * <p>
 * Otherwise (problem occurs during asynchronous operations in MC) {@link IJniExecutorObserver#error(int, String)} is called.
 * <p>
 * Simplified state diagram:
 * <p>
 * <img src="../../../../../doc/uml/TITAN_Executor_API_state_diagram_simple.png"/>
 */
public class JniExecutor implements IJNICallback {
	
	// Exception texts
	
	/** Used by checkConnection() */
	private static final String EXCEPTION_TEXT_WRONG_STATE_CONNECTED = "Executor is already initialized.";
	/** Used by checkConnection() */
	private static final String EXCEPTION_TEXT_WRONG_STATE_NOT_CONNECTED = "Executor is NOT initialized, call init() first.";
	/** Used by buildWrongStateMessage() */
	private static final String EXCEPTION_TEXT_WRONG_STATE_PART_1 = "Method cannot be called in this state. Current state: ";
	/** Used by buildWrongStateMessage() */
	private static final String EXCEPTION_TEXT_WRONG_STATE_PART_2 = ", expected state(s): ";
	
	/** Used by setConfigFileName() */
	private static final String EXCEPTION_TEXT_ILLEGAL_ARG_CFG_FILENAME_NULL = "Configuration file name is null.";
	/** Used by setConfigFileName() */
	private static final String EXCEPTION_TEXT_ILLEGAL_ARG_CFG_FILE_NOT_EXIST = "Configuration file does NOT exists.";
	/** Used by addHostController() */
	private static final String EXCEPTION_TEXT_ILLEGAL_ARG_ADD_HC_NULL = "Host Controller is null.";
	/** Used by executeControl() and executeTestcase() */
	private static final String EXCEPTION_TEXT_ILLEGAL_ARG_TEST_CONTROL_NAME_NULL_OR_EMPTY = "Test control name is null or empty.";
	/** Used by executeTestcase() */
	private static final String EXCEPTION_TEXT_ILLEGAL_ARG_TEST_CASE_NAME_NULL_OR_EMPTY = "Test case name is null or empty.";
	/** Used by executeCfg() */
	private static final String EXCEPTION_TEXT_ILLEGAL_ARG_TEST_CFG_INDEX_OUT_OF_BOUND = "Test index is out of bound.";
	
	/** Used by startSession() */
	private static final String EXCEPTION_TEXT_START_SESSION = "Start session failed. Error code: ";
	
	// Error texts

	/** Used by runCommand() */
	private static final String ERROR_TEXT_RUN_COMMAND_EXIT_CODE_NOT_0_PART_1 = "Command: \"";
	/** Used by runCommand() */
	private static final String ERROR_TEXT_RUN_COMMAND_EXIT_CODE_NOT_0_PART_2 = "\" exited with the following exit code: ";
	/** Used by runCommand() */
	private static final String ERROR_TEXT_RUN_COMMAND_FAILED = "Error running command: ";
	/** Used by runCommand() */
	private static final String ERROR_TEXT_RUN_COMMAND_INTERRUPTED = "The following command is interrupted: ";

	
	/**
	 * Default config string for JNIMiddleWare.configure(). This is used, if {@link #setConfigFileName(String)} is not called.
	 */
	private static final String DEFAULT_CONFIG_STRING = "//This part was added by the TITAN Executor API.\n" +
			                                            "[LOGGING]\n" +
			                                            "LogFile := \"./../log//%e.%h-part%i-%r.%s\"\n";
	
	/**
	 * Default TCP listen port. TCP listen port is needed as an input parameter to start a new session. 
	 */
	private static final int DEFAULT_TCP_LISTEN_PORT = 0;

	/**
	 * Default local host address.
	 * NULL is translated to 0.0.0.0 when it is sent to MainController
	 */
	private static final String DEFAULT_LOCAL_HOST_ADDRESS = "NULL";
	
	/**
	 * JNI middleware instance. OWNED
	 */
	private final JNIMiddleWare mJniMw;
	
	/**
	 * API observer for notifications and callbacks. NOT OWNED
	 */
	private IJniExecutorObserver mObserver = null;

	/**
	 * TCP port which is returned by start_session(), comes from MainController.
	 * It is needed as an input parameter to start a new HC.
	 * -1 means uninitialized/invalid value
	 */
	private int mMcPort = -1;
	
	/**
	 * Local host address. It is needed as an input parameter to start a new HC.
	 * Default value: NULL, which is translated to 0.0.0.0 when it is sent to MainController
	 */
	private String mMcHost = DEFAULT_LOCAL_HOST_ADDRESS;
	
	/**
	 * List of host controllers, which are started and connected with MainController by {@link #startHostControllers()}. OWNED
	 * @see #startHostControllers()
	 */
	private List<HostController> mHostControllers = null;
	
	/**
	 * true, if {@link #setConfigFileName(String)} is called, so config file is pre-processed by MainController,
	 *       and the result config data is stored in MC.
	 *       In this case {@link #startSession()} and {@link #configure()} will use the config data stored in MC.
	 * <p>
	 * false otherwise.
	 *       In this case {@link #startSession()} and {@link #configure()} will use the default values which are defined
	 *       in this class (private static final ... DEFAULT_*)
	 */
	private boolean mCfgFilePreprocessed = false;
	
	/**
	 * true, if {@link #shutdownSession()} is called, false otherwise.
	 * <p>
	 * The reason that we need this flag is that {@link #shutdownSession()} can be called in many states,
	 * so through more asynchronous requests when a request is finished we need to call another,
	 * and we need to remember that during the whole process.
	 */
	private boolean mShutdownRequested = false;
	
	/**
	 * Lock object for waitForCompletion()
	 */
	private final Object mLockCompletion = new Object();
	
	/**
	 * Pattern for verdict, that comes as a notification after execution of a testcase.
	 * If a test control is executed, this notification is sent multiple times after each testcase.
	 * <p>
	 * Example:
	 * <br>
	 * Test case HelloW finished. Verdict: pass
	 * <br>
	 * Test case HelloW2 finished. Verdict: inconc
	 */
	private static final Pattern PATTERN_VERDICT =
			Pattern.compile("Test case (.*) finished\\. Verdict: (none|pass|inconc|fail|error)");
    
	// Group indexes for this pattern
	// It is stored here, because group indexes must be synchronized with the pattern.
	private static final int PATTERN_VERDICT_GROUP_INDEX_TESTCASE = 1;
	private static final int PATTERN_VERDICT_GROUP_INDEX_VERDICTTYPENAME = 2;
	
	/**
	 * Pattern for dynamic testcase error, that comes as a notification after unsuccessful execution of a testcase.
	 * If a test control is executed, this notification is sent multiple times after each testcase.
	 * <p>
	 * Example:
	 * <br>
	 * Dynamic test case error: Test case tc_HelloW does not exist in module MyExample.
	 */
	private static final Pattern PATTERN_DYNAMIC_TESTCASE_ERROR =
			Pattern.compile("Dynamic test case error: (.*)");
    
	// Group indexes for this pattern
	// It is stored here, because group indexes must be synchronized with the pattern.
	private static final int PATTERN_DYNAMIC_TESTCASE_ERROR_GROUP_INDEX_ERROR_TEXT = 1;
	
	
	/**
	 * Pattern for verdict statistics, that comes as a notification after executing all the testcases after exit MTC.
	 * This notification is sent only once for a MTC session.
	 * <p>
	 * Example:
	 * <br>
	 * Verdict statistics: 0 none (0.00 %), 1 pass (50.00 %), 1 inconc (50.00 %), 0 fail (0.00 %), 0 error (0.00 %).
	 * <br>
	 * Verdict statistics: 0 none, 0 pass, 0 inconc, 0 fail, 0 error.
	 */
	private static final Pattern PATTERN_VERDICT_STATS =
			Pattern.compile("Verdict statistics: "
					+ "(\\d+) none[^,]*, "
					+ "(\\d+) pass[^,]*, "
					+ "(\\d+) inconc[^,]*, "
					+ "(\\d+) fail[^,]*, "
					+ "(\\d+) error.*");
    
	// Group indexes for this pattern
	// It is stored here, because group indexes must be synchronized with the pattern.
	private static final int PATTERN_VERDICT_STATS_GROUP_INDEX_NONE = 1;
	private static final int PATTERN_VERDICT_STATS_GROUP_INDEX_PASS = 2;
	private static final int PATTERN_VERDICT_STATS_GROUP_INDEX_INCONC = 3;
	private static final int PATTERN_VERDICT_STATS_GROUP_INDEX_FAIL = 4;
	private static final int PATTERN_VERDICT_STATS_GROUP_INDEX_ERROR = 5;
	
	/**
	 * Pattern for error outside of test cases, that comes as a notification after executing all the testcases after exit MTC.
	 * This notification is sent only once for a MTC session after PATTERN_VERDICT_STATS.
	 * <p>
	 * OPTIONAL
	 * <p>
	 * Example:
	 * <br>
	 * Number of errors outside test cases: 2
	 */
	private static final Pattern PATTERN_ERRORS_OUTSIDE_OF_TESTCASES =
			Pattern.compile("Number of errors outside test cases: (\\d+)");
	
	// Group indexes for this pattern
	// It is stored here, because group indexes must be synchronized with the pattern.
	private static final int PATTERN_ERRORS_OUTSIDE_OF_TESTCASES_GROUP_INDEX_ERRORS = 1;
	
	/**
	 * Pattern for overall verdict, that comes as a notification after executing all the testcases after exit MTC.
	 * This notification is sent only once for a MTC session after PATTERN_VERDICT_STATS and PATTERN_ERRORS_OUTSIDE_OF_TESTCASES (optional).
	 * <p>
	 * Example:
	 * <br>
	 * Test execution summary: 0 test case was executed. Overall verdict: error
	 */
	private static final Pattern PATTERN_OVERALL_VERDICT =
			Pattern.compile("Test case (.*) finished\\. Verdict: (none|pass|inconc|fail|error)");
    
	// Group indexes for this pattern
	// It is stored here, because group indexes must be synchronized with the pattern.
	private static final int PATTERN_OVERALL_VERDICT_GROUP_INDEX_NUMBER_OF_TESTCASE = 1;
	private static final int PATTERN_OVERALL_VERDICT_GROUP_INDEX_VERDICTTYPENAME = 2;
	
	/**
	 * Private constructor, because it is a singleton.
	 */
	private JniExecutor() {
		mJniMw = new JNIMiddleWare(this);
	}
	
	/**
	 * Lazy holder for the singleton (Bill Pugh solution)
	 * Until we need an instance, the holder class will not be initialized until required and you can still use other static members of the singleton class.
	 * @see <a href="http://en.wikipedia.org/wiki/Singleton_pattern#Initialization_On_Demand_Holder_Idiom">Wikipedia</a>
	 */
	private static class SingletonHolder {
		/** Singleton instance */
		private static final JniExecutor mInstance = new JniExecutor();
	}
	
	/**
	 * @return the singleton instance
	 */
	public static JniExecutor getInstance() {
		return SingletonHolder.mInstance;
	}

	@Override
	public void statusChangeCallback( final McStateEnum aState ) {
		Log.fi( aState );
		if ( mShutdownRequested ) {
			continueShutdown( aState );
		}
		
		observerStatusChanged( aState );
		Log.fo();
	}

	@Override
	public void errorCallback( final int aSeverity, final String aMsg ) {
		Log.fi( aSeverity, aMsg );
		observerError( aSeverity, aMsg );
		Log.fo();
	}

	@Override
	public void batchedInsertNotify( final List<String[]> aNotifications ) {
		Log.fi( aNotifications );
		for (String[] n : aNotifications) {
			Timeval timestamp;
			try {
				timestamp = new Timeval(Integer.parseInt(n[0]), Integer.parseInt(n[1]));
			} catch (Exception e) {
				timestamp = new Timeval();
			}
			
			final String source = n[2];
			int severity;
			try {
				severity = Integer.parseInt(n[3]);
			} catch (Exception e) {
				severity = 0;
			}
			
			final String message = n[4];
			notifyCallback(timestamp, source, severity, message );
		}
		Log.fo();
	}

	@Override
	public void notifyCallback( final Timeval aTime, final String aSource, final int aSeverity, final String aMsg ) {
		Log.fi( aTime, aSource, aSeverity, aMsg );
		if ( !processSpecialNotifications( aMsg ) ) {
			observerNotify( aTime, aSource, aSeverity, aMsg );
		}
		Log.fo();
	}

	/**
	 * Gets lock object for synchronizing MC related operations.
	 * @return lock object
	 */
	public final Object getLock() {
		return mJniMw.getLock();
	}
	
	/**
	 * Gets connection state. SYNCHRONOUS
	 * @return true, if connected (connection to MC is initialized (after init() is called) and not yet terminated (before asynchronous shutdownSession() is completed))
	 *         <br>
	 *         false otherwise (disconnected: not initialized or terminated)
	 */
	public boolean isConnected() {
		synchronized (getLock()) {
			return mJniMw.isConnected();
		}
	}

	/**
	 * Connects client to MainController. SYNCHRONOUS
	 * <p>
	 * It can be called in disconnected state,
	 * it moves MC to MC_INACTIVE state.
	 * <p>
	 * Do NOT call {@link #setObserver(IJniExecutorObserver)} or {@link #addHostController(HostController)} before {@link #init()}, because {@link #init()} will delete them.
	 * @throws JniExecutorWrongStateException 
	 * @throws JniExecutorJniLoadException 
	 */
	public void init() throws JniExecutorWrongStateException, JniExecutorJniLoadException {
		synchronized (getLock()) {
			// make sure, that we are in uninitialized state, otherwise calling reset() is NOT allowed
			checkConnection( false );
			JniExecutorJniLoadException e = JNIMiddleWare.getException();
			if ( e != null ) {
				throw e;
			}
			reset();
			mJniMw.initialize(1500);
		}
	}

	/**
	 * Adds a new Host Controller. SYNCHRONOUS
	 * <p>
	 * It doesn't communicate with MainController, just stores the HCs,
	 * {@link #startHostControllers()} will start the HCs according to this information
	 * <p>
	 * It can be called in MC_INACTIVE, MC_LISTENING or MC_LISTENING_CONFIGURED state, the operation will not change the state.
	 * @param aHc the new HC
	 * @throws JniExecutorIllegalArgumentException 
	 * @throws JniExecutorWrongStateException 
	 * @see #startHostControllers()
	 */
	public void addHostController( final HostController aHc ) throws JniExecutorIllegalArgumentException, JniExecutorWrongStateException {
		synchronized (getLock()) {
			checkState( McStateEnum.MC_INACTIVE, McStateEnum.MC_LISTENING, McStateEnum.MC_LISTENING_CONFIGURED );
			if ( aHc == null ) {
				throw new JniExecutorIllegalArgumentException( EXCEPTION_TEXT_ILLEGAL_ARG_ADD_HC_NULL );
			}

			if ( mHostControllers == null ) {
				mHostControllers = new ArrayList<HostController>();
			}
			mHostControllers.add( aHc );
		}
	}
	
	/**
         * @return added host controllers. It can be null
         */
	public List<HostController> getHostControllers() {
		return mHostControllers;
	}

	/**
	 * Sets the configuration file of the test. SYNCHRONOUS
	 * <p>
	 * If setConfigFileName() is called before {@link #startSession()} and {@link #configure()} (HIGHLY RECOMMENDED),
	 * it extracts the config string from the config file, and the result config string is stored in MainController,
	 * which will be used by configure() and no config data needed to be passed to MC.
	 * <p>
	 * If setConfigFileName() is NOT called before {@link #startSession()} and {@link #configure()},
	 * a default config string will be passed to MainController by {@link #configure()},
	 * a default TCP listen port and default MC host address (localhost) will be used in {@link #startSession()},
	 * and 1 HC will be started by {@link #startHostControllers()}.
	 * <p>
	 * It can be called in MC_INACTIVE state, the operation will not change the state. 
	 * 
	 * @param aConfigFileName the configuration file of the test with full path
	 * @throws JniExecutorWrongStateException 
	 * @throws JniExecutorIllegalArgumentException 
	 * @see #configure()
	 */
	public void setConfigFileName( final String aConfigFileName ) throws JniExecutorWrongStateException, JniExecutorIllegalArgumentException {
		Log.fi();
		synchronized (getLock()) {
			checkState( McStateEnum.MC_INACTIVE );
			if ( aConfigFileName == null ) {
				throw new JniExecutorIllegalArgumentException( EXCEPTION_TEXT_ILLEGAL_ARG_CFG_FILENAME_NULL );
			}
			// config file cannot be a remote file only local, see config_preproc_p.tab.cc preproc_parse_file()
			final File cfgFile = new File(aConfigFileName);
			if ( !cfgFile.exists() || !cfgFile.isFile() ) {
				throw new JniExecutorIllegalArgumentException( EXCEPTION_TEXT_ILLEGAL_ARG_CFG_FILE_NOT_EXIST );
			}
			mJniMw.do_set_cfg_file(aConfigFileName);
			mCfgFilePreprocessed = true;
		}
		Log.fo();
	}
	
	/**
	 * Sets the API observer for notifications and callbacks. SYNCHRONOUS
	 * <p>
	 * It can be called in connected state, the operation will not change the state.
	 * @param aObserver the observer, it can be null, but not recommended.
	 * @throws JniExecutorWrongStateException 
	 */
	public void setObserver( final IJniExecutorObserver aObserver ) throws JniExecutorWrongStateException {
		checkConnection(true);
		mObserver = aObserver;
	}
	
	/**
	 * Starts MC session. SYNCHRONOUS
	 * <p>
	 * It can be called in MC_INACTIVE state.
	 * <br>
	 * MC goes to MC_LISTENING state when operation is finished.
	 * <p>
	 * NOTE: The operation is synchronous, but {@link IJniExecutorObserver#statusChanged(McStateEnum)} with parameter MC_LISTENING is also called.
	 * @throws JniExecutorWrongStateException 
	 * @throws JniExecutorStartSessionException 
	 */
	public void startSession() throws JniExecutorWrongStateException, JniExecutorStartSessionException {
		Log.fi();
		synchronized (getLock()) {
			checkState( McStateEnum.MC_INACTIVE );
			
			// DEFAULT_LOCAL_HOST_ADDRESS is used by default if config file is not provided ( setConfigFileName() is not called )
			String localAddress = ( mCfgFilePreprocessed ? mJniMw.do_get_mc_host() : DEFAULT_LOCAL_HOST_ADDRESS );
			
			// TCP listening port
			// DEFAULT_TCP_LISTEN_PORT is used by default if config file is not provided ( setConfigFileName() is not called )
			int tcpport = ( mCfgFilePreprocessed ? mJniMw.do_get_port() : DEFAULT_TCP_LISTEN_PORT );
			
			mMcHost = localAddress;
			
			// 3rd parameter should be true on Linux, false on Windows
			int port = mJniMw.do_start_session(localAddress, tcpport, !JNIMiddleWare.isWin() );
			if (port <= 0) {
				// there were some errors starting the session
				// error message is sent by MC, so errorCallBack() -> IJniExecutorObserver.error() is called.
				throw new JniExecutorStartSessionException( EXCEPTION_TEXT_START_SESSION + port );
			}
			else {
				mMcPort = port;
			}
		}
		Log.fo();
	}
	
	/**
	 * Start the Host Controllers. ASYNCHRONOUS
	 * <p>
	 * Host Controllers are started with a system command, each in a separate thread,
	 * because all the commands block its thread until shutdown_session() is called which moves MC to MC_INACTIVE state.
	 * <p> 
	 * It can be called in MC_LISTENING or MC_LISTENING_CONFIGURED state.
	 * <br>
	 * MC goes from MC_LISTENING to MC_HC_CONNECTED state when all the HCs are started.
	 * <br>
	 * MC goes from MC_LISTENING_CONFIGURED to MC_CONFIGURING state when all the HCs are started.
	 * @throws JniExecutorWrongStateException 
	 */
	public void startHostControllers() throws JniExecutorWrongStateException {
		Log.fi();
		synchronized (getLock()) {
			checkState( McStateEnum.MC_LISTENING, McStateEnum.MC_LISTENING_CONFIGURED );
			if ( mHostControllers != null ) {
				for ( final HostController hc : mHostControllers ) {
					startHostController( hc );
				}
			}
		}
		Log.fo();
	}
	
	/**
	 * Starts the given Host Controller. ASYNCHRONOUS
	 * <p>
	 * A host controller is started with a system command.
	 * @param aHc the HC to start
	 */
	private void startHostController( final HostController aHc ) {
		final String command = aHc.getCommand( mMcHost, mMcPort );

		/*
		 * The command runs in a separate thread, because this command will move
		 * the MainController to MC_HC_CONNECTED state and it blocks the thread until
		 * shutdown_session() is called which moves MC to MC_INACTIVE state.
		 * So runCommand() function ends right after shutdown_session() is called.
		 */
		new Thread() {
		    public void run() {
		    	runCommand( command );
		    }
		}.start();
	}
	
	/**
	 * Runs a system command. SYNCHRONOUS
	 * <p>
	 * Tested on Linux, NOT tested on CygWin
	 * @param aCommand The system command. sh -c is added at the beginning of the command.
	 */
	private void runCommand( final String aCommand ) {
	    Log.fi(aCommand);
		Runtime run = Runtime.getRuntime();  
		Process p = null;  
		try {
			String[] cmd = { "sh", "-c", aCommand };
		    p = run.exec(cmd);

		    p.getErrorStream();
            BufferedReader br = new BufferedReader(new InputStreamReader(p.getInputStream()));
            String s;
            while ((s = br.readLine()) != null) {
            	Log.f("line: " + s);
                //send output to observer with notify()
            	notifyCallback( new Timeval(), mMcHost, 0, s);
            }
            p.waitFor();
            final int exitValue = p.exitValue();
            Log.f("exit: " + exitValue);
            if ( exitValue != 0 ) {
    			errorCallback(0, ERROR_TEXT_RUN_COMMAND_EXIT_CODE_NOT_0_PART_1 + aCommand +
    					         ERROR_TEXT_RUN_COMMAND_EXIT_CODE_NOT_0_PART_2 + exitValue);
            }
		}
		catch (IOException e) {
			Log.f(e.toString());
			errorCallback(0, ERROR_TEXT_RUN_COMMAND_FAILED + aCommand);
		} catch (InterruptedException e) {
			Log.f(e.toString());
			errorCallback(0, ERROR_TEXT_RUN_COMMAND_INTERRUPTED + aCommand);
		}
		finally {
		    p.destroy();
		}
		Log.fo();
	}
	
	/**
	 * Set parameters of the execution, which was pre-processed by setConfigFileName(). ASYNCHRONOUS
	 * <p>
	 * If setConfigFileName() is called before startSession() and configure() (HIGHLY RECOMMENDED),
	 * it extracts the config string from the config file, and the result config string is stored in MainController,
	 * which will be used by configure() and no config data needed to be passed to MC.
	 * <p>
	 * If setConfigFileName() is NOT called before startSession() and configure(),
	 * a default config string will be passed to MainController by configure(),
	 * a default TCP listen port and default MC host address (localhost) will be used in startSession(),
	 * and 1 HC will be started by startHostControllers().
	 * <p>
	 * It can be called in MC_LISTENING, MC_LISTENING_CONFIGURED or MC_HC_CONNECTED state.
	 * <br>
	 * MC goes from MC_LISTENING or MC_LISTENING_CONFIGURED to MC_LISTENING_CONFIGURED when operation is finished
	 *   (but config string is NOT yet downloaded to the HCs, because HCs are not started yet).
	 * <br>
	 * MC goes from MC_HC_CONNECTED to MC_CONFIGURING, and then to MC_ACTIVE when operation is finished (config string is downloaded to all HCs).
	 * @throws JniExecutorWrongStateException 
	 * @see JniExecutor#setConfigFileName(String)
	 */
	public void configure() throws JniExecutorWrongStateException {
		synchronized (getLock()) {
			checkState( McStateEnum.MC_HC_CONNECTED, McStateEnum.MC_LISTENING, McStateEnum.MC_LISTENING_CONFIGURED );
			
			// DEFAULT_CONFIG_STRING is used by default if config file is not provided ( setConfigFileName() is not called )
			// otherwise empty string is sent to MC, which means MC will used its stored config data
			mJniMw.do_configure( mCfgFilePreprocessed ? null : DEFAULT_CONFIG_STRING );
		}
	}
	
	/**
	 * Creates Main Test Component (MTC), which is the last step before we can execute the tests. ASYNCHRONOUS
	 * <p>
	 * It can be called in MC_ACTIVE state,
	 * it moves MC to MC_CREATING_MTC,
	 * and then to MC_READY state.
	 * @throws JniExecutorWrongStateException 
	 */
	public void createMTC() throws JniExecutorWrongStateException {
		synchronized (getLock()) {
			checkState( McStateEnum.MC_ACTIVE );
			mJniMw.do_create_mtc(0);
		}
	}
	
	/**
	 * Executes a test control by module name. ASYNCHRONOUS
	 * <p>
	 * It can be called in MC_READY state,
	 * it moves MC to MC_EXECUTING_CONTROL,
	 * and then to MC_READY state when test control execution is finished.
	 * @param aModule module name
	 * @throws JniExecutorWrongStateException if MC state != MC_READY
	 * @throws JniExecutorIllegalArgumentException if ( aModule == null || aModule.length() == 0 )
	 */
	public void executeControl( final String aModule ) throws JniExecutorWrongStateException, JniExecutorIllegalArgumentException {
		synchronized (getLock()) {
			checkState( McStateEnum.MC_READY );
			if ( aModule == null || aModule.length() == 0 ) {
				throw new JniExecutorIllegalArgumentException( EXCEPTION_TEXT_ILLEGAL_ARG_TEST_CONTROL_NAME_NULL_OR_EMPTY );
			}
			mJniMw.do_execute_control( aModule );
		}
	}

	/**
	 * Executes a testcase by name. ASYNCHRONOUS
	 * <p>
	 * It can be called in MC_READY state,
	 * it moves MC to MC_EXECUTING_TESTCASE,
	 * and then to MC_READY state when testcase execution is finished.
	 * @param aModule module name
	 * @param aTestcase testcase name
	 * @throws JniExecutorWrongStateException if MC state != MC_READY
	 * @throws JniExecutorIllegalArgumentException if aModule or aTestcase is null or empty
	 */
	public void executeTestcase( final String aModule, final String aTestcase ) throws JniExecutorWrongStateException, JniExecutorIllegalArgumentException {
		synchronized (getLock()) {
			checkState( McStateEnum.MC_READY );
			if ( aModule == null || aModule.length() == 0 ) {
				throw new JniExecutorIllegalArgumentException( EXCEPTION_TEXT_ILLEGAL_ARG_TEST_CONTROL_NAME_NULL_OR_EMPTY );
			}
			if ( aTestcase == null || aTestcase.length() == 0 ) {
				throw new JniExecutorIllegalArgumentException( EXCEPTION_TEXT_ILLEGAL_ARG_TEST_CASE_NAME_NULL_OR_EMPTY );
			}
			mJniMw.do_execute_testcase( aModule, aTestcase );
		}
	}
	
	/**
	 * Gets the length of the execute list. SYNCHRONOUS
	 * <p>
	 * Execute list is defined in the [EXECUTE] section in the configuration file.
	 * It must be called after setConfigFileName() to have valid data,
	 * otherwise it will return 0, because this value is not filled.
	 * <p>
	 * It can be called in MC_READY state,
	 * <p>
	 * @return The length of the execute list, or 0, if there is no [EXECUTE] section in the configuration file.
	 * @throws JniExecutorWrongStateException 
	 */
	public int getExecuteCfgLen() throws JniExecutorWrongStateException {
		synchronized (getLock()) {
			// for safety reasons, it could be read in any state, but in some cases data is uninitialized before calling set_cfg_file
			checkState( McStateEnum.MC_READY );
			return mJniMw.do_get_execute_cfg_len();
		}
	}
	
	/**
	 * Executes the index-th element of the execute list. ASYNCHRONOUS
	 * <p>
	 * Execute list is defined in the [EXECUTE] section in the configuration file.
	 * <p>
	 * It can be called in MC_READY state,
	 * it moves MC to MC_EXECUTING_TESTCASE or MC_EXECUTING_CONTROL,
	 * and then to MC_READY state when testcase or test control execution is finished.
	 * @param aIndex The test index from the execute list
	 * @throws JniExecutorWrongStateException 
	 * @throws JniExecutorIllegalArgumentException 
	 */
	public void executeCfg( final int aIndex ) throws JniExecutorWrongStateException, JniExecutorIllegalArgumentException {
		synchronized (getLock()) {
			checkState( McStateEnum.MC_READY );
			final int executeCfgLen = getExecuteCfgLen();
			if ( aIndex < 0 || aIndex >= executeCfgLen ) {
				throw new JniExecutorIllegalArgumentException( EXCEPTION_TEXT_ILLEGAL_ARG_TEST_CFG_INDEX_OUT_OF_BOUND );
			}
			mJniMw.do_execute_cfg( aIndex );
		}
	}
	
	/**
	 * Switches the "pause after testcase" flag on or off. By default it is off. SYNCHRONOUS
	 * <p>
	 * It makes sense only in case of test control.
	 * If it is on, the test execution is paused after every testcase,
	 * and state will be switched to MC_PAUSED.
	 * <p>
	 * It will switch to MC_PAUSED even after the last testcase.
	 * In this case continueExecution() will change the state to MC_EXECUTING_CONTROL and immediately to MC_READY if there is no more testcase to run.
	 * Otherwise continueExecution() will change the state to MC_EXECUTING_CONTROL, a the next testcase is executed.
	 * If we switch from on to off in MC_PAUSED state, execution of the next testcase will be automatically continued.
	 * <p>
	 * This function can be called in connected state.
	 * @param aNewState the new "pause after testcase" flag state. true: on, false: off
	 * @throws JniExecutorWrongStateException
	 */
	public void pauseExecution( final boolean aNewState ) throws JniExecutorWrongStateException {
		synchronized (getLock()) {
			checkConnection(true);
			mJniMw.do_stop_after_testcase( aNewState );
		}
	}
	
	/**
	 * Gets the "pause after testcase" flag. SYNCHRONOUS
	 * <p>
	 * This function can be called in connected state.
	 * @return true, if test execution will stop after a testcase, false otherwise
	 * @see #pauseExecution(boolean)
	 * @throws JniExecutorWrongStateException
	 */
	public boolean isPaused() throws JniExecutorWrongStateException {
		synchronized (getLock()) {
			checkConnection(true);
			return mJniMw.do_get_stop_after_testcase();
		}
	}
	
	/**
	 * Continues execution if test control is paused, it executes the next testcase from the control. ASYNCHRONOUS
	 * <p> 
	 * It can be called in MC_PAUSED state,
	 * it moves MC to MC_TERMINATING_TESTCASE, and then to MC_READY state.
	 * @throws JniExecutorWrongStateException 
	 */
	public void continueExecution() throws JniExecutorWrongStateException {
		synchronized (getLock()) {
			checkState( McStateEnum.MC_PAUSED );
			mJniMw.do_continue_testcase();
		}
	}
	
	/**
	 * Stops execution of a running or paused test. ASYNCHRONOUS
	 * <p>
	 * It can be called in MC_READY, MC_EXECUTING_TESTCASE, MC_EXECUTING_CONTROL  or MC_PAUSED state,
	 * it moves MC to MC_TERMINATING_TESTCASE, and then to MC_READY state.
	 * @throws JniExecutorWrongStateException 
	 */
	public void stopExecution() throws JniExecutorWrongStateException {
		synchronized (getLock()) {
			checkState( McStateEnum.MC_READY, McStateEnum.MC_EXECUTING_CONTROL, McStateEnum.MC_EXECUTING_TESTCASE, McStateEnum.MC_PAUSED );
			if ( McStateEnum.MC_READY != getState() ) {
				mJniMw.do_stop_execution();
			}
			// otherwise do nothing, MC would do the same, but there is no point to call it in this state
		}
	}
	
	/**
	 * Stops MTC. ASYNCHRONOUS
	 * <p>
	 * It can be called when no more tests to run.
	 * It can be called in MC_READY state,
	 * it moves MC to MC_TERMINATING_MTC state, and then to MC_ACTIVE state.
	 * @throws JniExecutorWrongStateException 
	 */
	public void exitMTC() throws JniExecutorWrongStateException {
		synchronized (getLock()) {
			checkState( McStateEnum.MC_READY );
			mJniMw.do_exit_mtc();
		}
	}
	
	/**
	 * Shuts down MC session. ASYNCHRONOUS
	 * <p>
	 * Shutdown can be called in any state, it goes through more states if needed.
	 * It goes to MC_INACTIVE state, and then to disconnected.
	 * <br>
	 * For example if a testcase is executed (MC_EXECUTING_CONTROL, MC_EXECUTING_TESTCASE or MC_PAUSED state),
	 * stop execution request is sent and we get to MC_TERMINATING_TESTCASE state, then to MC_READY state.
	 * Then exit mtc request is sent, and we get to MC_TERMINATING_MTC and then to MC_ACTIVE state.
	 * Then shutdown session request is sent, and we get to MC_SHUTDOWN, then to MC_INACTIVE state.
	 * Then synchronous terminate request is sent, which closes the connection.
	 * <p> 
	 * When it is called in MC_LISTENING, MC_LISTENING_CONFIGURED, MC_HC_CONNECTED or MC_ACTIVE state,
	 * it goes to MC_INACTIVE in one step via MC_SHUTDOWN.
	 */
	public void shutdownSession() {
		Log.fi();
		synchronized (getLock()) {
			if ( !mShutdownRequested && isConnected() ) {
				mShutdownRequested = true;
				final McStateEnum state = getState();
				continueShutdown( state );
			}
			// otherwise shutdown is already requested or not initialized, we don't need to do anything
		}
		Log.fo();
	}

	/**
	 * Continues next asynchronous (or final synchronous) step of shutdown.
	 * <p>
	 * Prerequisite: mShutdownRequested == true
	 * @param aState current MC state
	 */
	private void continueShutdown( final McStateEnum aState ) {
		Log.fi(aState);
		synchronized (getLock()) {
			switch ( aState ) {
			case MC_INACTIVE:
				mShutdownRequested = false;
				// session shutdown is finished (requested by jnimw.do_shutdown_session())
				mJniMw.terminate_internal();
				// NOTE: Do NOT call reset here, because it will set the observer to null,
				// and client will NOT be called back!
				mHostControllers = null;
				synchronized (mLockCompletion) {
					mLockCompletion.notifyAll();
				}
				break;
			case MC_LISTENING:
			case MC_LISTENING_CONFIGURED:
			case MC_HC_CONNECTED:
			case MC_ACTIVE:
				mJniMw.do_shutdown_session();
				// mJniMw.do_terminate_internal() must be also called when shutdown is finished, see statusChangeCallback() case MC_INACTIVE
				break;
			case MC_READY:
				mJniMw.do_exit_mtc();
				break;
			case MC_EXECUTING_CONTROL:
			case MC_EXECUTING_TESTCASE:
			case MC_PAUSED:
				mJniMw.do_stop_execution();
				break;

			default:
				// We are in some unstable/intermediate state, we don't have to do anything, because we will get to some stable state
				// NOTE: MC_EXECUTING_CONTROL and MC_EXECUTING_TESTCASE are also intermediate states, but they can be interrupted by calling do_stop_execution()
				break;
			}
		}
		Log.fo();
	}

	/**
	 * Gets number of hosts. SYNCHRONOUS
	 * <p>
	 * It must be called in McStateEnum.MC_HC_CONNECTED, McStateEnum.MC_ACTIVE or MC_READY state.
	 * @return number of hosts
	 * @throws JniExecutorWrongStateException 
	 */
	public int getNumberOfHosts() throws JniExecutorWrongStateException {
		synchronized (getLock()) {
			checkState( McStateEnum.MC_HC_CONNECTED, McStateEnum.MC_ACTIVE, McStateEnum.MC_READY );
			return mJniMw.do_get_nof_hosts();
		}
	}
	
	/**
	 * Gets host data. SYNCHRONOUS
	 * <p>
	 * It must be called in McStateEnum.MC_HC_CONNECTED, McStateEnum.MC_ACTIVE or MC_READY state.
	 * @param aIndex host index
	 * @return host data
	 * @throws JniExecutorWrongStateException 
	 */
	public HostStruct getHostData( final int aIndex ) throws JniExecutorWrongStateException {
		Log.fi();
		synchronized (getLock()) {
			checkState( McStateEnum.MC_HC_CONNECTED, McStateEnum.MC_ACTIVE, McStateEnum.MC_READY );
			HostStruct copy = null;
			try {
				final HostStruct host = mJniMw.do_get_host_data( aIndex );
				copy = (host != null ? new HostStruct(host) : null);
			} catch (Throwable t) {
				//do nothing copy is already null
				Log.f( t.toString() );
			} finally {
				mJniMw.do_release_data();
			}
			Log.fo( copy );
			return copy;
		}
	}
	
	/**
	 * Gets component data. SYNCHRONOUS
	 * <p>
	 * It must be called in MC_READY state
	 * @param aCompRef component reference, this info comes from HostData.components
	 * @return component data
	 * @throws JniExecutorWrongStateException 
	 */
	public ComponentStruct getComponentData( final int aCompRef ) throws JniExecutorWrongStateException {
		Log.fi();
		synchronized (getLock()) {
			checkState( McStateEnum.MC_READY );
			ComponentStruct copy = null;
			try {
				final ComponentStruct component = mJniMw.do_get_component_data( aCompRef );
				copy = (component != null ? new ComponentStruct(component) : null);
			} catch (Throwable t) {
				//do nothing copy is already null
				Log.f( t.toString() );
			} finally {
				mJniMw.do_release_data();
			}
			Log.fo( copy );
			return copy;
		}
	}
	
	/**
	 * Gets MC state from MainController. SYNCHRONOUS
	 * <p>
	 * It can be called in any state, but do not call it when there is something to read on the pipe.
	 * @return MC state
	 */
	public McStateEnum getState() {
		synchronized (getLock()) {
			return mJniMw.do_get_state();
		}
	}
	
	/**
	 * Checks if connected state.
	 * We are connected between init() and shutdownSession().
	 * So before init() we expect false, in any other case we expect true 
	 * @param aExpected expected connected state 
	 * @throws JniExecutorWrongStateException if current connected state is not the expected
	 */
	private void checkConnection( final boolean aExpected ) throws JniExecutorWrongStateException {
		final boolean connected = isConnected();
		if ( connected != aExpected ) {
			throw new JniExecutorWrongStateException( connected ? EXCEPTION_TEXT_WRONG_STATE_CONNECTED : EXCEPTION_TEXT_WRONG_STATE_NOT_CONNECTED );
		}
	}
	
	/**
	 * Checks if we are in any of the expected states
	 * @param aExpectedStates array of the expected states 
	 * @throws JniExecutorWrongStateException if current state is not one of the expected
	 */
	private void checkState( final McStateEnum... aExpectedStates ) throws JniExecutorWrongStateException {
		checkConnection(true);
		final McStateEnum currentState = getState();
		boolean found = false;
		for ( final McStateEnum state : aExpectedStates) {
			if ( currentState == state ) {
				found = true;
				break;
			}
		}
		if ( !found ) {
			handleWrongState(currentState, aExpectedStates);
		}
	}

	/**
	 * Builds error message for incorrect state
	 * @param aActualState the actual state
	 * @param aExpectedStates array of the expected states
	 * @return error message
	 */
	private static String buildWrongStateMessage( final McStateEnum aActualState, final McStateEnum[] aExpectedStates ) {
		StringBuilder sb = new StringBuilder();
		sb.append( EXCEPTION_TEXT_WRONG_STATE_PART_1 );
		sb.append(aActualState);
		sb.append( EXCEPTION_TEXT_WRONG_STATE_PART_2 );
		for (int i = 0; i < aExpectedStates.length; i++ ) {
			if ( i > 0 ) {
				sb.append(", ");
			}
			sb.append( aExpectedStates[ i ] );
		}
		
		return sb.toString();
	}
	
	/**
	 * Error handling for incorrect state
	 * @param aActualState the actual state
	 * @param aExpectedStates array of the expected states
	 * @throws JniExecutorWrongStateException 
	 */
	private static void handleWrongState( final McStateEnum aActualState, final McStateEnum[] aExpectedStates ) throws JniExecutorWrongStateException {
		final String msg = buildWrongStateMessage( aActualState, aExpectedStates );
		throw new JniExecutorWrongStateException( msg );
	}

	/**
	 * Waits until shutdown and terminate. SYNCHRONOUS
	 * <p>
	 * Recommended use:
	 * <ol> 
	 *  <li> Call it right after {@link #startHostControllers()}, which is asynchronous, and it will wait until the whole
	 *     process will be finished automatically or manually, successfully or unsuccessfully.
	 *  <li> Call it right after {@link #shutdownSession()} in any state to make sure, that session is shut down.
	 * </ol>
	 * <p>
	 * Finished means that shutdown session is requested and the state gets back to MC_INACTIVE.
	 * <br>
	 * If it is called when the executor is disconnected, it will return immediately.
	 * <br>
	 * If it is called in MC_INACTIVE state, the executor is terminated and returns immediately.
	 */
	public void waitForCompletion() {
		Log.fi();
		//NOTE: check must be changed to ( getState() == McStateEnum.MC_INACTIVE ) if
		//      shutdownSession() does NOT call terminate() when it gets to MC_INACTIVE state
		if ( !isConnected() ) {
			// The request is already finished, so request was called in disconnected/terminated state
			Log.f("The request is already finished, so request was called in disconnected/terminated state");
		}
		else {
			try {
				synchronized (mLockCompletion) {
					mLockCompletion.wait();
				}
			} catch (InterruptedException e) {
				Log.f(e.toString());
			}
		}
		Log.fo();
	}
	
	/**
	 * Reset member variables to initial state
	 */
	private void reset() {
		mObserver = null;
		mMcPort = -1;
		mMcHost = DEFAULT_LOCAL_HOST_ADDRESS;
		mHostControllers = null;
		mCfgFilePreprocessed = false;
		mShutdownRequested = false;
		synchronized (mLockCompletion) {
			mLockCompletion.notifyAll();
		}
	}
    
	/**
	 * If notification from MC is special (it must be handled differently)
	 * @param aMsg notification from MC, which is a possible special notification.
	 * @return true, if notification is handled, no need to send general notification
	 *         false otherwise
	 */
	private boolean processSpecialNotifications( final String aMsg ) {
		return processVerdict(aMsg)
			//|| processDynamicTestcaseError(aMsg)
			|| processVerdictStats(aMsg)
			//|| processErrorsOutsideOfTestcases(aMsg)
			//|| processOverallVerdict(aMsg)
			|| false;
	}
	
	/**
	 * If notification from MC matches to the verdict pattern,
	 * verdict notification is sent instead of general notification.
	 * @param aMsg notification from MC, which is a possible verdict.
	 * @return true, if notification is handled (verdict notification is sent),
	 *                  no need to send general notification
	 *         false otherwise
	 */
	private boolean processVerdict( final String aMsg ) {
		try {
			Matcher m = PATTERN_VERDICT.matcher(aMsg);
			if ( m.matches() ) {
				final String testcase = m.group(PATTERN_VERDICT_GROUP_INDEX_TESTCASE);
				final String verdictTypeName = m.group(PATTERN_VERDICT_GROUP_INDEX_VERDICTTYPENAME);
				VerdictTypeEnum verdictType = toVerdictType(verdictTypeName);
				if ( verdictType == null ) {
					return false;
				}
				observerVerdict(testcase, verdictType);
				return true;
			}
		} catch (Exception e) {
			Log.i(e.toString());
		}
		return false;
	}

	/**
	 * If notification from MC matches to the dynamic testcase error,
	 * error notification is sent instead of general notification.
	 * @param aMsg notification from MC, which is a possible dynamic testcase error.
	 * @return true, if notification is handled (error notification is sent),
	 *                  no need to send general notification
	 *         false otherwise
	 */
	private boolean processDynamicTestcaseError( final String aMsg ) {
		try {
			Matcher m = PATTERN_DYNAMIC_TESTCASE_ERROR.matcher(aMsg);
			if ( m.matches() ) {
				final String errorMsg = m.group(PATTERN_DYNAMIC_TESTCASE_ERROR_GROUP_INDEX_ERROR_TEXT);
				//TODO: send error notification
				return true;
			}
		} catch (Exception e) {
			Log.i(e.toString());
		}
		return false;
	}

	/**
	 * If notification from MC matches to the verdict statistics pattern,
	 * verdict statistics notification is sent instead of general notification.
	 * @param aMsg notification from MC, which is a possible verdict statistics.
	 * @return true, if notification is handled (verdict statistics notification is sent),
	 *                  no need to send general notification
	 *         false otherwise
	 */
	private boolean processVerdictStats( final String aMsg ) {
		try {
			Matcher m = PATTERN_VERDICT_STATS.matcher(aMsg);
			if ( m.matches() ) {
				final int none = Integer.parseInt(m.group(PATTERN_VERDICT_STATS_GROUP_INDEX_NONE));
				final int pass = Integer.parseInt(m.group(PATTERN_VERDICT_STATS_GROUP_INDEX_PASS));
				final int inconc = Integer.parseInt(m.group(PATTERN_VERDICT_STATS_GROUP_INDEX_INCONC));
				final int fail = Integer.parseInt(m.group(PATTERN_VERDICT_STATS_GROUP_INDEX_FAIL));
				final int error = Integer.parseInt(m.group(PATTERN_VERDICT_STATS_GROUP_INDEX_ERROR));
				final Map<VerdictTypeEnum, Integer> verdictStats = new HashMap<>();
				verdictStats.put(VerdictTypeEnum.NONE, none);
				verdictStats.put(VerdictTypeEnum.PASS, pass);
				verdictStats.put(VerdictTypeEnum.INCONC, inconc);
				verdictStats.put(VerdictTypeEnum.FAIL, fail);
				verdictStats.put(VerdictTypeEnum.ERROR, error);
				observerVerdictStats(verdictStats);
				return true;
			}
		} catch (Exception e) {
			Log.i(e.toString());
		}
		return false;
	}
	
	/**
	 * If notification from MC matches to the PATTERN_ERRORS_OUTSIDE_OF_TESTCASES pattern,
	 * special notification is sent instead of general notification.
	 * @param aMsg notification from MC, which is a possible verdict statistics.
	 * @return true, if notification is handled (special notification is sent),
	 *                  no need to send general notification
	 *         false otherwise
	 */
	private boolean processErrorsOutsideOfTestcases( final String aMsg ) {
		try {
			Matcher m = PATTERN_ERRORS_OUTSIDE_OF_TESTCASES.matcher(aMsg);
			if ( m.matches() ) {
				final int errors = Integer.parseInt(m.group(PATTERN_ERRORS_OUTSIDE_OF_TESTCASES_GROUP_INDEX_ERRORS));
				//TODO
				return true;
			}
		} catch (Exception e) {
			Log.i(e.toString());
		}
		return false;
	}
	
	/**
	 * If notification from MC matches to the overall verdict pattern,
	 * overall verdict notification is sent instead of general notification.
	 * @param aMsg notification from MC, which is a possible verdict statistics.
	 * @return true, if notification is handled (verdict statistics notification is sent),
	 *                  no need to send general notification
	 *         false otherwise
	 */
	private boolean processOverallVerdict( final String aMsg ) {
		try {
			Matcher m = PATTERN_OVERALL_VERDICT.matcher(aMsg);
			if ( m.matches() ) {
				final int executed = Integer.parseInt(m.group(PATTERN_OVERALL_VERDICT_GROUP_INDEX_NUMBER_OF_TESTCASE));
				final String verdictTypeName = m.group(PATTERN_OVERALL_VERDICT_GROUP_INDEX_VERDICTTYPENAME);
				final VerdictTypeEnum verdictType = toVerdictType(verdictTypeName);
				//TODO
				return true;
			}
		} catch (Exception e) {
			Log.i(e.toString());
		}
		return false;
	}
	
	/**
	 * Converts verdict type name to verdict type
	 * @param aVerdictTypeName verdict type name candidate
	 * @return converted verdict type or null if name is invalid
	 */
	private static VerdictTypeEnum toVerdictType( final String aVerdictTypeName ) {
		VerdictTypeEnum verdictType = null;
		switch ( aVerdictTypeName ) {
		case "none":
			verdictType = VerdictTypeEnum.NONE;
			break;
		case "pass":
			verdictType = VerdictTypeEnum.PASS;
			break;
		case "inconc":
			verdictType = VerdictTypeEnum.INCONC;
			break;
		case "fail":
			verdictType = VerdictTypeEnum.FAIL;
			break;
		case "error":
			verdictType = VerdictTypeEnum.ERROR;
			break;
		default:
			Log.i("Unknown verdict type: " + aVerdictTypeName);
		}
		return verdictType;
	}
	
	// observer callbacks. all the callbacks go through these methods to make sure, that
	//  - observer is not called if it's null
	//  - exception/error, which is thrown by the observer is ignored,
	//    this class provides a service, it has no idea how to handle client's exception/error
        
	/**
	 * Notification about status change. It also means, that the asynchronous request is finished successfully, if aNewState is the final state.
	 * @param aNewState the new MC state
	 */
	private void observerStatusChanged( final McStateEnum aNewState ) {
		Log.fi(aNewState);
		if ( mObserver != null ) {
			try {
				mObserver.statusChanged(aNewState);
			} catch (Throwable e) {
				Log.i(e.toString());
				// ignore client's exception/error, we have no idea how to handle
			}
		}
		Log.fo();
	}

	/**
	 * Error from MC. It also means, that the asynchronous request is finished unsuccessfully.
	 * @param aSeverity error severity
	 * @param aMsg error message
	 */
	private void observerError( final int aSeverity, final String aMsg ) {
		Log.fi(aSeverity, aMsg);
		if ( mObserver != null ) {
                	try {
				mObserver.error(aSeverity, aMsg);
			} catch (Throwable e) {
				Log.i(e.toString());
				// ignore client's exception/error, we have no idea how to handle
			}
		}
		Log.fo();
	}

	/**
	 * Notification callback, information comes from MC
	 * @param aTime timestamp
	 * @param aSource source, the machine identifier of MC 
	 * @param aSeverity message severity
	 * @param aMsg message text
	 */
	private void observerNotify(final Timeval aTime, final String aSource, final int aSeverity, final String aMsg) {
		Log.fi(aTime, aSource, aSeverity, aMsg);
		if ( mObserver != null ) {
                	try {
				mObserver.notify(aTime, aSource, aSeverity, aMsg);
			} catch (Throwable e) {
				Log.i(e.toString());
				// ignore client's exception/error, we have no idea how to handle
			}
		}
		Log.fo();
	}
        
	/**
	 * Verdict notification, that comes after execution of a testcase.
	 * If a test control is executed, this notification is sent multiple times after each testcase.
	 * @param aTestcase name of the testcase
	 * @param aVerdictType verdict type
	 */ 
	private void observerVerdict( final String aTestcase, final VerdictTypeEnum aVerdictType ) {
		Log.fi(aTestcase, aVerdictType);
		if ( mObserver != null ) {
                	try {
				mObserver.verdict(aTestcase, aVerdictType);
			} catch (Throwable e) {
				Log.i(e.toString());
				// ignore client's exception/error, we have no idea how to handle
			}
		}
		Log.fo();
	}
        
	/**
	 * Verdict statistics notification, that comes after executing all the testcases after exit MTC.
	 * This notification is sent only once for a MTC session.
	 * @param aVerdictStats verdict statistics
	 */
	private void observerVerdictStats( final Map<VerdictTypeEnum, Integer> aVerdictStats ) {
		Log.fi(aVerdictStats);
		if ( mObserver != null ) {
                	try {
				mObserver.verdictStats(aVerdictStats);
			} catch (Throwable e) {
				Log.i(e.toString());
				// ignore client's exception/error, we have no idea how to handle
			}
		}
		Log.fo();
	}
}
