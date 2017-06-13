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

import java.util.ArrayList;
import java.util.List;

import org.eclipse.titan.executorapi.exception.JniExecutorJniLoadException;
import org.eclipse.titan.executorapi.util.Log;

/**
 * The middleware that connects the JNI executor and the MainController.
 */
public final class JNIMiddleWare {
	private static final String STATUSCHANGE_INDICATOR = "S";
	private static final String ERRORCALLBACK_INDICATOR = "E";
	private static final String NOTIFICATION_INDICATOR = "N";
	private static JniExecutorJniLoadException mException = null;
	
	// Exception texts
	private static final String EXCEPTION_TEXT_JNI_LOAD_WINDOWS = "Execution in JNI mode is not supported on the Windows operating system.";
	private static final String EXCEPTION_TEXT_JNI_LOAD_SECURITY_MGR = "Security manager does not allow loading of the JNI dinamic library.";
	private static final String EXCEPTION_TEXT_JNI_LOAD_UNSATISFIED_LINK_PART_1 = "JNI dynamic library could not be loaded.\n\noriginal message: ";
	private static final String EXCEPTION_TEXT_JNI_LOAD_UNSATISFIED_LINK_PART_2 = "\n\njava.library.path = ";

	/**
	 * EventDispatcher instance, which runs in a separate thread to read the pipe for MC (MainController) events (status changes, notifications, errors).
	 * NOT OWNED, ownership passed to Thread
	 */
	private EventDispatcher mEventDispatcher;
	
	/** Event observer, which is notified about events read by mEventThread */
	private IJNICallback mJniCallback;
	
	/**
	 * The value of the pointer of the C++ API class of the MainController,
	 * or -1 if not connected (JNI-C++ connection is not started yet or already terminated)
	 */
	private volatile long mUserinterfacePtr = -1;
	
	/**
	 * Lock object for synchronizing MC related async and sync requests, and also EventHandler
	 */
	private final Object mLockMcRequest = new Object();

	public JNIMiddleWare(final IJNICallback aJniCallback) {
		this.mJniCallback = aJniCallback;
	}

	public static JniExecutorJniLoadException getException() {
		return mException;
	}
	
	/**
	 * @return true, if connected (JNI-C++ connection is initialized)
     *         false otherwise (not initialized or terminated)
	 */
	public boolean isConnected() {
		return mUserinterfacePtr != -1;
	}
	
	/**
	 * Gets lock object for synchronizing MC related operations.
	 * @return lock object
	 */
	public final Object getLock() {
		return mLockMcRequest;
	}
	
	/**
	 * Test OS
	 * @return true if OS is Windows 
	 */
	public static boolean isWin() {
		return System.getProperty("os.name").toLowerCase().indexOf("win") >= 0;
	}

	static {
		// test OS
		if ( isWin() ) {
			// Windows
			mException = new JniExecutorJniLoadException( EXCEPTION_TEXT_JNI_LOAD_WINDOWS );
		} else {
			try {
				System.loadLibrary("mctrjninative");
				Log.f("mctrjninative loaded");
			} catch (SecurityException e) {
				mException = new JniExecutorJniLoadException( EXCEPTION_TEXT_JNI_LOAD_SECURITY_MGR, e);
				mException.setStackTrace(e.getStackTrace());
			} catch (UnsatisfiedLinkError e) {
				mException = new JniExecutorJniLoadException( EXCEPTION_TEXT_JNI_LOAD_UNSATISFIED_LINK_PART_1 + e.getMessage() +
						                                      EXCEPTION_TEXT_JNI_LOAD_UNSATISFIED_LINK_PART_2 + System.getProperty("java.library.path"), e);
				mException.setStackTrace(e.getStackTrace());
			}
		}
	}

	/**
	 * A simple class to receive messages from the MainController and dispatch them to the Executor.
	 */
	private final class EventDispatcher implements Runnable {
		
		private volatile boolean mActive = true;
		
