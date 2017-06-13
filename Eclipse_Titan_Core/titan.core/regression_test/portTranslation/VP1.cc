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

#include "VP1.hh"
#include "PortVariables.hh"

namespace PortVariables {

VP1_PROVIDER::VP1_PROVIDER(const char *par_port_name)
	: PORT(par_port_name)
{

}

VP1_PROVIDER::~VP1_PROVIDER()
{

}

void VP1_PROVIDER::set_parameter(const char * /*parameter_name*/,
	const char * /*parameter_value*/)
{

}

/*void VP1_PROVIDER::Handle_Fd_Event(int fd, boolean is_readable,
	boolean is_writable, boolean is_error) {}*/

void VP1_PROVIDER::Handle_Fd_Event_Error(int /*fd*/)
{

}	

void VP1_PROVIDER::Handle_Fd_Event_Writable(int /*fd*/)
{

}

void VP1_PROVIDER::Handle_Fd_Event_Readable(int /*fd*/)
{

}

/*void VP1_PROVIDER::Handle_Timeout(double time_since_last_call) {}*/

void VP1_PROVIDER::user_map(const char * /*system_port*/)
{

}

void VP1_PROVIDER::user_unmap(const char * /*system_port*/)
{

}

void VP1_PROVIDER::user_start()
{

}

void VP1_PROVIDER::user_stop()
{

}

void VP1_PROVIDER::outgoing_send(const INTEGER& send_par)
{
	incoming_message(send_par);
}

void VP1_PROVIDER::outgoing_send(const CHARSTRING& send_par)
{
	incoming_message(send_par);
}

void VP1_PROVIDER::outgoing_send(const RoI& send_par)
{
	incoming_message(send_par);
}

} /* end of namespace */

