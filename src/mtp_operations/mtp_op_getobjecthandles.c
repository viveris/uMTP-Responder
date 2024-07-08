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
 * @file   mtp_op_getobjecthandles.c
 * @brief  get object handles operation
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include "buildconf.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>

#include "mtp.h"
#include "mtp_helpers.h"
#include "mtp_constant.h"
#include "mtp_operations.h"
#include "usb_gadget_fct.h"
#include "inotify.h"

#include "logs_out.h"

uint32_t mtp_op_GetObjectHandles(mtp_ctx * ctx,MTP_PACKET_HEADER * mtp_packet_hdr, int * size,uint32_t * ret_params, int * ret_params_size)
{
	int ofs;
	uint32_t storageid;
	uint32_t parent_handle;
	int handle_index;
	int nb_of_handles;
	fs_entry * entry;
	char * full_path;
	char * tmp_str;
	int sz,ret;

	if(!ctx->fs_db)
		return MTP_RESPONSE_SESSION_NOT_OPEN;

	if( pthread_mutex_lock( &ctx->inotify_mutex ) )
		return MTP_RESPONSE_GENERAL_ERROR;

	storageid = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER) + 0, 4);        // Get param 1 - Storage ID

	sz = build_response(ctx, mtp_packet_hdr->tx_id, MTP_CONTAINER_TYPE_DATA, mtp_packet_hdr->code, ctx->wrbuffer, ctx->usb_wr_buffer_max_size, 0,0);
	if(sz < 0)
		goto error;

	parent_handle = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER)+ 8, 4);     // Get param 3 - parent handle

	PRINT_DEBUG("MTP_OPERATION_GET_OBJECT_HANDLES - Parent Handle 0x%.8x, Storage ID 0x%.8x",parent_handle,storageid);

	if(!mtp_get_storage_root(ctx,storageid))
	{
		PRINT_WARN("MTP_OPERATION_GET_OBJECT_HANDLES : INVALID STORAGE ID!");

		pthread_mutex_unlock( &ctx->inotify_mutex );

		return MTP_RESPONSE_INVALID_STORAGE_ID;
	}

	tmp_str = NULL;
	full_path = NULL;
	entry = NULL;

	if(parent_handle && parent_handle!=0xFFFFFFFF)
	{
		entry = get_entry_by_handle(ctx->fs_db, parent_handle);
		if(entry)
		{
			tmp_str = build_full_path(ctx->fs_db, mtp_get_storage_root(ctx, entry->storage_id), entry);
			full_path = tmp_str;
		}
	}
	else
	{
		// root folder
		parent_handle = 0x00000000;
		full_path = mtp_get_storage_root(ctx,storageid);
		entry = get_entry_by_handle_and_storageid(ctx->fs_db, parent_handle, storageid);
	}

	nb_of_handles = 0;

	if( full_path )
	{
		// Count the number of files...
		ret = -1;

		if(!set_storage_giduid(ctx, storageid))
		{
			ret = scan_and_add_folder(ctx->fs_db, full_path, parent_handle, storageid);
		}
		restore_giduid(ctx);

		if(ret < 0)
		{
			PRINT_WARN("MTP_OPERATION_GET_OBJECT_HANDLES : FOLDER ACCESS ERROR !");

			pthread_mutex_unlock( &ctx->inotify_mutex );

			return MTP_RESPONSE_ACCESS_DENIED;
		}

		init_search_handle(ctx->fs_db, parent_handle, storageid);

		while( get_next_child_handle(ctx->fs_db) )
		{
			nb_of_handles++;
		}

		PRINT_DEBUG("MTP_OPERATION_GET_OBJECT_HANDLES - %d objects found",nb_of_handles);

		// Restart
		init_search_handle(ctx->fs_db, parent_handle, storageid);

		// Register a watch point.
		if( entry )
		{
			if ( entry->flags & ENTRY_IS_DIR )
			{
				entry->watch_descriptor = inotify_handler_addwatch( ctx, full_path );
			}
		}

		if (tmp_str)
			free(tmp_str);
	}

	// Update packet size
	poke32(ctx->wrbuffer, 0, ctx->usb_wr_buffer_max_size, sizeof(MTP_PACKET_HEADER) + ((1+nb_of_handles)*4) );

	// Build and send the handles array
	ofs = sizeof(MTP_PACKET_HEADER);

	ofs = poke32(ctx->wrbuffer, ofs, ctx->usb_wr_buffer_max_size, nb_of_handles);

	PRINT_DEBUG("MTP_OPERATION_GET_OBJECT_HANDLES response :");

	handle_index = 0;
	do
	{
		do
		{
			entry = get_next_child_handle(ctx->fs_db);
			if(entry)
			{
				PRINT_DEBUG("File : %s Handle:%.8x",entry->name,entry->handle);
				ofs = poke32(ctx->wrbuffer, ofs, ctx->usb_wr_buffer_max_size, entry->handle);

				handle_index++;
			}
		}while( ofs >= 0 && ofs < ctx->max_packet_size && handle_index < nb_of_handles);

		if(ofs < 0)
			goto error;

		PRINT_DEBUG_BUF(ctx->wrbuffer, ofs);

		// Current usb packet full, need to send it.
		write_usb(ctx->usb_ctx,EP_DESCRIPTOR_IN,ctx->wrbuffer,ofs);

		ofs = 0;

	}while( handle_index < nb_of_handles);

	// Total size = Header size + nb of handles field (uint32_t) + all handles
	check_and_send_USB_ZLP(ctx , sizeof(MTP_PACKET_HEADER) + sizeof(uint32_t) + (nb_of_handles * sizeof(uint32_t)) );

	pthread_mutex_unlock( &ctx->inotify_mutex );

	return MTP_RESPONSE_OK;

error:
	pthread_mutex_unlock( &ctx->inotify_mutex );

	return MTP_RESPONSE_GENERAL_ERROR;
}
