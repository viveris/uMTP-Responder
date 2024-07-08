/*
 * uMTP Responder
 * Copyright (c) 2018 - 2024 Viveris Technologies
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

#include "buildconf.h"

#include "mtp_constant.h"

const unsigned short supported_op[]=
{
	MTP_OPERATION_GET_DEVICE_INFO                        ,//0x1001
	MTP_OPERATION_OPEN_SESSION                           ,//0x1002
	MTP_OPERATION_CLOSE_SESSION                          ,//0x1003
	MTP_OPERATION_GET_STORAGE_IDS                        ,//0x1004
	MTP_OPERATION_GET_STORAGE_INFO                       ,//0x1005
	//MTP_OPERATION_GET_NUM_OBJECTS                        ,//0x1006
	MTP_OPERATION_GET_OBJECT_HANDLES                     ,//0x1007
	MTP_OPERATION_GET_OBJECT_INFO                        ,//0x1008
	MTP_OPERATION_GET_OBJECT                             ,//0x1009
	//MTP_OPERATION_GET_THUMB                              ,//0x100A
	MTP_OPERATION_DELETE_OBJECT                          ,//0x100B
	MTP_OPERATION_SEND_OBJECT_INFO                       ,//0x100C
	MTP_OPERATION_SEND_OBJECT                            ,//0x100D
	MTP_OPERATION_GET_DEVICE_PROP_DESC                   ,//0x1014
	MTP_OPERATION_GET_DEVICE_PROP_VALUE                  ,//0x1015
	MTP_OPERATION_SET_DEVICE_PROP_VALUE                  ,//0x1016
	//MTP_OPERATION_RESET_DEVICE_PROP_VALUE                ,//0x1017
	MTP_OPERATION_GET_PARTIAL_OBJECT                     ,//0x101B
	MTP_OPERATION_GET_OBJECT_PROPS_SUPPORTED             ,//0x9801
	MTP_OPERATION_GET_OBJECT_PROP_DESC                   ,//0x9802
	MTP_OPERATION_GET_OBJECT_PROP_VALUE                  ,//0x9803
	MTP_OPERATION_SET_OBJECT_PROP_VALUE                  ,//0x9804
	MTP_OPERATION_GET_OBJECT_PROP_LIST                   ,//0x9805
	//MTP_OPERATION_GET_OBJECT_REFERENCES                  ,//0x9810
	//MTP_OPERATION_SET_OBJECT_REFERENCES                  ,//0x9811
	MTP_OPERATION_GET_PARTIAL_OBJECT_64                  ,//0x95C1
	MTP_OPERATION_SEND_PARTIAL_OBJECT                    ,//0x95C2
	MTP_OPERATION_TRUNCATE_OBJECT                        ,//0x95C3
	MTP_OPERATION_BEGIN_EDIT_OBJECT                      ,//0x95C4
	MTP_OPERATION_END_EDIT_OBJECT                         //0x95C5
};

const int supported_op_size=sizeof(supported_op);

const unsigned short supported_event[]=
{
	MTP_EVENT_OBJECT_ADDED               ,       // 0x4002
	MTP_EVENT_OBJECT_REMOVED             ,       // 0x4003
	MTP_EVENT_STORE_ADDED                ,       // 0x4004
	MTP_EVENT_STORE_REMOVED              ,       // 0x4005
	MTP_EVENT_STORAGE_INFO_CHANGED       ,       // 0x400C
	MTP_EVENT_OBJECT_INFO_CHANGED        ,       // 0x4007
	MTP_EVENT_DEVICE_PROP_CHANGED        ,       // 0x4006
	MTP_EVENT_OBJECT_PROP_CHANGED                // 0xC801
};

const int supported_event_size=sizeof(supported_event);
