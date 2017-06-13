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

#include "PT2.hh"

namespace PortTranslation {

PT2::PT2(const char *par_port_name)
	: PT2_BASE(par_port_name)
{

}

PT2::~PT2()
{

}

void PT2::set_parameter(const char * /*parameter_name*/,
	const char * /*parameter_value*/)
{

}

/*void PT2::Handle_Fd_Event(int fd, boolean is_readable,
	boolean is_writable, boolean is_error) {}*/

void PT2::Handle_Fd_Event_Error(int /*fd*/)
{

}

void PT2::Handle_Fd_Event_Writable(int /*fd*/)
{

}

void PT2::Handle_Fd_Event_Readable(int /*fd*/)
{

}

/*void PT2::Handle_Timeout(double time_since_last_call) {}*/

void PT2::user_map(const char * /*system_port*/)
{

}

void PT2::user_unmap(const char * /*system_port*/)
{

}

void PT2::user_start()
{

}

void PT2::user_stop()
{

}

void PT2::outgoing_send(const MyRec& send_par)
{
	OCTETSTRING os = send_par.val();
	incoming_message(oct2char(os));
}

void PT2::outgoing_send(const OCTETSTRING& /*send_par*/)
{

}

void PT2::outgoing_send(const BITSTRING& /*send_par*/)
{

}

void PT2::outgoing_send(const HEXSTRING& /*send_par*/)
{

}

} /* end of namespace */

