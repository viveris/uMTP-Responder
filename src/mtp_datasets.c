/*
 * uMTP Responder
 * Copyright (c) 2018 - 2021 Viveris Technologies
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
 * @file   mtp_datasets.c
 * @brief  MTP datasets builders
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include "buildconf.h"

#include <inttypes.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#include "mtp.h"
#include "mtp_helpers.h"
#include "mtp_constant.h"
#include "mtp_constant_strings.h"
#include "mtp_datasets.h"
#include "mtp_support_def.h"
#include "mtp_properties.h"

#include "usb_gadget_fct.h"
#include "fs_handles_db.h"
#include "logs_out.h"

void set_default_date(struct tm * date)
{
	memset(date, 0, sizeof(struct tm));
	date->tm_year = 110;
}

int build_deviceinfo_dataset(mtp_ctx * ctx, void * buffer, int maxsize)
{
	int ofs,i,elements_cnt;

	ofs = poke16(buffer, 0, maxsize, MTP_VERSION);                                                   // Standard Version
	ofs = poke32(buffer, ofs, maxsize, 0x00000006);                                                  // MTP Vendor Extension ID
	ofs = poke16(buffer, ofs, maxsize, MTP_VERSION);                                                 // MTP Version
	ofs = poke_string(buffer, ofs, maxsize, DevInfos_MTP_Extensions);                                // MTP Extensions
	ofs = poke16(buffer, ofs, maxsize, 0x0000);                                                      // Functional Mode
	ofs = poke_array(buffer, ofs, maxsize, supported_op_size, 2, (void*)&supported_op,1);            // Operations Supported
	ofs = poke_array(buffer, ofs, maxsize, supported_event_size, 2, (void*)&supported_event,1);      // Events Supported

	// Supported device properties
	elements_cnt = 0;
	while( dev_properties[elements_cnt].prop_code != 0xFFFF )
	{
		elements_cnt++;
	}

	ofs = poke32(buffer, ofs, maxsize, elements_cnt);
	for( i = 0; i < elements_cnt ; i++ )
	{
		ofs = poke16(buffer, ofs, maxsize, dev_properties[i].prop_code);
	}


	// Supported formats

	// Capture Formats... (No capture format)

	ofs = poke32(buffer, ofs, maxsize, 0x00000000);

	// Playback Formats
	elements_cnt = 0;
	while( fmt_properties[elements_cnt].format_code != 0xFFFF )
	{
		elements_cnt++;
	}

	ofs = poke32(buffer, ofs, maxsize, elements_cnt);
	for( i = 0; i < elements_cnt ; i++ )
	{
		ofs = poke16(buffer, ofs, maxsize, fmt_properties[i].format_code);
	}

	ofs = poke_string(buffer, ofs, maxsize, ctx->usb_cfg.usb_string_manufacturer);                     // Manufacturer
	ofs = poke_string(buffer, ofs, maxsize, ctx->usb_cfg.usb_string_product);                          // Model
	ofs = poke_string(buffer, ofs, maxsize, ctx->usb_cfg.usb_string_version);                          // Device Version
	ofs = poke_string(buffer, ofs, maxsize, ctx->usb_cfg.usb_string_serial);                           // Serial Number

	return ofs;
}

int build_storageinfo_dataset(mtp_ctx * ctx,void * buffer, int maxsize,uint32_t storageid)
{
	int ofs;
	char * storage_description;
	char volumeident[16];
	char * storage_path = NULL;
	struct statvfs fsbuf;
	uint64_t freespace = 0x4000000000000000U;
	uint64_t totalspace = 0x8000000000000000U;
	uint32_t storage_flags;
	uint16_t storage_type;

	ofs = 0;

	storage_description = mtp_get_storage_description(ctx,storageid);
	storage_path = mtp_get_storage_root(ctx, storageid);
	storage_flags = mtp_get_storage_flags(ctx, storageid);

	if (storage_flags & UMTP_STORAGE_REMOVABLE)
		storage_type = MTP_STORAGE_REMOVABLE_RAM;
	else
		storage_type = MTP_STORAGE_FIXED_RAM;

	if(storage_description && storage_path)
	{
		PRINT_DEBUG("Add storageinfo for %s", storage_path);
		ofs = poke16(buffer, ofs, maxsize, storage_type);                                        // Storage Type
		ofs = poke16(buffer, ofs, maxsize, MTP_STORAGE_FILESYSTEM_HIERARCHICAL);                 // Filesystem Type

		// Access Capability
		if( storage_flags & UMTP_STORAGE_READONLY )
			ofs = poke16(buffer, ofs, maxsize, MTP_STORAGE_READ_ONLY_WITHOUT_DELETE);
		else
			ofs = poke16(buffer, ofs, maxsize, MTP_STORAGE_READ_WRITE);

		if(statvfs(storage_path, &fsbuf) == 0)
		{
			totalspace = (uint64_t)fsbuf.f_bsize * (uint64_t)fsbuf.f_blocks;
			freespace = (uint64_t)fsbuf.f_bsize * (uint64_t)fsbuf.f_bavail;
			PRINT_DEBUG("Total space %" PRIu64 " byte(s)", totalspace);
			PRINT_DEBUG("Free space %" PRIu64 " byte(s)", freespace);
		}
		else
		{
			PRINT_WARN("Failed to get statvfs for %s", storage_path);
		}

		ofs = poke32(buffer, ofs, maxsize, totalspace&0x00000000FFFFFFFF);                       // Max Capacity
		ofs = poke32(buffer, ofs, maxsize, (totalspace>>32));                                    //

		ofs = poke32(buffer, ofs, maxsize, freespace&0x00000000FFFFFFFF);                        // Free space in Bytes
		ofs = poke32(buffer, ofs, maxsize, (freespace>>32));                                     //

		ofs = poke32(buffer, ofs, maxsize, 0x40000000);                                          // Free Space In Objects

		ofs = poke_string(buffer, ofs, maxsize, storage_description);                            // Storage Description

		sprintf(volumeident,"UMTPRD_%.8X",storageid);
		ofs = poke_string(buffer, ofs, maxsize, volumeident);                                    // Volume Identifier
	}
	else
	{
		PRINT_WARN("build_storageinfo_dataset : Storage not found ! (0x%.8X)", storageid);
	}

	return ofs;
}

int build_objectinfo_dataset(mtp_ctx * ctx, void * buffer, int maxsize,fs_entry * entry)
{
	struct stat64 entrystat;
	time_t t;
	struct tm lt;
	int ofs,ret;
	char * path;
	char timestr[32];

	ofs = 0;

	ret = -1;
	path = build_full_path(ctx->fs_db, mtp_get_storage_root(ctx, entry->storage_id), entry);

	if(path)
	{
		ret = stat64(path, &entrystat);
	}

	if(ret)
	{
		if(path)
			free(path);
		return 0;
	}

	ofs = poke32(buffer, ofs, maxsize, entry->storage_id);                                       // StorageID  (NR)
	if(entry->flags & ENTRY_IS_DIR)
		ofs = poke16(buffer, ofs, maxsize, MTP_FORMAT_ASSOCIATION);                              // ObjectFormat Code
	else
		ofs = poke16(buffer, ofs, maxsize, MTP_FORMAT_UNDEFINED);                                // ObjectFormat Code
	ofs = poke16(buffer, ofs, maxsize, 0x0000);                                                  // Protection Status (NR)

	entry->size = entrystat.st_size;

	ofs = poke32(buffer, ofs, maxsize, entry->size);                                             // Object Compressed Size
	ofs = poke16(buffer, ofs, maxsize, 0x0000);                                                  // Thumb Format (NR)
	ofs = poke32(buffer, ofs, maxsize, 0x00000000);                                              // Thumb Compressed Size (NR)
	ofs = poke32(buffer, ofs, maxsize, 0x00000000);                                              // Thumb Pix Width (NR)
	ofs = poke32(buffer, ofs, maxsize, 0x00000000);                                              // Thumb Pix Height (NR)
	ofs = poke32(buffer, ofs, maxsize, 0x00000000);                                              // Image Pix Width (NR)
	ofs = poke32(buffer, ofs, maxsize, 0x00000000);                                              // Image Pix Height (NR)
	ofs = poke32(buffer, ofs, maxsize, 0x00000000);                                              // Image Bit Depth (NR)
	ofs = poke32(buffer, ofs, maxsize, entry->parent);                                           // Parent Object (NR)

	if(entry->flags & 0x000001)
		ofs = poke16(buffer, ofs, maxsize, MTP_ASSOCIATION_TYPE_GENERIC_FOLDER);                 // Association Type
	else
		ofs = poke16(buffer, ofs, maxsize, 0x0000);                                              // Association Type

	ofs = poke32(buffer, ofs, maxsize, 0x00000000);                                              // Association Description
	ofs = poke32(buffer, ofs, maxsize, 0x00000000);                                              // Sequence Number (NR)
	ofs = poke_string(buffer, ofs, maxsize, entry->name);                                         // Filename

	// Date Created (NR) "YYYYMMDDThhmmss.s"
	set_default_date(&lt);
	t = entrystat.st_mtime;
	localtime_r(&t, &lt);
	snprintf(timestr,sizeof(timestr),"%.4d%.2d%.2dT%.2d%.2d%.2d",1900 + lt.tm_year, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);
	ofs = poke_string(buffer, ofs, maxsize, timestr);

	// Date Modified (NR) "YYYYMMDDThhmmss.s"
	set_default_date(&lt);
	t = entrystat.st_mtime;
	localtime_r(&t, &lt);
	snprintf(timestr,sizeof(timestr),"%.4d%.2d%.2dT%.2d%.2d%.2d",1900 + lt.tm_year, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);
	ofs = poke_string(buffer, ofs, maxsize, timestr);

	ofs = poke08(buffer, ofs, maxsize, 0x00);      // Keywords (NR)

	if(path)
	{
		free(path);
	}
	return ofs;
}

int build_event_dataset(mtp_ctx * ctx, void * buffer, int maxsize, uint32_t event, uint32_t session, uint32_t transaction, int nbparams, uint32_t * parameters)
{
	int ofs,i;

	ofs = 0;

	ofs = poke32(buffer, ofs, maxsize, 0);                                            // Size
	ofs = poke16(buffer, ofs, maxsize, MTP_CONTAINER_TYPE_EVENT);                     // Type
	ofs = poke16(buffer, ofs, maxsize, event );                                       // Event Code
	ofs = poke32(buffer, ofs, maxsize, ctx->session_id);                              // MTP Session ID
	for(i=0;i<nbparams;i++)
	{
		ofs = poke32(buffer, ofs, maxsize, parameters[i]);
	}

	if( ofs >= 0)
	{
		// Update size
		poke32(buffer, 0, maxsize, ofs);
	}

	return ofs;
}
