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
 * @file   mtp_support_def.c
 * @brief  MTP support definitions.
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include "mtp_constant.h"

const unsigned short supported_op[]=
{
	MTP_OPERATION_GET_DEVICE_INFO                        ,//0x1001
	MTP_OPERATION_OPEN_SESSION                           ,//0x1002
	MTP_OPERATION_CLOSE_SESSION                          ,//0x1003
	MTP_OPERATION_GET_STORAGE_IDS                        ,//0x1004
	MTP_OPERATION_GET_STORAGE_INFO                       ,//0x1005
	MTP_OPERATION_GET_NUM_OBJECTS                        ,//0x1006
	MTP_OPERATION_GET_OBJECT_HANDLES                     ,//0x1007
	MTP_OPERATION_GET_OBJECT_INFO                        ,//0x1008
	MTP_OPERATION_GET_OBJECT                             ,//0x1009
	//MTP_OPERATION_GET_THUMB                            ,//0x100A
	MTP_OPERATION_DELETE_OBJECT                          ,//0x100B
	MTP_OPERATION_SEND_OBJECT_INFO                       ,//0x100C
	MTP_OPERATION_SEND_OBJECT                            ,//0x100D
	// MTP_OPERATION_INITIATE_CAPTURE                    ,//0x100E
	// MTP_OPERATION_FORMAT_STORE                        ,//0x100F
	MTP_OPERATION_RESET_DEVICE                           ,//0x1010
	MTP_OPERATION_SELF_TEST                              ,//0x1011
	MTP_OPERATION_SET_OBJECT_PROTECTION                  ,//0x1012
	MTP_OPERATION_POWER_DOWN                             ,//0x1013
	//MTP_OPERATION_GET_DEVICE_PROP_DESC                 ,//0x1014
	//MTP_OPERATION_GET_DEVICE_PROP_VALUE                ,//0x1015
	//MTP_OPERATION_SET_DEVICE_PROP_VALUE                ,//0x1016
	//MTP_OPERATION_RESET_DEVICE_PROP_VALUE              ,//0x1017
	//MTP_OPERATION_TERMINATE_OPEN_CAPTURE               ,//0x1018
	MTP_OPERATION_MOVE_OBJECT                            ,//0x1019
	MTP_OPERATION_COPY_OBJECT                            ,//0x101A
	MTP_OPERATION_GET_PARTIAL_OBJECT                     ,//0x101B
	MTP_OPERATION_INITIATE_OPEN_CAPTURE                  ,//0x101C
	/* MTP_OPERATION_GET_OBJECT_PROPS_SUPPORTED          ,//0x9801
	MTP_OPERATION_GET_OBJECT_PROP_DESC                   ,//0x9802
	MTP_OPERATION_GET_OBJECT_PROP_VALUE                  ,//0x9803
	MTP_OPERATION_SET_OBJECT_PROP_VALUE                  ,//0x9804
	MTP_OPERATION_GET_OBJECT_PROP_LIST                   ,//0x9805
	MTP_OPERATION_SET_OBJECT_PROP_LIST                   ,//0x9806
	MTP_OPERATION_GET_INTERDEPENDENT_PROP_DESC           ,//0x9807
	MTP_OPERATION_SEND_OBJECT_PROP_LIST                  ,//0x9808
	MTP_OPERATION_GET_OBJECT_REFERENCES                  ,//0x9810
	MTP_OPERATION_SET_OBJECT_REFERENCES                  ,//0x9811*/
	MTP_OPERATION_SKIP                                   ,//0x9820
};

const int supported_op_size=sizeof(supported_op);

const unsigned short supported_event[]=
{
	MTP_EVENT_UNDEFINED                  ,       // 0x4000
	MTP_EVENT_CANCEL_TRANSACTION         ,       // 0x4001
	MTP_EVENT_OBJECT_ADDED               ,       // 0x4002
	MTP_EVENT_OBJECT_REMOVED             ,       // 0x4003
	MTP_EVENT_STORE_ADDED                ,       // 0x4004
	MTP_EVENT_STORE_REMOVED              ,       // 0x4005
	MTP_EVENT_DEVICE_PROP_CHANGED        ,       // 0x4006
	MTP_EVENT_OBJECT_INFO_CHANGED        ,       // 0x4007
	MTP_EVENT_DEVICE_INFO_CHANGED        ,       // 0x4008
	MTP_EVENT_REQUEST_OBJECT_TRANSFER    ,       // 0x4009
	MTP_EVENT_STORE_FULL                 ,       // 0x400A
	MTP_EVENT_DEVICE_RESET               ,       // 0x400B
	MTP_EVENT_STORAGE_INFO_CHANGED       ,       // 0x400C
	MTP_EVENT_CAPTURE_COMPLETE           ,       // 0x400D
	MTP_EVENT_UNREPORTED_STATUS          ,       // 0x400E
	MTP_EVENT_OBJECT_PROP_CHANGED        ,       // 0xC801
	MTP_EVENT_OBJECT_PROP_DESC_CHANGED   ,       // 0xC802
	MTP_EVENT_OBJECT_REFERENCES_CHANGED  ,       // 0xC803
};

const int supported_event_size=sizeof(supported_event);

const unsigned short supported_property[]=
{
	MTP_DEVICE_PROPERTY_UNDEFINED    //  ,         // 0x5000
	//MTP_DEVICE_PROPERTY_BATTERY_LEVEL            // 0x5001
};

const int supported_property_size=sizeof(supported_property);

const unsigned short supported_formats[]=
{
	MTP_FORMAT_UNDEFINED                     , //    0x3000   // Undefined object
	MTP_FORMAT_ASSOCIATION                     //    0x3001   // Association (for example, a folder)
};

const int supported_formats_size=sizeof(supported_formats);
