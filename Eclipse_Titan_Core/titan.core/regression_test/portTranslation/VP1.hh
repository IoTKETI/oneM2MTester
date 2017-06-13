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

#ifndef VP1_HH
#define VP1_HH

#include <TTCN3.hh>

// Note: Header file PortVariables.hh must not be included into this file!
// (because it includes this file)
// Please add the declarations of message types manually.

namespace PortVariables {

typedef PreGenRecordOf::PREGEN__RECORD__OF__INTEGER RoI;

class VP1_PROVIDER : public PORT {
public:
	VP1_PROVIDER(const char *par_port_name);
	~VP1_PROVIDER();

	void set_parameter(const char *parameter_name,
		const char *parameter_value);

private:
	/* void Handle_Fd_Event(int fd, boolean is_readable,
		boolean is_writable, boolean is_error); */
	void Handle_Fd_Event_Error(int fd);
	void Handle_Fd_Event_Writable(int fd);
	void Handle_Fd_Event_Readable(int fd);
	/* void Handle_Timeout(double time_since_last_call); */
protected:
	void user_map(const char *system_port);
	void user_unmap(const char *system_port);

	void user_start();
	void user_stop();

public:
	void outgoing_send(const INTEGER& send_par);
	void outgoing_send(const CHARSTRING& send_par);
	void outgoing_send(const RoI& send_par);
	virtual void incoming_message(const INTEGER& incoming_par) = 0;
	virtual void incoming_message(const CHARSTRING& incoming_par) = 0;
	virtual void incoming_message(const RoI& incoming_par) = 0;
};

} /* end of namespace */

#endif
