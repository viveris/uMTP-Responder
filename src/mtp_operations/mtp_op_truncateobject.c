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
 * @file   mtp_op_truncateobject.c
 * @brief  truncate object operation.
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include "buildconf.h"

#include <unistd.h>
#include <pthread.h>
#include <inttypes.h>
#include <errno.h>

#include "logs_out.h"

#include "mtp.h"
#include "mtp_helpers.h"
#include "mtp_constant.h"
#include "mtp_operations.h"

uint32_t mtp_op_TruncateObject(mtp_ctx * ctx,MTP_PACKET_HEADER * mtp_packet_hdr, int * size,uint32_t * ret_params, int * ret_params_size)
{
	uint32_t response_code;
	uint32_t handle;
	mtp_offset offset;
	fs_entry * entry;
	char * full_path;

	if(!ctx->fs_db)
		return MTP_RESPONSE_SESSION_NOT_OPEN;

	pthread_mutex_lock( &ctx->inotify_mutex );

	response_code = MTP_RESPONSE_GENERAL_ERROR;

	handle = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER), 4);
	offset = peek64(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER) + 4, 8);   // Get param 2 - Offset in bytes

	entry = get_entry_by_handle(ctx->fs_db, handle);
	if( entry )
	{

		if( check_handle_access( ctx, entry, handle, 1, &response_code) )
		{
			pthread_mutex_unlock( &ctx->inotify_mutex );
			return response_code;
		}

		full_path = build_full_path(ctx->fs_db, mtp_get_storage_root(ctx, entry->storage_id), entry);
		if(full_path)
		{
			PRINT_DEBUG("Truncate file at 0x%"SIZEHEX" Bytes",offset);
			if( !truncate64(full_path, offset) )
			{
				response_code = MTP_RESPONSE_OK;
			}
			else
			{
				response_code = posix_to_mtp_errcode(errno);
			}
		}
	}
	else
	{
		response_code = MTP_RESPONSE_INVALID_OBJECT_HANDLE;
	}

	pthread_mutex_unlock( &ctx->inotify_mutex );

	return response_code;
}
