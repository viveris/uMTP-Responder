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
	MTP_PROPERTY_STORAGE_ID                            , //  0xDC01
	MTP_PROPERTY_OBJECT_FORMAT                         , //  0xDC02
	MTP_PROPERTY_PROTECTION_STATUS                     , //  0xDC03
	MTP_PROPERTY_OBJECT_SIZE                           , //  0xDC04
	MTP_PROPERTY_ASSOCIATION_TYPE                      , //  0xDC05
	MTP_PROPERTY_OBJECT_FILE_NAME                      , //  0xDC07
	MTP_PROPERTY_DATE_CREATED                          , //  0xDC08
	MTP_PROPERTY_DATE_MODIFIED                         , //  0xDC09
	MTP_PROPERTY_PARENT_OBJECT                         , //  0xDC0B
	0xFFFF
};

int build_properties_dataset(mtp_ctx * ctx,void * buffer, int maxsize,uint32_t property_id,uint32_t format_id)
{
	int ofs;

	ofs = 0;

	PRINT_DEBUG("build_properties_dataset : 0x%.4X (%s) (Format : 0x%.4X - %s )", property_id, mtp_get_property_string(property_id), format_id, mtp_get_format_string(format_id));

	switch(property_id)
	{
		case MTP_PROPERTY_STORAGE_ID:
			poke(buffer, &ofs, 2, MTP_PROPERTY_STORAGE_ID);            // PropertyCode
			poke(buffer, &ofs, 2, MTP_TYPE_UINT32);                    // DataType
			poke(buffer, &ofs, 1, 0x00);                               // Get
			poke(buffer, &ofs, 4, 0x00000000);                         // DefaultValue
			poke(buffer, &ofs, 4, 0x00000001);                         // GroupCode
			poke(buffer, &ofs, 1, 0x00);                               // FormFlag : None
		break;

		case MTP_PROPERTY_OBJECT_FORMAT:
			poke(buffer, &ofs, 2, MTP_PROPERTY_OBJECT_FORMAT);         // PropertyCode
			poke(buffer, &ofs, 2, MTP_TYPE_UINT16);                    // DataType
			poke(buffer, &ofs, 1, 0x00);                               // Get
			poke(buffer, &ofs, 2, 0x3000);                             // DefaultValue
			poke(buffer, &ofs, 4, 0x00000001);                         // GroupCode
			poke(buffer, &ofs, 1, 0x00);                               // FormFlag : None
		break;

		case MTP_PROPERTY_ASSOCIATION_TYPE:
			poke(buffer, &ofs, 2, MTP_PROPERTY_ASSOCIATION_TYPE);      // PropertyCode
			poke(buffer, &ofs, 2, MTP_TYPE_UINT16);                    // DataType
			poke(buffer, &ofs, 1, 0x00);                               // Get
			poke(buffer, &ofs, 2, 0x0001);                             // DefaultValue
			poke(buffer, &ofs, 4, 0x00000001);                         // GroupCode
			poke(buffer, &ofs, 1, 0x00);                               // FormFlag : None
		break;

		case MTP_PROPERTY_ASSOCIATION_DESC:
			poke(buffer, &ofs, 2, MTP_PROPERTY_ASSOCIATION_DESC);      // PropertyCode
			poke(buffer, &ofs, 2, MTP_TYPE_UINT32);                    // DataType
			poke(buffer, &ofs, 1, 0x00);                               // Get
			poke(buffer, &ofs, 4, 0x00000000);                             // DefaultValue
			poke(buffer, &ofs, 4, 0x00000001);                         // GroupCode
			poke(buffer, &ofs, 1, 0x00);                               // FormFlag : None
		break;

		case MTP_PROPERTY_PROTECTION_STATUS:
			poke(buffer, &ofs, 2, MTP_PROPERTY_PROTECTION_STATUS);     // PropertyCode
			poke(buffer, &ofs, 2, MTP_TYPE_UINT16);                    // DataType
			poke(buffer, &ofs, 1, 0x00);                               // Get
			poke(buffer, &ofs, 2, 0x0000);                             // DefaultValue
			poke(buffer, &ofs, 4, 0x00000001);                         // GroupCode
			poke(buffer, &ofs, 1, 0x00);                               // FormFlag : (Enumeration)
		break;

		case MTP_PROPERTY_HIDDEN:
			poke(buffer, &ofs, 2, MTP_PROPERTY_PROTECTION_STATUS);     // PropertyCode
			poke(buffer, &ofs, 2, MTP_TYPE_UINT16);                    // DataType
			poke(buffer, &ofs, 1, 0x00);                               // Get
			poke(buffer, &ofs, 2, 0x0000);                             // DefaultValue
			poke(buffer, &ofs, 4, 0x00000001);                         // GroupCode
			poke(buffer, &ofs, 1, 0x00);                               // FormFlag : (Enumeration)
		break;

		case MTP_PROPERTY_SYSTEM_OBJECT:
			poke(buffer, &ofs, 2, MTP_PROPERTY_PROTECTION_STATUS);     // PropertyCode
			poke(buffer, &ofs, 2, MTP_TYPE_UINT16);                    // DataType
			poke(buffer, &ofs, 1, 0x00);                               // Get
			poke(buffer, &ofs, 2, 0x0000);                             // DefaultValue
			poke(buffer, &ofs, 4, 0x00000001);                         // GroupCode
			poke(buffer, &ofs, 1, 0x00);                               // FormFlag : (Enumeration)
		break;

		case MTP_PROPERTY_OBJECT_SIZE:
			poke(buffer, &ofs, 2, MTP_PROPERTY_OBJECT_SIZE);           // PropertyCode
			poke(buffer, &ofs, 2, MTP_TYPE_UINT64);                    // DataType
			poke(buffer, &ofs, 1, 0x00);                               // Get/Set
			poke(buffer, &ofs, 8, 0x0000000000000000);                 // DefaultValue
			poke(buffer, &ofs, 4, 0x00000001);                         // GroupCode
			poke(buffer, &ofs, 1, 0x00);                               // FormFlag : None
		break;

		case MTP_PROPERTY_OBJECT_FILE_NAME:
			poke(buffer, &ofs, 2, MTP_PROPERTY_OBJECT_FILE_NAME);      // PropertyCode
			poke(buffer, &ofs, 2, MTP_TYPE_STR);                       // DataType
			poke(buffer, &ofs, 1, 0x01);                               // Get/Set
			poke(buffer, &ofs, 1, 0x00);                               // DefaultValue : Null sized strcat
			poke(buffer, &ofs, 4, 0x00000001);                         // GroupCode
			poke(buffer, &ofs, 1, 0x00);                               // FormFlag : RegEx
		break;

		case MTP_PROPERTY_DATE_CREATED:
			poke(buffer, &ofs, 2, MTP_PROPERTY_DATE_MODIFIED);         // PropertyCode
			poke(buffer, &ofs, 2, MTP_TYPE_STR);                       // DataType
			poke(buffer, &ofs, 1, 0x00);                               // Get/Set
			poke(buffer, &ofs, 1, 0x00);                               // DefaultValue : Null sized strcat
			poke(buffer, &ofs, 4, 0x00000001);                         // GroupCode
			poke(buffer, &ofs, 1, 0x03);                               // FormFlag : None
		break;

		case MTP_PROPERTY_DATE_MODIFIED:
			poke(buffer, &ofs, 2, MTP_PROPERTY_DATE_MODIFIED);         // PropertyCode
			poke(buffer, &ofs, 2, MTP_TYPE_STR);                       // DataType
			poke(buffer, &ofs, 1, 0x00);                               // Get
			poke(buffer, &ofs, 1, 0x00);                               // DefaultValue : Null sized strcat
			poke(buffer, &ofs, 4, 0x00000001);                         // GroupCode
			poke(buffer, &ofs, 1, 0x03);                               // FormFlag : None
		break;

		case MTP_PROPERTY_PARENT_OBJECT:
			poke(buffer, &ofs, 2, MTP_PROPERTY_PARENT_OBJECT);         // PropertyCode
			poke(buffer, &ofs, 2, MTP_TYPE_UINT32);                    // DataType
			poke(buffer, &ofs, 1, 0x00);                               // Get
			poke(buffer, &ofs, 4, 0x00000000);                         // DefaultValue : Null sized strcat
			poke(buffer, &ofs, 4, 0x00000001);                         // GroupCode
			poke(buffer, &ofs, 1, 0x00);                               // FormFlag : None
		break;

		case MTP_PROPERTY_PERSISTENT_UID:
			poke(buffer, &ofs, 2, MTP_PROPERTY_PERSISTENT_UID);         // PropertyCode
			poke(buffer, &ofs, 2, MTP_TYPE_UINT128);                    // DataType
			poke(buffer, &ofs, 1, 0x00);                                // Get
			poke(buffer, &ofs, 16, 0x00000000);                         // DefaultValue : Null sized strcat
			poke(buffer, &ofs, 4, 0x00000001);                         // GroupCode
			poke(buffer, &ofs, 1, 0x00);                               // FormFlag : None
		break;

		case MTP_PROPERTY_NAME:
			poke(buffer, &ofs, 2, MTP_PROPERTY_NAME);                  // PropertyCode
			poke(buffer, &ofs, 2, MTP_TYPE_STR);                       // DataType
			poke(buffer, &ofs, 1, 0x00);                               // Get/Set
			poke(buffer, &ofs, 1, 0x00);                               // DefaultValue : Null sized strcat
			poke(buffer, &ofs, 4, 0xFFFFFFFF);                         // GroupCode
			poke(buffer, &ofs, 1, 0x00);                               // FormFlag : None
		break;
	}

	return ofs;
}