		@Override
		public void run() {
			Log.fi();
			while ( mActive && mUserinterfacePtr != -1 ) {
				final String s = do_readPipe();
				synchronized (getLock()) {
					if ( s.startsWith( STATUSCHANGE_INDICATOR ) ) {
						Log.f("JNIMiddleWare$EventDispatcher.run() STATUSCHANGE_INDICATOR, s == \"" + s + "\"");
						processStatusChange( s );
					} else if ( s.startsWith( ERRORCALLBACK_INDICATOR ) ) {
						Log.f("JNIMiddleWare$EventDispatcher.run() ERRORCALLBACK_INDICATOR, s == \"" + s + "\"");
						processErrorCallback( s );
					} else if ( s.startsWith( NOTIFICATION_INDICATOR ) ) {
						Log.f("JNIMiddleWare$EventDispatcher.run() NOTIFICATION_INDICATOR, s == \"" + s + "\"");
						processNotifyCallback( s );
					}
				}
			}
			Log.fo();
		}
		
		private void processStatusChange( final String aStatusChangeMsg ) {
			Log.fi(aStatusChangeMsg);
			String lastStatusChangeMsg = aStatusChangeMsg;
			// Do NOT call any JNI functions (except pipe related ones) until there are something to read on the pipe,
			// otherwise C++ side will crash.
			// That's why processStatusChangeCallback(); is the last we call, which contains JNI calls.
			// Probably if there was JNI calls in notify and error callbacks, there would be a crash, too
			List<String> notifications = new ArrayList<String>();
			while ( mActive && do_isPipeReadable() ) {
				final String s = do_readPipe();
				if ( s.startsWith( STATUSCHANGE_INDICATOR ) ) {
					Log.f("JNIMiddleWare$EventDispatcher.processStatusChange() STATUSCHANGE_INDICATOR, s == \"" + s + "\"");
					lastStatusChangeMsg = s;
				} else if ( s.startsWith( ERRORCALLBACK_INDICATOR ) ) {
					Log.f("JNIMiddleWare$EventDispatcher.processStatusChange() ERRORCALLBACK_INDICATOR, s == \"" + s + "\"");
					processErrorCallback( s );
				} else if ( s.startsWith( NOTIFICATION_INDICATOR ) ) {
					Log.f("JNIMiddleWare$EventDispatcher.processStatusChange() NOTIFICATION_INDICATOR, s == \"" + s + "\"");
					notifications.add( s );
				} else {
					break;
				}
			}
			if ( notifications.size() > 0 ) {
				batchedProcessNotifications(notifications);
				notifications.clear();
			}
			processStatusChangeCallback( lastStatusChangeMsg );
			Log.fo();
		}

		/**
		 * Inactivates thread.
		 * When shutdown_session() is completed, EventDispatcher can be in a state,
		 * when there is still something to read on the pipe,
		 * but we do NOT allow to read the pipe for this thread, so run() is completed().
		 * It is done so, because if a new connection is started immediately,
		 * the old EventHandler can catch the new EventHandler's messages from pipe,
		 * which can cause the new EventHandler to wait forever at do_readpipe(). 
		 */
		public void inactivate() {
			mActive = false;
		}
	}


	// public Boolean lock = new Boolean(true);

	// *******************************************************
	// MC manipulating native methods
	// *******************************************************
	// returns the value of a pointer of a C++ class
	private native long init(final int par_max_ptcs);
	private long do_init(final int par_max_ptcs) {
		Log.fi( par_max_ptcs );
		final long result = init(par_max_ptcs);
		Log.fo( result );
		return result;
	}

	/**
	 * Cleans up everything including pipe, so it will not call back. SYNCHRONOUS
	 */
	private native void terminate();
	public void do_terminate() {
		Log.fi();
		terminate();
		Log.fo();
	}

	private native void add_host(final String group_name, final String host_name);
	public void do_add_host(final String group_name, final String host_name) {
		Log.fi( group_name, host_name );
		add_host(group_name, host_name);
		Log.fo();
	}

