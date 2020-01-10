/*
 * uMTP Responder
 * Copyright (c) 2018 - 2020 Viveris Technologies
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
 * @file   mtp_op_getpartialobject.c
 * @brief  get partial object operation.
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include "buildconf.h"

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include <errno.h>

#include <pthread.h>

#include <inttypes.h>

#include "mtp_datasets.h"

#include "logs_out.h"

#include "mtp_helpers.h"

#include "mtp.h"
#include "mtp_datasets.h"
#include "mtp_properties.h"

#include "mtp_constant.h"
#include "mtp_constant_strings.h"

#include "mtp_support_def.h"

#include "usb_gadget_fct.h"

#include "mtp_operations.h"

#include "mtp_ops_helpers.h"

uint32_t mtp_op_GetPartialObject(mtp_ctx * ctx,MTP_PACKET_HEADER * mtp_packet_hdr, int * size,uint32_t * ret_params, int * ret_params_size)
{
	int sz;
	uint32_t response_code;
	uint32_t handle;
	fs_entry * entry;
	mtp_size actualsize;
	mtp_offset offset;
	int maxsize;

	if(!ctx->fs_db)
		return MTP_RESPONSE_SESSION_NOT_OPEN;

	pthread_mutex_lock( &ctx->inotify_mutex );

	handle = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER), 4);           // Get param 1 - Object handle

	if( mtp_packet_hdr->code == MTP_OPERATION_GET_PARTIAL_OBJECT_64 )
	{
		offset = peek64(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER) + 4, 8); // Get param 2 - Offset in bytes
		maxsize = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER) + 12, 4); // Get param 3 - Max size in bytes
		entry = get_entry_by_handle(ctx->fs_db, handle);
		PRINT_DEBUG("MTP_OPERATION_GET_PARTIAL_OBJECT_64 : handle 0x%.8X - Offset 0x%"SIZEHEX" - Maxsize 0x%.8X", handle,offset,maxsize);
	}
	else
	{
		offset = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER) + 4, 4);   // Get param 2 - Offset in bytes
		maxsize = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER) + 8, 4);  // Get param 3 - Max size in bytes
		entry = get_entry_by_handle(ctx->fs_db, handle);
		PRINT_DEBUG("MTP_OPERATION_GET_PARTIAL_OBJECT : handle 0x%.8X - Offset 0x%"SIZEHEX" - Maxsize 0x%.8X", handle,offset,maxsize);
	}

	if( check_handle_access( ctx, entry, handle, 0, &response_code) )
	{
		pthread_mutex_unlock( &ctx->inotify_mutex );
		return response_code;
	}

	if(entry)
	{
		sz = build_response(ctx, mtp_packet_hdr->tx_id, MTP_CONTAINER_TYPE_DATA, mtp_packet_hdr->code, ctx->wrbuffer,0,0);

		actualsize = send_file_data( ctx, entry, offset, maxsize );
		if( actualsize >= 0 )
		{
			ret_params[0] = actualsize & 0xFFFFFFFF;
			*ret_params_size = sizeof(uint32_t);
		}
		else
		{
			if(actualsize == -2)
			{
				pthread_mutex_unlock( &ctx->inotify_mutex );
				return MTP_RESPONSE_NO_RESPONSE;
			}
		}

		*size = sz;

		response_code = MTP_RESPONSE_OK;
	}
	else
	{
		PRINT_WARN("MTP_OPERATION_GET_PARTIAL_OBJECT ! : Entry/Handle not found (0x%.8X)", handle);

		response_code = MTP_RESPONSE_INVALID_OBJECT_HANDLE;
	}

	pthread_mutex_unlock( &ctx->inotify_mutex );

	return response_code;
}
