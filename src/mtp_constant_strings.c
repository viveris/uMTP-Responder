/*
 * uMTP Responder
 * Copyright (c) 2018 - 2019 Viveris Technologies
 *
 * uMTP Responder is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * uMTP Responder is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with uMTP Responder; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * @file   mtp_constant_strings.c
 * @brief  MTP Codes to string decoder (Debug purpose)
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

#include <errno.h>

#include "fs_handles_db.h"
#include "mtp.h"
#include "mtp_constant.h"

#include "mtp_constant_strings.h"

const char DevInfos_MTP_Extensions[] = "microsoft.com: 1.0; android.com: 1.0;";

typedef struct _opcodestring
{
	const char * str;
	uint16_t operation;
}opcodestring;

typedef struct _packettypestring
{
	const char * str;
	uint16_t type;
}packettypestring;

const opcodestring op_codes[]=
{
	{ "MTP_OPERATION_GET_DEVICE_INFO",            0x1001 },
	{ "MTP_OPERATION_OPEN_SESSION",               0x1002 },
	{ "MTP_OPERATION_CLOSE_SESSION",              0x1003 },
	{ "MTP_OPERATION_GET_STORAGE_IDS",            0x1004 },
	{ "MTP_OPERATION_GET_STORAGE_INFO",           0x1005 },
	{ "MTP_OPERATION_GET_NUM_OBJECTS",            0x1006 },
	{ "MTP_OPERATION_GET_OBJECT_HANDLES",         0x1007 },
	{ "MTP_OPERATION_GET_OBJECT_INFO",            0x1008 },
	{ "MTP_OPERATION_GET_OBJECT",                 0x1009 },
	{ "MTP_OPERATION_GET_THUMB",                  0x100A },
	{ "MTP_OPERATION_DELETE_OBJECT",              0x100B },
	{ "MTP_OPERATION_SEND_OBJECT_INFO",           0x100C },
	{ "MTP_OPERATION_SEND_OBJECT",                0x100D },
	{ "MTP_OPERATION_INITIATE_CAPTURE",           0x100E },
	{ "MTP_OPERATION_FORMAT_STORE",               0x100F },
	{ "MTP_OPERATION_RESET_DEVICE",               0x1010 },
	{ "MTP_OPERATION_SELF_TEST",                  0x1011 },
	{ "MTP_OPERATION_SET_OBJECT_PROTECTION",      0x1012 },
	{ "MTP_OPERATION_POWER_DOWN",                 0x1013 },
	{ "MTP_OPERATION_GET_DEVICE_PROP_DESC",       0x1014 },
	{ "MTP_OPERATION_GET_DEVICE_PROP_VALUE",      0x1015 },
	{ "MTP_OPERATION_SET_DEVICE_PROP_VALUE",      0x1016 },
	{ "MTP_OPERATION_RESET_DEVICE_PROP_VALUE",    0x1017 },
	{ "MTP_OPERATION_TERMINATE_OPEN_CAPTURE",     0x1018 },
	{ "MTP_OPERATION_MOVE_OBJECT",                0x1019 },
	{ "MTP_OPERATION_COPY_OBJECT",                0x101A },
	{ "MTP_OPERATION_GET_PARTIAL_OBJECT   ",      0x101B },
	{ "MTP_OPERATION_INITIATE_OPEN_CAPTURE",      0x101C },
	{ "MTP_OPERATION_GET_OBJECT_PROPS_SUPPORTED", 0x9801 },
	{ "MTP_OPERATION_GET_OBJECT_PROP_DESC ",      0x9802 },
	{ "MTP_OPERATION_GET_OBJECT_PROP_VALUE",      0x9803 },
	{ "MTP_OPERATION_SET_OBJECT_PROP_VALUE",      0x9804 },
	{ "MTP_OPERATION_GET_OBJECT_PROP_LIST ",      0x9805 },
	{ "MTP_OPERATION_SET_OBJECT_PROP_LIST ",      0x9806 },
	{ "MTP_OPERATION_GET_INTERDEPENDENT_PROP_DESC", 0x9807 },
	{ "MTP_OPERATION_SEND_OBJECT_PROP_LIST",      0x9808 },
	{ "MTP_OPERATION_GET_OBJECT_REFERENCES",      0x9810 },
	{ "MTP_OPERATION_SET_OBJECT_REFERENCES",      0x9811 },
	{ "MTP_OPERATION_SKIP",                       0x9820 },
	{ 0, 0 }
};

const packettypestring packet_types[]=
{
	{ "UNDEFINED",   0},
	{ "OPERATION",   1},
	{ "DATA",        2},
	{ "RESPONSE",    3},
	{ "EVENT",       4},
	{ 0, 0 }
};

const char * mtp_get_operation_string(uint16_t operation)
{
	int i;
	const char * strptr;

	strptr = "????";

	i = 0;
	while( op_codes[i].str && op_codes[i].operation != operation )
	{
		i++;
	}

	if(op_codes[i].str)
		strptr = op_codes[i].str;

	return strptr;
}

const char * mtp_get_type_string(uint16_t type)
{
	int i;
	const char * strptr;

	strptr = "????";

	i = 0;
	while( packet_types[i].str && packet_types[i].type != type )
	{
		i++;
	}

	if(op_codes[i].str)
		strptr = packet_types[i].str;

	return strptr;
}