	private native void assign_component(final String host_or_group, final String component_id);
	public void do_assign_component(final String host_or_group, final String component_id) {
		Log.fi( host_or_group, component_id );
		assign_component(host_or_group, component_id);
		Log.fo();
	}

	private native void destroy_host_groups();
	public void do_destroy_host_groups() {
		Log.fi();
		destroy_host_groups();
		Log.fo();
	}

	private native void set_kill_timer(final double timer_val);
	public void do_set_kill_timer(final double timer_val) {
		Log.fi( timer_val );
		set_kill_timer(timer_val);
		Log.fo();
	}

	private native int start_session(final String local_address, final int tcp_port, final boolean unixdomainsocketenabled);
	public int do_start_session(final String local_address, final int tcp_port, final boolean unixdomainsocketenabled) {
		Log.fi( local_address, tcp_port, unixdomainsocketenabled );
		final int result = start_session(local_address, tcp_port, unixdomainsocketenabled);
		Log.fo( result );
		return result;
	}

	private native void shutdown_session();
	public void do_shutdown_session() {
		Log.f("JNIMiddleWare.do_shutdown_session()");
		shutdown_session();
	}

	/**
	 * ASYNCHRONOUS
	 * Configure MainController and HCs.
	 * It is also known as "Set parameter" operation.
	 * 
	 * It can be called in the following states:
	 *   MC_LISTENING            -> MC_LISTENING_CONFIGURED will be the next state, when configuring operation is finished
	 *   MC_LISTENING_CONFIGURED -> MC_LISTENING_CONFIGURED will be the next state, when configuring operation is finished
	 *   MC_HC_CONNECTED         -> MC_CONFIGURING immediately and MC_ACTIVE when configuring operation is finished
	 *   
	 * @param config_file config string, generated from the config file by removing unnecessary parts. It can be empty string, and also null.
	 *                    Generating config string can be done in 2 ways:
	 *                      by "hand" (if the parameter is not empty string and not null):
	 *                        on the java side, and pass it as a parameter, for an example see JniExecutor.DEFAULT_CONFIG_STRING or BaseExecutor.generateCfgString()
	 *                      by MainController (if the parameter is empty string or null):
	 *                        call set_cfg_file() after init() with the cfg file, and let MainController process it and create the config string,
	 *                        and then config() can be called with empty string or null as a parameter, which means that config string from MainController config data will be used.
	 */
	private native void configure( final String config_file );
	
	/**
	 * Wrapper for the native configure() with logging functionality.
	 * @param config_file config string
	 */
	public void do_configure( final String config_file ) {
		Log.fi();
		Log.f( "config_file:" );
		Log.f( config_file );
		configure( config_file );
		Log.fo();
	}

	/**
	 * SYNCHRONOUS
	 * The name of the config file is sent to MainController for syntactic and semantic analysis.
     * The result config string is needed by start_session() and configure().
     * Result is stored in MainController.
     * We need the following info from it:
     * <ul>
     *   <li>config_read_buffer: passed to configure() if parameter is an empty string
     *   <li>local_address:      needed by the java side for start_session() and starting the HC, it is read by calling get_mc_host()
     *   <li>tcp_listen_port:    needed by the java side for start_session(), it is read by calling get_port()
     * </ul>
	 * It must be called right after init().
	 * @param config_file_name TTCN-3 cfg file name with full path
	 */
	private native void set_cfg_file(final String config_file_name);
	
	/**
	 * Wrapper for the native set_cfg_file() with logging functionality.
	 * @param config_file_name TTCN-3 cfg file name with full path
	 */
	public void do_set_cfg_file(final String config_file_name) {
		Log.fi( config_file_name );
		set_cfg_file(config_file_name);
		Log.fo();
	}

	/**
	 * Gets the MainController host,
	 * which is extracted from the config file, so set_cfg_file()
	 * must be called before this to get valid data.
	 * It is needed as an input parameter for start_session() and for starting the HCs.
	 * @return MC host
	 */
	private native String get_mc_host();
	
