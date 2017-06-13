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

import java.io.File;

import org.eclipse.titan.executorapi.exception.JniExecutorIllegalArgumentException;

/**
 * Contains the data describing a Host Controller.
 */
public final class HostController {
	
	// Exception texts
	
	/** Used by the constructor */
	private static final String EXCEPTION_TEXT_ILLEGAL_ARG_WORKINGDIR_NULL = "Working directory is null";
	/** Used by the constructor */
	private static final String EXCEPTION_TEXT_ILLEGAL_ARG_EXECUTABLE_NULL = "Executable is null";
	/** Used by the constructor */
	private static final String EXCEPTION_TEXT_ILLEGAL_ARG_WORKINGDIR_NOT_EXIST = "Working directory does NOT exists";
	/** Used by the constructor */
	private static final String EXCEPTION_TEXT_ILLEGAL_ARG_EXECUTABLE_NOT_EXIST = "Executable does NOT exists";
	
	/**
	 * The name of the host, it can be null, default: null (localhost)
	 */
	private String mHost = null;
	
	/**
	 * The working directory to use when executing commands
	 */
	private String mWorkingDirectory;
	
	/**
	 * The executable if the Host Controller.
	 * This executable is started (with 2 parameters: MC host, MC port)
	 * in the working directory when a HC is started. 
	 */
	private String mExecutable;

	/**
	 * Constructor
	 * @param aHost the name of the host, it can be null (localhost)
	 * @param aWorkingdirectory the working directory to use when executing commands
	 * @param aExecutable the Host Controller's executable to start
	 * @throws JniExecutorIllegalArgumentException if aWorkingdirectory == null or aExecutable == null 
	 */
	public HostController( final String aHost, final String aWorkingdirectory, final String aExecutable ) throws JniExecutorIllegalArgumentException {
		if ( aWorkingdirectory == null ) {
			if ( aExecutable == null ) {
				throw new JniExecutorIllegalArgumentException( EXCEPTION_TEXT_ILLEGAL_ARG_WORKINGDIR_NULL + ", " + EXCEPTION_TEXT_ILLEGAL_ARG_EXECUTABLE_NULL );
			} else {
				throw new JniExecutorIllegalArgumentException( EXCEPTION_TEXT_ILLEGAL_ARG_WORKINGDIR_NULL );
			}
		} else if ( aExecutable == null ) {
			throw new JniExecutorIllegalArgumentException( EXCEPTION_TEXT_ILLEGAL_ARG_EXECUTABLE_NULL );
		}
		
		if ( isLocalhost(aHost) ) {
			// if working directory directory is local, it must exist
			final File workingDir = new File(aWorkingdirectory);
			if ( !workingDir.exists() || !workingDir.isDirectory() ) {
				throw new JniExecutorIllegalArgumentException( EXCEPTION_TEXT_ILLEGAL_ARG_WORKINGDIR_NOT_EXIST );
			}

			// also the executable
			final String execFullPath = aWorkingdirectory + ( aWorkingdirectory.endsWith( File.separator ) ? "" : File.separator ) + aExecutable;
			final File executable = new File( execFullPath );
			if ( !executable.exists() || !executable.isFile() ) {
				throw new JniExecutorIllegalArgumentException( EXCEPTION_TEXT_ILLEGAL_ARG_EXECUTABLE_NOT_EXIST );
			}
		}
		
		this.mHost = aHost;
		this.mWorkingDirectory = aWorkingdirectory;
		this.mExecutable = aExecutable;
	}
	
	public String getHost() {
		return mHost;
	}

	public String getWorkingDirectory() {
		return mWorkingDirectory;
	}

	public String getExecutable() {
		return mExecutable;
	}

	/**
	 * Builds a command to start the HC
	 * @param aMcHost MainController host
	 * @param aMcPort MainController port
	 * @return the actual command to execute in a shell to start up the Host Controller fully configured
	 */
	public String getCommand( final String aMcHost, final int aMcPort ) {
		final StringBuilder commandSb = new StringBuilder();

		if ( !isLocalhost( mHost ) ) {
			// remote host
			// ssh %Host cd %Workingdirectory; %Executable %MCHost %MCPort
			commandSb.append( "ssh " );
			commandSb.append( mHost );
			commandSb.append( " " );
		}
		
		// local host
		// cd %Workingdirectory; %Executable %MCHost %MCPort
		
		commandSb.append( "cd " );
		commandSb.append( mWorkingDirectory );
		commandSb.append( "; ./" );
		commandSb.append( mExecutable );
		commandSb.append( " " );
		commandSb.append( "NULL".equals( aMcHost ) ? "0.0.0.0" : aMcHost );
		commandSb.append( " " );
		commandSb.append( aMcPort );
		return commandSb.toString();
	}
	
	/**
	 * Checks if the host name is localhost
	 * @param aHostName the host name, it can be IP address, it can be null
	 * @return true if the host name is localhost, false otherwise
	 */
	private static boolean isLocalhost( final String aHostName ) {
		return (
				aHostName == null ||
				"".equalsIgnoreCase( aHostName ) ||
				"localhost".equalsIgnoreCase( aHostName ) ||
				"0.0.0.0".equalsIgnoreCase( aHostName ) ||
				"NULL".equalsIgnoreCase( aHostName )
			   );
	}
}

