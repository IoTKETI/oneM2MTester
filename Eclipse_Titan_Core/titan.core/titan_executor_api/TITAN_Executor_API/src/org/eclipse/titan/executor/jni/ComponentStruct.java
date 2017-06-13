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

import org.eclipse.titan.executorapi.util.StringUtil;

/**
 * Data structure for each TC.
 * <p>
 * The original C++ structure can be found at TTCNv3\mctr2\mctr\MainController.h
 */
public final class ComponentStruct {

	public int comp_ref;
	public QualifiedName comp_type;
	public String comp_name;
	public String log_source;
	public HostStruct comp_location;
	public TcStateEnum tc_state;
	public VerdictTypeEnum local_verdict;
	public int tc_fd;
	public byte[] text_buf;
	public QualifiedName tc_fn_name;
	public String return_type;
	public int return_value_len;
	//void *return_value;
	public boolean is_alive;
	public boolean stop_requested;
	public boolean process_killed;

	/**
	 * Constructor
	 * @param tb text_buf size
	 */
	public ComponentStruct(final int tb) {
		text_buf = new byte[tb];
	}
	
	/**
	 * Makes a copy of the original structure.
	 * @param aComponent original
	 */
	public ComponentStruct( final ComponentStruct aComponent ) {
		this.comp_ref = aComponent.comp_ref;
		this.comp_type = new QualifiedName(new String(aComponent.comp_type.module_name), new String(aComponent.comp_type.definition_name));
		this.comp_name = new String(aComponent.comp_name);
		this.log_source = new String(aComponent.log_source);
		this.comp_location = new HostStruct(aComponent.comp_location);
		this.tc_state = aComponent.tc_state;
		this.local_verdict = aComponent.local_verdict;
		this.tc_fd = aComponent.tc_fd;
		this.text_buf = aComponent.text_buf.clone();
		this.tc_fn_name = new QualifiedName(new String(aComponent.tc_fn_name.module_name), new String(aComponent.tc_fn_name.definition_name));;
		this.return_type = new String(aComponent.return_type);
		this.return_value_len = aComponent.return_value_len;
		this.is_alive = aComponent.is_alive;
		this.stop_requested = aComponent.stop_requested;
		this.process_killed = aComponent.process_killed;
	}
	
	@Override
	public String toString() {
		final StringBuilder sb = new StringBuilder();
		sb.append("{\n");
		sb.append("  comp_ref        : " + comp_ref         + "\n");
		sb.append("  comp_type       : " + comp_type        + "\n");
		sb.append("  comp_name       : " + comp_name        + "\n");
		sb.append("  log_source      : " + log_source       + "\n");
		
		final StringBuilder sbCompLocation = new StringBuilder( comp_location.toString() );
		StringUtil.indent(sbCompLocation, "                    ");
		sb.append("  comp_location   : " + sbCompLocation   + "\n");
		
		sb.append("  tc_state        : " + tc_state         + "\n");
		sb.append("  local_verdict   : " + local_verdict.getName()    + "\n");
		sb.append("  tc_fd           : " + tc_fd            + "\n");
		
		sb.append("  text_buf        : ");
		StringUtil.appendObject( sb, text_buf ); 
		sb.append("\n");
		
		sb.append("  tc_fn_name      : " + tc_fn_name       + "\n");
		sb.append("  return_type     : " + return_type      + "\n");
		sb.append("  return_value_len: " + return_value_len + "\n");
		sb.append("  is_alive        : " + is_alive         + "\n");
		sb.append("  stop_requested  : " + stop_requested   + "\n");
		sb.append("  process_killed  : " + process_killed   + "\n");
		sb.append("}");
		return sb.toString();
	}

}