int build_device_properties_dataset(mtp_ctx * ctx,void * buffer, int maxsize,uint32_t property_id)
{
	int ofs;

	ofs = 0;

	PRINT_DEBUG("build_device_properties_dataset : 0x%.4X (%s)", property_id, mtp_get_property_string(property_id));

	switch(property_id)
	{
		case MTP_DEVICE_PROPERTY_SYNCHRONIZATION_PARTNER:
			poke(buffer, &ofs, 2, MTP_DEVICE_PROPERTY_SYNCHRONIZATION_PARTNER);  // PropertyCode
			poke(buffer, &ofs, 2, MTP_TYPE_UINT32);                    // DataType
			poke(buffer, &ofs, 1, 0x00);                               // Get
			poke(buffer, &ofs, 4, 0x00000000);                         // DefaultValue
			poke(buffer, &ofs, 4, 0x00000001);                         // GroupCode
			poke(buffer, &ofs, 1, 0x00);                               // FormFlag : None
		break;

		case MTP_DEVICE_PROPERTY_IMAGE_SIZE:
			poke(buffer, &ofs, 2, MTP_DEVICE_PROPERTY_IMAGE_SIZE);  // PropertyCode
			poke(buffer, &ofs, 2, MTP_TYPE_UINT32);                    // DataType
			poke(buffer, &ofs, 1, 0x00);                               // Get
			poke(buffer, &ofs, 4, 0x00000000);                         // DefaultValue
			poke(buffer, &ofs, 4, 0x00000001);                         // GroupCode
			poke(buffer, &ofs, 1, 0x00);                               // FormFlag : None
		break;

		case MTP_DEVICE_PROPERTY_BATTERY_LEVEL:
			poke(buffer, &ofs, 2, MTP_DEVICE_PROPERTY_BATTERY_LEVEL);         // PropertyCode
			poke(buffer, &ofs, 2, MTP_TYPE_UINT16);                    // DataType
			poke(buffer, &ofs, 1, 0x00);                               // Get
			poke(buffer, &ofs, 2, 0x3000);                             // DefaultValue
			poke(buffer, &ofs, 4, 0x00000001);                         // GroupCode
			poke(buffer, &ofs, 1, 0x00);                               // FormFlag : None
		break;

		case MTP_DEVICE_PROPERTY_DEVICE_FRIENDLY_NAME:
			poke(buffer, &ofs, 2, MTP_DEVICE_PROPERTY_DEVICE_FRIENDLY_NAME);      // PropertyCode
			poke(buffer, &ofs, 2, MTP_TYPE_STR);                       // DataType
			poke(buffer, &ofs, 1, 0x01);                               // Get/Set
			poke(buffer, &ofs, 1, 0x00);                               // DefaultValue : Null sized strcat

			poke(buffer, &ofs, 1, 0x05);                               // "uMTP". TODO : Add an option in the config file to change this.
			poke(buffer, &ofs, 1, 0x75);
			poke(buffer, &ofs, 1, 0x00);
			poke(buffer, &ofs, 1, 0x4D);
			poke(buffer, &ofs, 1, 0x00);
			poke(buffer, &ofs, 1, 0x54);
			poke(buffer, &ofs, 1, 0x00);
			poke(buffer, &ofs, 1, 0x50);
			poke(buffer, &ofs, 1, 0x00);
			poke(buffer, &ofs, 1, 0x00);
			poke(buffer, &ofs, 1, 0x00);
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

	poke(buffer, &ofs, 4, nb_supported_prop);

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

int build_ObjectPropValue_dataset(mtp_ctx * ctx,void * buffer, int maxsize,uint32_t handle,uint32_t prop_code)
{
	int ofs;
	fs_entry * entry;
	char timestr[32];

	ofs = 0;

	PRINT_DEBUG("build_ObjectPropValue_dataset : Handle 0x%.8X - Property 0x%.4X (%s)", handle, prop_code, mtp_get_property_string(prop_code));

	entry = get_entry_by_handle(ctx->fs_db, handle);
	if( entry )
	{
		switch(prop_code)
		{
			case MTP_PROPERTY_OBJECT_FORMAT:
				if(entry->flags & ENTRY_IS_DIR)
					poke(buffer, &ofs, 2, MTP_FORMAT_ASSOCIATION);                          // ObjectFormat Code
				else
					poke(buffer, &ofs, 2, MTP_FORMAT_UNDEFINED);                            // ObjectFormat Code
			break;

			case MTP_PROPERTY_OBJECT_SIZE:
				poke(buffer, &ofs, 4, entry->size);
				poke(buffer, &ofs, 4, 0x00000000);
			break;

			case MTP_PROPERTY_OBJECT_FILE_NAME:
				poke_string(buffer, &ofs,entry->name);                                      // Filename
			break;

			case MTP_PROPERTY_STORAGE_ID:
				poke(buffer, &ofs, 4, entry->storage_id);
			break;

			case MTP_PROPERTY_PARENT_OBJECT:
					poke(buffer, &ofs, 4, entry->parent);
			break;

			case MTP_PROPERTY_HIDDEN:
					poke(buffer, &ofs, 2, 0x0000);
			break;

			case MTP_PROPERTY_SYSTEM_OBJECT:
					poke(buffer, &ofs, 2, 0x0000);
			break;

			case MTP_PROPERTY_PROTECTION_STATUS:
				poke(buffer, &ofs, 2, 0x0000);
			break;

			case MTP_PROPERTY_ASSOCIATION_TYPE:
				if(entry->flags & ENTRY_IS_DIR)
						poke(buffer, &ofs, 2, 0x0001);                          // ObjectFormat Code
				else
						poke(buffer, &ofs, 2, 0x0000);                          // ObjectFormat Code
			break;

			case MTP_PROPERTY_ASSOCIATION_DESC:
				poke(buffer, &ofs, 4, 0x00000000);
			break;


			case MTP_PROPERTY_DATE_CREATED:
			case MTP_PROPERTY_DATE_MODIFIED:
				snprintf(timestr,sizeof(timestr),"%.4d%.2d%.2dT%.2d%.2d%.2d",1900 + 110, 1, 2, 10, 11,12);
				poke_string(buffer, &ofs,timestr);
			break;

			default:
				PRINT_ERROR("build_ObjectPropValue_dataset : Unsupported property : 0x%.4X (%s)", prop_code, mtp_get_property_string(prop_code));
			break;
		}
	}

	return ofs;
}
