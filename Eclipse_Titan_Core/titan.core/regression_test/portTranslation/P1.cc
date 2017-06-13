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

#include "P1.hh"
#include "PortTranslation.hh"

namespace PortTranslation {

P1_PROVIDER::P1_PROVIDER(const char *par_port_name)
	: PORT(par_port_name)
{

}

P1_PROVIDER::~P1_PROVIDER()
{

}

void P1_PROVIDER::set_parameter(const char * /*parameter_name*/,
	const char * /*parameter_value*/)
{

}

/*void P1_PROVIDER::Handle_Fd_Event(int fd, boolean is_readable,
	boolean is_writable, boolean is_error) {}*/

void P1_PROVIDER::Handle_Fd_Event_Error(int /*fd*/)
{

}

void P1_PROVIDER::Handle_Fd_Event_Writable(int /*fd*/)
{

}

void P1_PROVIDER::Handle_Fd_Event_Readable(int /*fd*/)
{

}

/*void P1_PROVIDER::Handle_Timeout(double time_since_last_call) {}*/

void P1_PROVIDER::user_map(const char * /*system_port*/)
{

}

void P1_PROVIDER::user_unmap(const char * /*system_port*/)
{

}

void P1_PROVIDER::user_start()
{

}

void P1_PROVIDER::user_stop()
{

}

void P1_PROVIDER::outgoing_send(const MyRec& send_par)
{
	OCTETSTRING os = send_par.val();
	incoming_message(os);
}

void P1_PROVIDER::outgoing_send(const OCTETSTRING& send_par)
{
	INTEGER integer = send_par.lengthof();
	incoming_message(integer);
}

void P1_PROVIDER::outgoing_send(const BITSTRING& send_par)
{
	// Test that the receive mapping handles fragmented case
	if (send_par.lengthof() == 48) {
		for (int i = 0; i < 48; i++) {
			BITSTRING bs = send_par[i];
			incoming_message(bs);
		}
	} else {
		incoming_message(send_par);
	}
}

void P1_PROVIDER::outgoing_send(const CHARSTRING& send_par)
{
	incoming_message(send_par);
}

void P1_PROVIDER::outgoing_send(const INTEGER& send_par)
{
	incoming_message(send_par);
}

void P1_PROVIDER::outgoing_send(const HEXSTRING& send_par)
{
	incoming_message(send_par);
}

} /* end of namespace */

