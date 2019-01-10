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
 * @file   mtp_datasets.c
 * @brief  MTP datasets builders
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include <stdint.h>
#include <stdio.h>

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

#include "usb_gadget_fct.h"

#include "mtp_constant.h"
#include "mtp_constant_strings.h"

#include "mtp_support_def.h"

int build_deviceinfo_dataset(mtp_ctx * ctx, void * buffer, int maxsize)
{
	int ofs;

	ofs = 0;

	poke(buffer, &ofs, 2, MTP_VERSION);                                                  // Standard Version
	poke(buffer, &ofs, 4, 0x00000006);                                                   // MTP Vendor Extension ID
	poke(buffer, &ofs, 2, MTP_VERSION);                                                  // MTP Version
	poke_string(buffer, &ofs, DevInfos_MTP_Extensions);                                  // MTP Extensions
	poke(buffer, &ofs, 2, 0x0000);                                                       // Functional Mode
	poke_array(buffer, &ofs, supported_op_size, 2, (void*)&supported_op,1);              // Operations Supported
	poke_array(buffer, &ofs, supported_event_size, 2, (void*)&supported_event,1);        // Events Supported
	poke_array(buffer, &ofs, supported_property_size, 2, (void*)&supported_property,1);  // Device Properties Supported
	poke_array(buffer, &ofs, supported_formats_size, 2, (void*)&supported_formats,1);    // Capture Formats
	poke_array(buffer, &ofs, supported_formats_size, 2, (void*)&supported_formats,1);    // Playback Formats

	poke_string(buffer, &ofs, ctx->usb_cfg.usb_string_manufacturer);                     // Manufacturer
	poke_string(buffer, &ofs, ctx->usb_cfg.usb_string_product);                          // Model
	poke_string(buffer, &ofs, "Rev A");                                                  // Device Version
	poke_string(buffer, &ofs, ctx->usb_cfg.usb_string_serial);                           // Serial Number

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

	ofs = 0;

	storage_description = mtp_get_storage_description(ctx,storageid);
	storage_path = mtp_get_storage_root(ctx, storageid);
	if(storage_description && storage_path)
	{
		PRINT_DEBUG("Add storageinfo for %s", storage_path);
		poke(buffer, &ofs, 2, MTP_STORAGE_FIXED_RAM);                               // Storage Type
		poke(buffer, &ofs, 2, MTP_STORAGE_FILESYSTEM_HIERARCHICAL);                 // Filesystem Type
		poke(buffer, &ofs, 2, MTP_STORAGE_READ_WRITE);                              // Access Capability

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

		poke(buffer, &ofs, 4, totalspace&0x00000000FFFFFFFF);                       // Max Capacity
		poke(buffer, &ofs, 4, (totalspace>>32));                                    //

		poke(buffer, &ofs, 4, freespace&0x00000000FFFFFFFF);                        // Free space in Bytes
		poke(buffer, &ofs, 4, (freespace>>32));                                     //

		poke(buffer, &ofs, 4, 0xFFFFFFFF);                                          // Free Space In Objects

		poke_string(buffer, &ofs, storage_description);                             // Storage Description

		sprintf(volumeident,"UMTPRD_%.8X",storageid);
		poke_string(buffer, &ofs, volumeident);                                     // Volume Identifier
	}
	else
	{
		PRINT_WARN("build_storageinfo_dataset : Storage not found ! (0x%.8X)", storageid);
	}

	return ofs;
}

int build_objectinfo_dataset(mtp_ctx * ctx, void * buffer, int maxsize,fs_entry * entry)
{
	struct stat entrystat;
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
	    ret = stat(path, &entrystat);
	}

	if(ret)
	{
		if(path)
			free(path);
		return 0;
	}

	poke(buffer, &ofs, 4, entry->storage_id);                                   // StorageID  (NR)
	if(entry->flags & ENTRY_IS_DIR)
		poke(buffer, &ofs, 2, MTP_FORMAT_ASSOCIATION);                          // ObjectFormat Code
	else
		poke(buffer, &ofs, 2, MTP_FORMAT_UNDEFINED);                            // ObjectFormat Code
	poke(buffer, &ofs, 2, 0x0000);                                              // Protection Status (NR)

	entry->size = entrystat.st_size;

	poke(buffer, &ofs, 4, entry->size);                                         // Object Compressed Size
	poke(buffer, &ofs, 2, 0x0000);                                              // Thumb Format (NR)
	poke(buffer, &ofs, 4, 0x00000000);                                          // Thumb Compressed Size (NR)
	poke(buffer, &ofs, 4, 0x00000000);                                          // Thumb Pix Width (NR)
	poke(buffer, &ofs, 4, 0x00000000);                                          // Thumb Pix Height (NR)
	poke(buffer, &ofs, 4, 0x00000000);                                          // Image Pix Width (NR)
	poke(buffer, &ofs, 4, 0x00000000);                                          // Image Pix Height (NR)
	poke(buffer, &ofs, 4, 0x00000000);                                          // Image Bit Depth (NR)
	poke(buffer, &ofs, 4, entry->parent);                                       // Parent Object (NR)

	if(entry->flags & 0x000001)
		poke(buffer, &ofs, 2, MTP_ASSOCIATION_TYPE_GENERIC_FOLDER);             // Association Type
	else
		poke(buffer, &ofs, 2, 0x0000);                                          // Association Type

	poke(buffer, &ofs, 4, 0x00000000);                                          // Association Description
	poke(buffer, &ofs, 4, 0x00000000);                                          // Sequence Number (NR)
	poke_string(buffer, &ofs,entry->name);                                      // Filename

	// Date Created (NR) "YYYYMMDDThhmmss.s"
	t = entrystat.st_mtime;
	localtime_r(&t, &lt);
	snprintf(timestr,sizeof(timestr),"%.4d%.2d%.2dT%.2d%.2d%.2d",1900 + lt.tm_year, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);
	poke_string(buffer, &ofs,timestr);

	// Date Modified (NR) "YYYYMMDDThhmmss.s"
	t = entrystat.st_mtime;
	localtime_r(&t, &lt);
	snprintf(timestr,sizeof(timestr),"%.4d%.2d%.2dT%.2d%.2d%.2d",1900 + lt.tm_year, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);
	poke_string(buffer, &ofs,timestr);

	poke_string(buffer, &ofs,"");                                               // Keywords (NR)

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

	poke(buffer, &ofs, 4, 0);                                            // Size
	poke(buffer, &ofs, 2, MTP_CONTAINER_TYPE_EVENT);                     // Type
	poke(buffer, &ofs, 2, event );                                       // Event Code
	poke(buffer, &ofs, 4, ctx->session_id);                              // MTP Session ID
	for(i=0;i<nbparams;i++)
	{
		poke(buffer, &ofs, 4, parameters[i]);
	}

	// Update size
	i = 0;
	poke(buffer, &i, 4, ofs);

	return ofs;
}
