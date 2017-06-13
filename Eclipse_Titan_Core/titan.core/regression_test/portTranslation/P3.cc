/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Szabo, Bence Janos
 *
 ******************************************************************************/

#include "P3.hh"

namespace PortTranslation {

P3::P3(const char *par_port_name)
	: P3_BASE(par_port_name)
{

}

P3::~P3()
{

}

void P3::set_parameter(const char * /*parameter_name*/,
	const char * /*parameter_value*/)
{

}

/*void P3::Handle_Fd_Event(int fd, boolean is_readable,
	boolean is_writable, boolean is_error) {}*/

void P3::Handle_Fd_Event_Error(int /*fd*/)
{

}

void P3::Handle_Fd_Event_Writable(int /*fd*/)
{

}

void P3::Handle_Fd_Event_Readable(int /*fd*/)
{

}

/*void P3::Handle_Timeout(double time_since_last_call) {}*/

void P3::user_map(const char * /*system_port*/)
{

}

void P3::user_unmap(const char * /*system_port*/)
{

}

void P3::user_start()
{

}

void P3::user_stop()
{

}

void P3::outgoing_send(const MyRec& send_par)
{
	OCTETSTRING os = send_par.val();
	incoming_message(os);
}

void P3::outgoing_send(const BITSTRING& /*send_par*/)
{

}

void P3::outgoing_send(const OCTETSTRING& /*send_par*/)
{

}

void P3::outgoing_send(const HEXSTRING& /*send_par*/)
{

}

void P3::outgoing_send(const INTEGER& /*send_par*/)
{

}

} /* end of namespace */

