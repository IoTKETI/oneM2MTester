/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Baranyi, Botond
 *   Forstner, Matyas
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef MESSAGE_TYPES_HH
#define MESSAGE_TYPES_HH

/* Any relation - any direction */

#define MSG_ERROR		0

/* Any relation - to MC (up) */

#define MSG_LOG			1

/* First messages - to MC (up) */

/* from HCs */
#define MSG_VERSION		2
/* from MTC */
#define MSG_MTC_CREATED		3
/* from PTCs */
#define MSG_PTC_CREATED		4

/* Messages from MC to HC (down) */

#define MSG_CREATE_MTC		2
#define MSG_CREATE_PTC		3
#define MSG_KILL_PROCESS	4
#define MSG_EXIT_HC		5

/* Messages from HC to MC (up) */

#define MSG_CREATE_NAK		4
#define MSG_HC_READY		5

/* Messages from MC to TC (down) */

#define MSG_CREATE_ACK		1
#define MSG_START_ACK		2
#define MSG_STOP		3
#define MSG_STOP_ACK		4
#define MSG_KILL_ACK		5
#define MSG_RUNNING		6
#define MSG_ALIVE		7
#define MSG_DONE_ACK		8
#define MSG_KILLED_ACK		9
#define MSG_CANCEL_DONE		10
#define MSG_COMPONENT_STATUS	11
#define MSG_CONNECT_LISTEN	12
#define MSG_CONNECT		13
#define MSG_CONNECT_ACK		14
#define MSG_DISCONNECT		15
#define MSG_DISCONNECT_ACK	16
#define MSG_MAP			17
#define MSG_MAP_ACK		18
#define MSG_UNMAP		19
#define MSG_UNMAP_ACK		20

/* Messages from MC to MTC (down) */

#define MSG_EXECUTE_CONTROL	21
#define MSG_EXECUTE_TESTCASE	22
#define MSG_PTC_VERDICT		23
#define MSG_CONTINUE		24
#define MSG_EXIT_MTC		25

/* Messages from MC to PTC (down) */

#define MSG_START		21
#define MSG_KILL		22

/* Messages from TC to MC (up) */

#define MSG_CREATE_REQ		2
#define MSG_START_REQ		3
#define MSG_STOP_REQ		4
#define MSG_KILL_REQ		5
#define MSG_IS_RUNNING		6
#define MSG_IS_ALIVE		7
#define MSG_DONE_REQ		8
#define MSG_KILLED_REQ		9
#define MSG_CANCEL_DONE_ACK	10
#define MSG_CONNECT_REQ		11
#define MSG_CONNECT_LISTEN_ACK	12
#define MSG_CONNECTED		13
#define MSG_CONNECT_ERROR	14
#define MSG_DISCONNECT_REQ	15
#define MSG_DISCONNECTED	16
#define MSG_MAP_REQ		17
#define MSG_MAPPED		18
#define MSG_UNMAP_REQ		19
#define MSG_UNMAPPED		20
#define MSG_DEBUG_HALT_REQ	101
#define MSG_DEBUG_CONTINUE_REQ	102
#define MSG_DEBUG_BATCH		103

/* Messages from MTC to MC (up) */

#define MSG_TESTCASE_STARTED	21
#define MSG_TESTCASE_FINISHED	22
#define MSG_MTC_READY		23

/* Messages from PTC to MC (up) */

#define MSG_STOPPED		21
#define MSG_STOPPED_KILLED	22
#define MSG_KILLED		23

/* Messages from MC to HC or TC (down) */

#define MSG_DEBUG_COMMAND	100

/* Messages from HC or TC to MC (up) */

#define MSG_DEBUG_RETURN_VALUE	100

/* Messages from MC to HC or MTC (down) */

#define MSG_CONFIGURE		200

/* Messages from HC or MTC to MC (up) */

#define MSG_CONFIGURE_ACK	200
#define MSG_CONFIGURE_NAK	201

#endif
