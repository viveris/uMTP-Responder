/*
 * uMTP Responder
 * Copyright (c) 2018 - 2025 Viveris Technologies
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
 * @file   mtp_cfg.c
 * @brief  Configuration file parser.
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include "buildconf.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mtp.h"
#include "mtp_cfg.h"

#include "fs_handles_db.h"
#include "usbstring.h"

#include "default_cfg.h"

#include "logs_out.h"

typedef int (* KW_FUNC)(mtp_ctx * context, char * line, int cmd);

enum
{
	STORAGE_CMD = 0,
	USBVENDORID_CMD,
	USBPRODUCTID_CMD,
	USBCLASS_CMD,
	USBSUBCLASS_CMD,
	USBPROTOCOL_CMD,
	USBDEVVERSION_CMD,
	USBMAXPACKETSIZE_CMD,
	USBFUNCTIONFSMODE_CMD,
	USBMAXRDBUFFERSIZE_CMD,
	USBMAXWRBUFFERSIZE_CMD,
	READBUFFERSIZE_CMD,

	USB_DEV_PATH_CMD,
	USB_EPIN_PATH_CMD,
	USB_EPOUT_PATH_CMD,
	USB_EPINT_PATH_CMD,

	MANUFACTURER_STRING_CMD,
	PRODUCT_STRING_CMD,
	SERIAL_STRING_CMD,
	VERSION_STRING_CMD,
	MTP_EXTENSIONS_STRING_CMD,
	INTERFACE_STRING_CMD,

	WAIT_CONNECTION,
	LOOP_ON_DISCONNECT,

	SHOW_HIDDEN_FILES,
	UMASK,
	DEFAULT_UID_CMD,
	DEFAULT_GID_CMD,

	NO_INOTIFY,

	SYNC_WHEN_CLOSE

};

typedef struct kw_list_
{
	char * keyword;
	KW_FUNC func;
	int cmd;
}kw_list;

static int is_end_line(char c)
{
	if( c == 0 || c == '#' || c == '\r' || c == '\n' )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

static int is_space(char c)
{
	if( c == ' ' || c == '\t' )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

static int get_next_word(char * line, int offset)
{
	while( !is_end_line(line[offset]) && ( line[offset] == ' ' ) )
	{
		offset++;
	}

	return offset;
}

static int copy_param(char * dest, char * line, int offs)
{
	int i,insidequote;

	i = 0;
	insidequote = 0;
	while( !is_end_line(line[offs]) && ( insidequote || !is_space(line[offs]) ) && (i < (MAX_CFG_STRING_SIZE - 1)) )
	{
		if(line[offs] != '"')
		{
			if(dest)
				dest[i] = line[offs];

			i++;
		}
		else
		{
			if(insidequote)
				insidequote = 0;
			else
				insidequote = 1;
		}

		offs++;
	}

	if(dest)
		dest[i] = 0;

	return offs;
}

static int get_param_offset(char * line, int param)
{
	int param_cnt, offs;

	offs = 0;
	offs = get_next_word(line, offs);

	for (param_cnt = 0; param_cnt < param; param_cnt++)
	{
		offs = copy_param(NULL, line, offs);

		offs = get_next_word( line, offs );

		if(line[offs] == 0 || line[offs] == '#')
			return -1;
	}

	return offs;
}

static int get_param(char * line, int param_offset,char * param)
{
	int offs;

	offs = get_param_offset(line, param_offset);

	if(offs>=0)
	{
		offs = copy_param(param, line, offs);

		return 1;
	}

	return -1;
}

static int extract_cmd(char * line, char * command)
{
	int offs,i;

	i = 0;
	offs = 0;

	offs = get_next_word(line, offs);

	if( !is_end_line(line[offs]) )
	{
		while( !is_end_line(line[offs]) && !is_space(line[offs]) && i < (MAX_CFG_STRING_SIZE - 1) )
		{
			command[i] = line[offs];
			offs++;
			i++;
		}

		command[i] = 0;

		return i;
	}

	return 0;
}

int test_flag(char * str, char * flag, char * param)
{
	int i,j,flaglen;
	char previous_char;

	flaglen = strlen(flag);
	i = 0;
	previous_char = 0;
	while( str[i] )
	{
		if(!strncmp(&str[i],flag,strlen(flag)))
		{
			if( (previous_char == 0 || previous_char == ',') && \
				(str[i + flaglen] == 0 || str[i + flaglen] == ','  || str[i + flaglen] == '=') )
			{
				if(param && str[i + flaglen] == '=')
				{
					// get the parameter
					i = i + flaglen + 1;
					j = 0;
					while(str[i] && str[i] != ',' && str[i] != ' ' && j < (MAX_CFG_STRING_SIZE - 1))
					{
						param[j] = str[i];
						i++;
						j++;
					}
					param[j] = 0;
				}

				return 1;
			}
		}

		previous_char = str[i];

		i++;
	}

	return 0;
}

int mtp_remove_storage_from_line(mtp_ctx * context, char * name, int idx)
{
	char storagename[MAX_CFG_STRING_SIZE];
	int i;

	i = get_param(name, idx, storagename);
	if (i < 0)
		return i;

	PRINT_MSG("Remove storage %s", storagename);

	return mtp_remove_storage(context, storagename);
}

void mtp_add_storage_from_line(mtp_ctx * context, char * line, int idx)
{
	int i, j, k, uid, gid, print_uid, print_gid;
	char storagename[MAX_CFG_STRING_SIZE];
	char storagepath[MAX_CFG_STRING_SIZE];
	char options[MAX_CFG_STRING_SIZE];
	char tmpstr[MAX_CFG_STRING_SIZE];
	uint32_t flags;

	uid = -1;
	gid = -1;

	i = get_param(line, idx + 1,storagename);
	j = get_param(line, idx,storagepath);
	flags = UMTP_STORAGE_READWRITE;

	if( i >= 0 && j >= 0 )
	{
		k = get_param(line, idx + 2,options);
		if( k >= 0 )
		{
			if(test_flag(options, "ro",NULL))
			{
				flags |= UMTP_STORAGE_READONLY;
			}

			if(test_flag(options, "rw",NULL))
			{
				flags |= UMTP_STORAGE_READWRITE;
			}

			if(test_flag(options, "notmounted",NULL))
			{
				flags |= UMTP_STORAGE_NOTMOUNTED;
			}

			if(test_flag(options, "removable",NULL))
			{
				flags |= UMTP_STORAGE_REMOVABLE;
			}

			if(test_flag(options, "locked",NULL))
			{
				flags |= (UMTP_STORAGE_LOCKABLE | UMTP_STORAGE_LOCKED);
			}

			if(test_flag(options, "uid",tmpstr))
			{
				uid = atoi(tmpstr);
			}

			if(test_flag(options, "gid",tmpstr))
			{
				gid = atoi(tmpstr);
			}
		}

		if(uid == -1)
			print_uid = context->default_uid;
		else
			print_uid = uid;

		if(gid == -1)
			print_gid = context->default_gid;
		else
			print_gid = gid;

		PRINT_MSG("Add storage %s - Root Path: %s - UID: %d - GID: %d - Flags: 0x%.8X", storagename, storagepath, print_uid, print_gid, flags);

		mtp_add_storage(context, storagepath, storagename, uid, gid, flags);
	}
}

static int get_storage_params(mtp_ctx * context, char * line,int cmd)
{
	mtp_add_storage_from_line(context, line, 1);

	return 0;
}

static int get_hex_param(mtp_ctx * context, char * line,int cmd)
{
	int i;
	char tmp_txt[MAX_CFG_STRING_SIZE];
	unsigned long param_value;

	i = get_param(line, 1,tmp_txt);

	if( i >= 0 )
	{
		param_value = strtoul(tmp_txt,0,16);
		switch(cmd)
		{
			case USBVENDORID_CMD:
				if( param_value < 0x10000 )
				{
					context->usb_cfg.usb_vendor_id = param_value;
				}
				else
				{
					PRINT_MSG("Bad vendor id value !\n");
				}
			break;

			case USBPRODUCTID_CMD:
				if( param_value < 0x10000 )
				{
					context->usb_cfg.usb_product_id = param_value;
				}
				else
				{
					PRINT_MSG("Bad product id value !\n");
				}
			break;

			case USBCLASS_CMD:
				if( param_value < 256 )
				{
					context->usb_cfg.usb_class = param_value;
				}
				else
				{
					PRINT_MSG("Bad class value !\n");
				}
			break;

			case USBSUBCLASS_CMD:
				if( param_value < 256 )
				{
					context->usb_cfg.usb_subclass = param_value;
				}
				else
				{
					PRINT_MSG("Bad subclass value !\n");
				}
			break;

			case USBPROTOCOL_CMD:
				if( param_value < 256 )
				{
					context->usb_cfg.usb_protocol = param_value;
				}
				else
				{
					PRINT_MSG("Bad protocol value !\n");
				}
			break;

			case USBDEVVERSION_CMD:
				if( param_value < 0x10000 )
				{
					context->usb_cfg.usb_dev_version = param_value;
				}
				else
				{
					PRINT_MSG("Bad version value !\n");
				}
			break;

			case USBMAXPACKETSIZE_CMD:
				if( param_value < 0x10000)
				{
					context->usb_cfg.usb_max_packet_size = param_value;
				}
				else
				{
					PRINT_MSG("Bad max packet size value !\n");
					context->usb_cfg.usb_max_packet_size = MAX_PACKET_SIZE;
				}
			break;

			case USBMAXRDBUFFERSIZE_CMD:
				context->usb_rd_buffer_max_size = param_value & (~(512-1));
			break;

			case USBMAXWRBUFFERSIZE_CMD:
				context->usb_wr_buffer_max_size = param_value & (~(512-1));
			break;

			case READBUFFERSIZE_CMD:
				context->read_file_buffer_size = param_value;
			break;

			case USBFUNCTIONFSMODE_CMD:
				if( param_value )
					context->usb_cfg.usb_functionfs_mode = USB_FFS_MODE;
				else
					context->usb_cfg.usb_functionfs_mode = 0;
			break;

			case WAIT_CONNECTION:
				context->usb_cfg.wait_connection = param_value;
			break;

			case LOOP_ON_DISCONNECT:
				context->usb_cfg.loop_on_disconnect = param_value;
			break;

			case SHOW_HIDDEN_FILES:
				context->usb_cfg.show_hidden_files = param_value;
			break;

			case NO_INOTIFY:
				context->no_inotify = param_value;
			break;

			case SYNC_WHEN_CLOSE:
				context->sync_when_close = param_value;

		}
	}

	return 0;
}

static int get_str_param(mtp_ctx * context, char * line,int cmd)
{
	int i;
	char tmp_txt[MAX_CFG_STRING_SIZE];

	i = get_param(line, 1,tmp_txt);

	if( i >= 0 )
	{
		switch(cmd)
		{
			case USB_DEV_PATH_CMD:
				strncpy(context->usb_cfg.usb_device_path,tmp_txt,MAX_CFG_STRING_SIZE);
			break;

			case USB_EPIN_PATH_CMD:
				strncpy(context->usb_cfg.usb_endpoint_in,tmp_txt,MAX_CFG_STRING_SIZE);
			break;

			case USB_EPOUT_PATH_CMD:
				strncpy(context->usb_cfg.usb_endpoint_out,tmp_txt,MAX_CFG_STRING_SIZE);
			break;

			case USB_EPINT_PATH_CMD:
				strncpy(context->usb_cfg.usb_endpoint_intin,tmp_txt,MAX_CFG_STRING_SIZE);
			break;

			case MANUFACTURER_STRING_CMD:
				strncpy(context->usb_cfg.usb_string_manufacturer,tmp_txt,MAX_CFG_STRING_SIZE);
			break;

			case PRODUCT_STRING_CMD:
				strncpy(context->usb_cfg.usb_string_product,tmp_txt,MAX_CFG_STRING_SIZE);
			break;

			case SERIAL_STRING_CMD:
				strncpy(context->usb_cfg.usb_string_serial,tmp_txt,MAX_CFG_STRING_SIZE);
			break;

			case VERSION_STRING_CMD:
				strncpy(context->usb_cfg.usb_string_version,tmp_txt,MAX_CFG_STRING_SIZE);
			break;

			case MTP_EXTENSIONS_STRING_CMD:
				strncpy(context->usb_cfg.usb_string_mtp_extensions,tmp_txt,MAX_CFG_STRING_SIZE);
			break;

			case INTERFACE_STRING_CMD:
				strncpy(context->usb_cfg.usb_string_interface,tmp_txt,MAX_CFG_STRING_SIZE);
			break;
		}
	}

	return 0;
}

static int get_oct_param(mtp_ctx * context, char * line,int cmd)
{
	int i;
	char tmp_txt[MAX_CFG_STRING_SIZE];
	unsigned long param_value;

	i = get_param(line, 1, tmp_txt);

	if (i >= 0)
	{
		param_value = strtoul(tmp_txt, 0, 8);
		switch (cmd)
		{
			case UMASK:
				context->usb_cfg.val_umask = param_value;
			break;
		}
	}
	return 0;
}

static int get_dec_param(mtp_ctx * context, char * line,int cmd)
{
	int i;
	char tmp_txt[MAX_CFG_STRING_SIZE];
	unsigned long param_value;

	i = get_param(line, 1, tmp_txt);

	if (i >= 0)
	{
		param_value = strtoul(tmp_txt, 0, 10);
		switch (cmd)
		{
			case DEFAULT_UID_CMD:
				context->default_uid = param_value;
			break;
			case DEFAULT_GID_CMD:
				context->default_gid = param_value;
			break;
		}
	}
	return 0;
}

kw_list kwlist[] =
{
	{"storage",                get_storage_params, STORAGE_CMD},
	{"usb_vendor_id",          get_hex_param,   USBVENDORID_CMD},
	{"usb_product_id",         get_hex_param,   USBPRODUCTID_CMD},
	{"usb_class",              get_hex_param,   USBCLASS_CMD},
	{"usb_subclass",           get_hex_param,   USBSUBCLASS_CMD},
	{"usb_protocol",           get_hex_param,   USBPROTOCOL_CMD},
	{"usb_dev_version",        get_hex_param,   USBDEVVERSION_CMD},
	{"usb_max_packet_size",    get_hex_param,   USBMAXPACKETSIZE_CMD},
	{"usb_max_rd_buffer_size", get_hex_param,   USBMAXRDBUFFERSIZE_CMD},
	{"usb_max_wr_buffer_size", get_hex_param,   USBMAXWRBUFFERSIZE_CMD},
	{"read_buffer_cache_size", get_hex_param,   READBUFFERSIZE_CMD},

	{"usb_functionfs_mode",    get_hex_param,   USBFUNCTIONFSMODE_CMD},

	{"usb_dev_path",           get_str_param,   USB_DEV_PATH_CMD},
	{"usb_epin_path",          get_str_param,   USB_EPIN_PATH_CMD},
	{"usb_epout_path",         get_str_param,   USB_EPOUT_PATH_CMD},
	{"usb_epint_path",         get_str_param,   USB_EPINT_PATH_CMD},
	{"manufacturer",           get_str_param,   MANUFACTURER_STRING_CMD},
	{"product",                get_str_param,   PRODUCT_STRING_CMD},
	{"serial",                 get_str_param,   SERIAL_STRING_CMD},
	{"firmware_version",       get_str_param,   VERSION_STRING_CMD},
	{"interface",              get_str_param,   INTERFACE_STRING_CMD},
	{"mtp_extensions",         get_str_param,   MTP_EXTENSIONS_STRING_CMD},

	{"wait",                   get_hex_param,   WAIT_CONNECTION},
	{"loop_on_disconnect",     get_hex_param,   LOOP_ON_DISCONNECT},

	{"show_hidden_files",      get_hex_param,   SHOW_HIDDEN_FILES},
	{"umask",                  get_oct_param,   UMASK},
	{"default_uid",            get_dec_param,   DEFAULT_UID_CMD},
	{"default_gid",            get_dec_param,   DEFAULT_GID_CMD},

	{"no_inotify",             get_hex_param,   NO_INOTIFY},

	{"sync_when_close",        get_hex_param,   SYNC_WHEN_CLOSE},

	{ 0, 0, 0 }
};

static int exec_cmd(mtp_ctx * context, char * command,char * line)
{
	int i;

	i = 0;
	while(kwlist[i].func)
	{
		if( !strcmp(kwlist[i].keyword,command) )
		{
			kwlist[i].func(context, line, kwlist[i].cmd);
			return 1;
		}

		i++;
	}

	return 0;
}

int execute_line(mtp_ctx * context,char * line)
{
	char command[MAX_CFG_STRING_SIZE];

	command[0] = 0;

	if( extract_cmd(line, command) )
	{
		if(strlen(command))
		{
			if( !exec_cmd(context, command,line))
			{
				PRINT_ERROR("Line syntax error : %s",line);

				return 0;
			}
		}

		return 1;
	}

	return 0;
}

int mtp_load_config_file(mtp_ctx * context, const char * conffile)
{
	int err = 0;
	FILE * f;
	char line[MAX_CFG_STRING_SIZE];

	memset((void*)&context->usb_cfg,0x00, sizeof(mtp_usb_cfg));

	// Set default config
	strncpy(context->usb_cfg.usb_device_path,         USB_DEV,                MAX_CFG_STRING_SIZE);
	strncpy(context->usb_cfg.usb_endpoint_in,         USB_EPIN,               MAX_CFG_STRING_SIZE);
	strncpy(context->usb_cfg.usb_endpoint_out,        USB_EPOUT,              MAX_CFG_STRING_SIZE);
	strncpy(context->usb_cfg.usb_endpoint_intin,      USB_EPINTIN,            MAX_CFG_STRING_SIZE);
	strncpy(context->usb_cfg.usb_string_manufacturer, MANUFACTURER,           MAX_CFG_STRING_SIZE);
	strncpy(context->usb_cfg.usb_string_product,      PRODUCT,                MAX_CFG_STRING_SIZE);
	strncpy(context->usb_cfg.usb_string_serial,       SERIALNUMBER,           MAX_CFG_STRING_SIZE);
	strncpy(context->usb_cfg.usb_string_version,      "Rev A",                MAX_CFG_STRING_SIZE);
	strncpy(context->usb_cfg.usb_string_mtp_extensions, "",                   MAX_CFG_STRING_SIZE);
	strncpy(context->usb_cfg.usb_string_interface,    "MTP",                  MAX_CFG_STRING_SIZE);

	context->usb_cfg.usb_vendor_id       = USB_DEV_VENDOR_ID;
	context->usb_cfg.usb_product_id      = USB_DEV_PRODUCT_ID;
	context->usb_cfg.usb_class           = USB_DEV_CLASS;
	context->usb_cfg.usb_subclass        = USB_DEV_SUBCLASS;
	context->usb_cfg.usb_protocol        = USB_DEV_PROTOCOL;
	context->usb_cfg.usb_dev_version     = USB_DEV_VERSION;
	context->usb_cfg.usb_max_packet_size = MAX_PACKET_SIZE;
	context->usb_cfg.usb_functionfs_mode = USB_FFS_MODE;

	context->usb_cfg.wait_connection = 0;
	context->usb_cfg.loop_on_disconnect = 0;

	context->usb_cfg.show_hidden_files = 1;

	context->usb_cfg.val_umask = -1;

	context->default_gid = -1;
	context->default_uid = -1;

	context->no_inotify = 0;
	context->sync_when_close = 0;

	f = fopen(conffile, "r");
	if(f)
	{
		do
		{
			if(!fgets(line,sizeof(line),f))
				break;

			if(feof(f))
				break;

			execute_line(context, line);
		}while(1);

		fclose(f);
	}
	else
	{
		PRINT_ERROR("Can't open %s ! Using default settings...", conffile);
	}

	PRINT_MSG("USB Device path : %s",context->usb_cfg.usb_device_path);
	PRINT_MSG("USB In End point path : %s",context->usb_cfg.usb_endpoint_in);
	PRINT_MSG("USB Out End point path : %s",context->usb_cfg.usb_endpoint_out);
	PRINT_MSG("USB Event End point path : %s",context->usb_cfg.usb_endpoint_intin);
	PRINT_MSG("USB Max packet size : 0x%X bytes",context->usb_cfg.usb_max_packet_size);
	PRINT_MSG("USB Max write buffer size : 0x%X bytes",context->usb_wr_buffer_max_size);
	PRINT_MSG("USB Max read buffer size : 0x%X bytes",context->usb_rd_buffer_max_size);
	PRINT_MSG("Read file buffer size : 0x%X bytes",context->read_file_buffer_size);

	PRINT_MSG("Manufacturer string : %s",context->usb_cfg.usb_string_manufacturer);
	PRINT_MSG("Product string : %s",context->usb_cfg.usb_string_product);
	PRINT_MSG("Serial string : %s",context->usb_cfg.usb_string_serial);
	PRINT_MSG("Firmware Version string : %s", context->usb_cfg.usb_string_version);
	PRINT_MSG("MTP Exstensions string : %s", context->usb_cfg.usb_string_mtp_extensions);
	PRINT_MSG("Interface string : %s",context->usb_cfg.usb_string_interface);

	PRINT_MSG("USB Vendor ID : 0x%.4X",context->usb_cfg.usb_vendor_id);
	PRINT_MSG("USB Product ID : 0x%.4X",context->usb_cfg.usb_product_id);
	PRINT_MSG("USB class ID : 0x%.2X",context->usb_cfg.usb_class);
	PRINT_MSG("USB subclass ID : 0x%.2X",context->usb_cfg.usb_subclass);
	PRINT_MSG("USB Protocol ID : 0x%.2X",context->usb_cfg.usb_protocol);
	PRINT_MSG("USB Device version : 0x%.4X",context->usb_cfg.usb_dev_version);

	if(context->usb_cfg.usb_functionfs_mode)
	{
		PRINT_MSG("USB FunctionFS Mode");
	}
	else
	{
		PRINT_MSG("USB GadgetFS Mode");
	}

	PRINT_MSG("Wait for connection : %i",context->usb_cfg.wait_connection);
	PRINT_MSG("Loop on disconnect : %i",context->usb_cfg.loop_on_disconnect);
	PRINT_MSG("Show hidden files : %i",context->usb_cfg.show_hidden_files);
	if(context->usb_cfg.val_umask >= 0)
	{
		umask(context->usb_cfg.val_umask);
		PRINT_MSG("File creation umask : %03o",context->usb_cfg.val_umask);
	}
	else
	{
		PRINT_MSG("File creation umask : System default umask");
	}

	if(context->default_uid != -1)
	{
		PRINT_MSG("Default UID : %d",context->default_uid);
	}
	else
	{
		PRINT_MSG("Default UID : %d",geteuid());
	}

	if(context->default_gid != -1)
	{
		PRINT_MSG("Default GID : %d",context->default_gid);
	}
	else
	{
		PRINT_MSG("Default GID : %d",getegid());
	}

	PRINT_MSG("inotify : %s",context->no_inotify?"no":"yes");

	PRINT_MSG("Sync when close : %s",context->sync_when_close?"yes":"no");

	return err;
}