	/**
	 * Wrapper for the native get_port() with logging functionality.
	 * @return MC host
	 */
	public String do_get_mc_host() {
		Log.fi();
		final String result = get_mc_host();
		Log.fo( result );
		return result;
	}

	/**
	 * Gets the TCP listen port,
	 * which is extracted from the config file, so set_cfg_file()
	 * must be called before this to get valid data.
	 * It is needed as an input parameter for start_session().
	 * @return TCP listen port
	 */
	private native int get_port();
	
	/**
	 * Wrapper for the native get_port() with logging functionality.
	 * @return TCP listen port
	 */
	public int do_get_port() {
		Log.fi();
		final int result = get_port();
		Log.fo( result );
		return result;
	}

	private native void create_mtc(final int host_index);
	public void do_create_mtc(final int host_index) {
		Log.fi( host_index );
		create_mtc(host_index);
		Log.fo();
	}

	private native void exit_mtc();
	public void do_exit_mtc() {
		Log.fi();
		exit_mtc();
		Log.fo();
	}

	private native void execute_control(final String module_name);
	public void do_execute_control(final String module_name) {
		Log.fi( module_name );
		execute_control(module_name);
		Log.fo();
	}

	/**
	 * Execute testcase by name
	 * @param module_name module name
	 * @param testcase_name
	 */
	private native void execute_testcase(final String module_name, final String testcase_name);
	public void do_execute_testcase(final String module_name, final String testcase_name) {
		Log.fi( module_name, testcase_name );
		execute_testcase(module_name, testcase_name);
		Log.fo();
	}

	/**
	 * @return The length of the execute list,
	 *         which is defined in the [EXECUTE] section in the configuration file.
	 *         0, if there is no [EXECUTE] section in the configuration file.
	 */
	private native int get_execute_cfg_len();
	public int do_get_execute_cfg_len() {
		Log.fi();
		final int result = get_execute_cfg_len();
		Log.fo( result );
		return result;
	}
	
	/**
	 * Executes the index-th element of the execute list,
	 * which is defined in the [EXECUTE] section in the configuration file.
	 * @param index The test index from the execute list
	 */
	private native void execute_cfg(final int index);
	public void do_execute_cfg(final int index) {
		Log.fi(index);
		execute_cfg(index);
		Log.fo();
	}

	private native void stop_after_testcase(final boolean new_state);
	public void do_stop_after_testcase(final boolean new_state) {
		Log.fi( new_state );
		stop_after_testcase(new_state);
		Log.fo();
	}

	private native void continue_testcase();
	public void do_continue_testcase() {
		Log.fi();
		continue_testcase();
		Log.fo();
	}

	private native void stop_execution();
	public void do_stop_execution() {
		Log.fi();
		stop_execution();
		Log.fo();
	}

	// returns mc_state_enum
	private native McStateEnum get_state();
	public McStateEnum do_get_state() {
		final McStateEnum state = get_state();
		Log.f("JNIMiddleWare.do_get_state() " + state );
		return state;
	}

	private native boolean get_stop_after_testcase();
	public boolean do_get_stop_after_testcase() {
		final boolean result = get_stop_after_testcase();
		Log.f("JNIMiddleWare.do_get_stop_after_testcase() " + result);
		return result;
	}

	private native int get_nof_hosts();
	public int do_get_nof_hosts() {
		final int result = get_nof_hosts();
		Log.f("JNIMiddleWare.do_get_nof_hosts() " + result);
		return result;
	}

	private native HostStruct get_host_data(final int host_index);
	public HostStruct do_get_host_data(final int host_index) {
		final HostStruct result = get_host_data(host_index);
		Log.f("JNIMiddleWare.do_get_host_data( " + host_index + " ) " + result);
		return result;
	}

