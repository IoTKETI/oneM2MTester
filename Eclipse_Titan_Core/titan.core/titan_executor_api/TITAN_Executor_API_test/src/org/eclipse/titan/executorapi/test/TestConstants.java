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

import java.io.File;

public class TestConstants {

	//private static final String WORKSPACE = "/home/earplov/runtime-EclipseApplication/";
	//public static final String WORKINGDIR = WORKSPACE + "hw/bin";
	//public static final String EXECUTABLE = "hw";
	//public static final String CFG_FILE = WORKSPACE + "hw/src/MyExample.cfg";
	
	private static final String WORKSPACE = "../../titan/Install/";
	public static final String WORKINGDIR = getWorkspace() + "demo";
	public static final String EXECUTABLE = "MyExample";
	public static final String CFG_FILE = getWorkspace() + "demo/MyExample.cfg";
	
	public static final String MODULE = EXECUTABLE;
	public static final String TESTCASE1 = "HelloW"; 
	public static final String TESTCASE2 = "HelloW2"; 
	
	// constants for error situations
	
	//public static final String EXISTS_BUT_NOT_DIR_WORKINGDIR = WORKSPACE + "hw/bin/hw";
	//public static final String EXISTS_BUT_NOT_FILE_WORKINGDIR = WORKSPACE + "hw";
	//public static final String EXISTS_BUT_NOT_FILE_EXECUTABLE = "bin";
	
	/** extension for a filename to make sure, that it will invalid */
	public static final String NONEXISTENTFILENAMEEXT = "72634753271645721854657214521";
	
	/** Working dir for this case: exists, but not a dir */
	public static final String EXISTS_BUT_NOT_DIR_WORKINGDIR = WORKSPACE + "demo/MyExample";
	
	/** Working dir for this case: executable exists, but not a file (it's a directory) */
	public static final String EXISTS_BUT_NOT_FILE_WORKINGDIR = WORKSPACE;
	
	/** Executable for this case: executable exists, but not a file (it's a directory) */
	public static final String EXISTS_BUT_NOT_FILE_EXECUTABLE = "demo";
	
	private static final String getWorkspace() {
		String ttcn3_dir = System.getenv().get("TTCN3_DIR");
		String workspace = WORKSPACE;
	    if (ttcn3_dir != null && ttcn3_dir.length() > 0) {
	    	workspace = ttcn3_dir;
	    }
	    workspace += ( workspace.endsWith( File.separator ) ? "" : File.separator );
    	return workspace;
	}
}
