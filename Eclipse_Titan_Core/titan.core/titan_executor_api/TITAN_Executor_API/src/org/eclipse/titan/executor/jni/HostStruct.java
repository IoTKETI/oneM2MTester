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
 * Data structure for each host (and the corresponding HC).
 * <p>
 * The original C++ structure can be found at TTCNv3\mctr2\mctr\MainController.h
 */
public final class HostStruct {
	public String ip_addr;
	public String hostname;
	public String hostname_local;
	public String machine_type;
	public String system_name;
	public String system_release;
	public String system_version;
	public boolean[] transport_supported;
	public String log_source;
	public HcStateEnum hc_state;
	public int hc_fd;
	public byte[] text_buf;
	public int[] components;
	public String[] allowed_components;
	public boolean all_components_allowed;
	public int n_active_components;

	public HostStruct(final int tr, final int tb, final int co, final int ac) {
		transport_supported = new boolean[tr];
		text_buf = new byte[tb];
		components = new int[co];
		allowed_components = new String[ac];
	}

	/**
	 * Makes a copy of the original structure.
	 * @param aHost original
	 */
	public HostStruct( final HostStruct aHost ) {
		this.ip_addr = new String(aHost.ip_addr);
		this.hostname = new String(aHost.hostname);
		this.hostname_local = new String(aHost.hostname_local);
		this.machine_type = new String(aHost.machine_type);
		this.system_name = new String(aHost.system_name);
		this.system_release = new String(aHost.system_release);
		this.system_version = new String(aHost.system_version);
		this.transport_supported = aHost.transport_supported.clone();
		this.log_source = new String(aHost.log_source);
		this.hc_state = aHost.hc_state;
		this.hc_fd = aHost.hc_fd;
		this.text_buf = aHost.text_buf.clone();
		this.components = aHost.components.clone();
		this.allowed_components = new String[aHost.allowed_components.length];
		for (int i = 0; i < aHost.allowed_components.length; i++) {
			this.allowed_components[i] = new String(aHost.allowed_components[i]);
		}
		this.all_components_allowed = aHost.all_components_allowed;
		this.n_active_components = aHost.n_active_components;
	}

	@Override
	public String toString() {
		final StringBuilder sb = new StringBuilder();
		sb.append("{\n");
		sb.append("  ip_addr               : " + ip_addr                + "\n");
		sb.append("  hostname              : " + hostname               + "\n");
		sb.append("  hostname_local        : " + hostname_local         + "\n");
		sb.append("  machine_type          : " + machine_type           + "\n");
		sb.append("  system_name           : " + system_name            + "\n");
		sb.append("  system_release        : " + system_release         + "\n");
		sb.append("  system_version        : " + system_version         + "\n");
		
		sb.append("  transport_supported   : ");
		StringUtil.appendObject(sb, transport_supported); 
		sb.append("\n");
		
		sb.append("  log_source            : " + log_source             + "\n");
		sb.append("  hc_state              : " + hc_state               + "\n");
		sb.append("  hc_fd                 : " + hc_fd                  + "\n");
		
		sb.append("  text_buf              : ");
		StringUtil.appendObject(sb, text_buf);
		sb.append("\n");
		
		sb.append("  components            : ");
		StringUtil.appendObject(sb, components);
		sb.append("\n");
		
		sb.append("  allowed_components    : ");
		StringUtil.appendObject(sb, allowed_components);
		sb.append("\n");
		
		sb.append("  all_components_allowed: " + all_components_allowed + "\n");
		sb.append("  n_active_components   : " + n_active_components    + "\n");
		sb.append("}");
		return sb.toString();
	}

}