	private native ComponentStruct get_component_data(final int component_reference);
	public ComponentStruct do_get_component_data(final int component_reference) {
		final ComponentStruct result = get_component_data(component_reference);
		Log.f("JNIMiddleWare.do_get_component_data( " + component_reference +" ) " + result);
		return result;
	}

	private native void release_data();
	public void do_release_data() {
		Log.fi();
		release_data();
		Log.fo();
	}

	/**
	 * Transforms the MC's state into a humanly readable text.
	 *
	 * @param state the MC state
	 * @return the name of the state provided
	 */
	private native String get_mc_state_name(final McStateEnum state);
	public String do_get_mc_state_name(final McStateEnum state) {
		final String result = get_mc_state_name(state);
		Log.f("JNIMiddleWare.do_get_mc_state_name( " + state +" ) " + result);
		return result;
	}

	/**
	 * Transforms the HC's state into a humanly readable text.
	 *
	 * @param state the HC state
	 * @return the name of the state provided
	 */
	private native String get_hc_state_name(final HcStateEnum state);
	public String do_get_hc_state_name(final HcStateEnum state) {
		final String result = get_hc_state_name(state);
		Log.f("JNIMiddleWare.do_get_hc_state_name( " + state +" ) " + result);
		return result;
	}

	/**
	 * Transforms the testcase's state into a humanly readable text.
	 *
	 * @param state the testcase state
	 * @return the name of the state provided
	 */
	private native String get_tc_state_name(final TcStateEnum state);
	public String do_get_tc_state_name(final TcStateEnum state) {
		final String result = get_tc_state_name(state);
		Log.f("JNIMiddleWare.do_get_tc_state_name( " + state +" ) " + result);
		return result;
	}

	/**
	 * Transforms the transportation type into a humanly readable text.
	 *
	 * @param transport the type of transport to use to communicate wit the HCs
	 * @return the name of the state provided
	 */
	private native String get_transport_name(final TransportTypeEnum transport);
	public String do_get_transport_name(final TransportTypeEnum transport) {
		final String result = get_transport_name(transport);
		Log.f("JNIMiddleWare.do_get_transport_name( " + transport + " ) " + result);
		return result;
	}

	// *******************************************************
	// Other native methods
	// *******************************************************
	private native void check_mem_leak(final String program_name);
	public void do_check_mem_leak(final String program_name) {
		Log.f("JNIMiddleWare.do_get_transport_name( " + program_name +" )");
		check_mem_leak(program_name);
	}

	private native void print_license_info();
	public void do_print_license_info() {
		Log.f("JNIMiddleWare.do_print_license_info()");
		print_license_info();
	}

	private native int check_license();
	public int do_check_license() {
		final int result = check_license();
		Log.f("JNIMiddleWare.do_check_license() " + result);
		return result;
	}

	/**
	 * Gets status change/notification/error string from pipe.
	 * <ul>
	 * <li>Saa000 is status change, where
	 *   <ul>
	 *   <li> aa is the MC state in decimal string (0-14)
	 *   </ul>
	 *   Example: S01000
	 * <li>Naaaaab*|c*|d*|e* is notification, where
	 *   <ul>
	 *   <li>aaaaa packet header
	 *   <li>b* (variable length, decimal string) timestamp tv_sec part
	 *   <li>c* (variable length, decimal string) timestamp tv_usec part
	 *   <li>d* (variable length, stuffed string) source
	 *   <li>e* (variable length, decimal string) severity
	 *   <li>f* (variable length, stuffed string) message
	 *   </ul>
	 *   Example: N000751401991983|667374|MTC@ubuntu|20|Test case HelloW2 finished. Verdict: inconc
	 * <li>Eaaaaab*|c* is error, where
	 *   <ul>
	 *   <li>b* (variable length, decimal string) severity
	 *   <li>c* (variable length, stuffed string) message
	 *   </ul>
	 * </ul>
	 * 
	 * stuffed string: contains escape char ('\'), which is resolved in splitPacket().
	 * See also JNI/jnimw.cc stuffer() 
	 * @return status change/notification/error string from pipe
	 */
	private native String readPipe();
	public String do_readPipe() {
		Log.fi();
		final String result = readPipe();
		Log.fo( result );
		return result;
	}

