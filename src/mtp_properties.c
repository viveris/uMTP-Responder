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
 * @file   mtp_properties.c
 * @brief  MTP properties datasets helpers
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/statvfs.h>

#include <time.h>

#include "logs_out.h"

#include "mtp_helpers.h"

#include "fs_handles_db.h"

#include "mtp.h"
#include "mtp_datasets.h"
#include "mtp_properties.h"

#include "usb_gadget_fct.h"

#include "mtp_constant.h"
#include "mtp_constant_strings.h"

#include "mtp_support_def.h"

uint16_t supported_properties[]=
{
	/*MTP_PROPERTY_STORAGE_ID,
	MTP_PROPERTY_OBJECT_FORMAT,
	MTP_PROPERTY_PROTECTION_STATUS,
	MTP_PROPERTY_OBJECT_SIZE,
	MTP_PROPERTY_ASSOCIATION_TYPE,
	MTP_PROPERTY_ASSOCIATION_DESC,*/
	MTP_PROPERTY_OBJECT_FILE_NAME,
	/*MTP_PROPERTY_DATE_CREATED,
	MTP_PROPERTY_DATE_MODIFIED,
	MTP_PROPERTY_KEYWORDS,
	MTP_PROPERTY_PARENT_OBJECT,
	MTP_PROPERTY_ALLOWED_FOLDER_CONTENTS,
	MTP_PROPERTY_HIDDEN,
	MTP_PROPERTY_SYSTEM_OBJECT,*/
	0xFFFF
};

int build_properties_dataset(mtp_ctx * ctx,void * buffer, int maxsize,uint32_t property_id,uint32_t format_id)
{
	int ofs;

	ofs = 0;

	PRINT_DEBUG("build_properties_dataset : 0x%.4X (%s) (Format : 0x%.4X - %s )", property_id, mtp_get_property_string(property_id), format_id, mtp_get_format_string(format_id));

	switch(property_id)
	{
		case MTP_PROPERTY_OBJECT_FILE_NAME:
			poke(buffer, &ofs, 2, MTP_PROPERTY_OBJECT_FILE_NAME);      // PropertyCode
			poke(buffer, &ofs, 2, MTP_TYPE_STR);                       // DataType
			poke(buffer, &ofs, 1, 0x01);                               // Get/Set
			poke(buffer, &ofs, 1, 0x00);                               // DefaultValue : Null sized strcat
			poke(buffer, &ofs, 4, 0x0000000);                          // GroupCode
			poke(buffer, &ofs, 1, 0x00);                               // FormFlag : None
		break;
	}

	return ofs;
}

int build_properties_supported_dataset(mtp_ctx * ctx,void * buffer, int maxsize,uint32_t format_id)
{
	int ofs,i;
	int nb_supported_prop;

	PRINT_DEBUG("build_properties_supported_dataset : (Format : 0x%.4X - %s )", format_id, mtp_get_format_string(format_id));

	nb_supported_prop = 0;

	while( supported_properties[nb_supported_prop] != 0xFFFF )
		nb_supported_prop++;

	ofs = 0;

	poke(buffer, &ofs, 2, nb_supported_prop);

	i = 0;
	while( supported_properties[i] != 0xFFFF )
	{
		poke(buffer, &ofs, 2, supported_properties[i]);
		i++;
	}

	return ofs;
}

int setObjectPropValue(mtp_ctx * ctx,MTP_PACKET_HEADER * mtp_packet_hdr, uint32_t handle,uint32_t prop_code)
{
	fs_entry * entry;
	char * path;
	char * path2;
	char tmpstr[256+1];
	unsigned int stringlen;

	PRINT_DEBUG("setObjectPropValue : (Handle : 0x%.8X - Prop code : 0x%.4X )", handle, prop_code);

	if( handle != 0xFFFFFFFF )
	{
		switch( prop_code )
		{
			case MTP_PROPERTY_OBJECT_FILE_NAME:
				entry = get_entry_by_handle(ctx->fs_db, handle);
				if(!entry)
					return MTP_RESPONSE_INVALID_OBJECT_HANDLE;

				path = build_full_path(ctx->fs_db, mtp_get_storage_root(ctx, entry->storage_id), entry);
				if(!path)
					return MTP_RESPONSE_GENERAL_ERROR;

				memset(tmpstr,0,sizeof(tmpstr));

				stringlen = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER), 1);

				if( stringlen > sizeof(tmpstr))
					stringlen = sizeof(tmpstr);

				unicode2charstring(tmpstr, (uint16_t *) ((char*)(mtp_packet_hdr) + sizeof(MTP_PACKET_HEADER) + 1), stringlen);

				if( entry->name )
				{
					free(entry->name);
					entry->name = NULL;
				}

				entry->name = malloc(strlen(tmpstr)+1);
				if( entry->name )
				{
					strcpy(entry->name,tmpstr);
				}

				path2 = build_full_path(ctx->fs_db, mtp_get_storage_root(ctx, entry->storage_id), entry);
				if(!path2)
				{
					free(path);
					return MTP_RESPONSE_GENERAL_ERROR;
				}

				if(rename(path, path2))
				{
					PRINT_ERROR("setObjectPropValue : Can't rename %s to %s", path, path2);

					free(path);
					free(path2);
					return MTP_RESPONSE_GENERAL_ERROR;
				}

				free(path);
				free(path2);
				return MTP_RESPONSE_OK;
			break;

			default:
				return MTP_RESPONSE_INVALID_OBJECT_PROP_CODE;
			break;
		}
	}
	else
	{
		return MTP_RESPONSE_INVALID_OBJECT_HANDLE;
	}
}