	private native boolean isPipeReadable();
	public boolean do_isPipeReadable() {
		final boolean result = isPipeReadable();
		Log.f("JNIMiddleWare.do_isPipeReadable() " + result);
		return result;
	}

	private static native long getSharedLibraryVersion();
	public static long do_getSharedLibraryVersion() {
		final long result = getSharedLibraryVersion();
		Log.f("JNIMiddleWare.do_getSharedLibraryVersion() " + result);
		return result;
	}

	/**
	 * Initialize the Main Controller.
	 *
	 * @param par_max_ptcs the maximum number of PTCs this Main Controller will be allowed to handle at a time.
	 */
	public void initialize( final int par_max_ptcs ) {
		Log.fi();
		if (mUserinterfacePtr == -1) {
			mUserinterfacePtr = do_init( par_max_ptcs );
			mEventDispatcher = new EventDispatcher();
			new Thread( mEventDispatcher ).start();
		}
		// otherwise Main Controller has been already initialized, nothing to do
		Log.fo();
	}

	public void terminate_internal() {
		Log.fi();
		mUserinterfacePtr = -1;
		mEventDispatcher.inactivate();
		mEventDispatcher = null;
		do_terminate();
		Log.fo();
	}

	/**
	 * Decodes the MainController sent messages.
	 * Messages sent by the Main Controller are separated with a '|' sign
	 *
	 * @param s the string to split
	 *
	 * @return the resulting string list
	 * */
	private ArrayList<String> splitPacket(final String s) {
		final String pdu = s.substring(6);
		final ArrayList<String> stringList = new ArrayList<String>();
		StringBuilder sb = new StringBuilder();
		final int length = pdu.length();
		char ch;
		int i = 0;
		while (i < length) {
			ch = pdu.charAt(i);
			switch (ch) {
			case '|':
				stringList.add(sb.toString());
				sb = new StringBuilder();
				i++;
				break;
			case '\\':
				sb.append(pdu.charAt(i + 1));
				i += 2;
				break;
			default:
				sb.append(ch);
				i++;
				break;
			}
		}
		stringList.add(sb.toString());
		return stringList;
	}

	/**
	 * Transfers the MainController sent status change notification to the executor.
	 * @param s status change message form pipe
	 */
	private void processStatusChangeCallback(final String s) {
		Log.fi();
		// Saa000, where aa is the MC state in decimal string
		int i = Integer.parseInt(s.substring(1, 3));
		McStateEnum state = McStateEnum.values()[i];
		mJniCallback.statusChangeCallback(state);
		Log.fo();
	}

	private void processErrorCallback(final String s) {
		Log.fi( s );
		final String[] tempArray = new String[1];
		final String[] ppdu = splitPacket(s).toArray(tempArray);

		mJniCallback.errorCallback(Integer.parseInt(ppdu[0]), ppdu[1]);
		Log.fo();
	}

	public void batchedProcessNotifications(final List<String> s) {
		Log.fi( s );
		final String[] tempArray = new String[1];
		final ArrayList<String[]> result = new ArrayList<String[]>();
		for (int i = 0, size = s.size(); i < size; i++) {
			result.add(splitPacket(s.get(i)).toArray(tempArray));
		}
		mJniCallback.batchedInsertNotify(result);
		Log.fo();
	}

	public void processNotifyCallback(final String s) {
		Log.fi( s );
		final String[] tempArray = new String[1];
		final String[] ppdu = splitPacket(s).toArray(tempArray);

		mJniCallback.notifyCallback(new Timeval(Integer.parseInt(ppdu[0]), Integer.parseInt(ppdu[1])), ppdu[2], Integer.parseInt(ppdu[3]), ppdu[4]);
		Log.fo();
	}
}
