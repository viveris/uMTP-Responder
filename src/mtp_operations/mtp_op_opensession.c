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
 * @file   mtp_op_opensession.c
 * @brief  open session operation.
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include "buildconf.h"

#include <inttypes.h>
#include <pthread.h>

#include "mtp.h"
#include "mtp_helpers.h"
#include "mtp_constant.h"
#include "mtp_operations.h"

#include "logs_out.h"

uint32_t mtp_op_OpenSession(mtp_ctx * ctx,MTP_PACKET_HEADER * mtp_packet_hdr, int * size,uint32_t * ret_params, int * ret_params_size)
{
	int i;
	uint32_t id;

	id = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER), 4); // Get param 1

	if(!id)
	{
		return MTP_RESPONSE_INVALID_PARAMETER;
	}

	if(ctx->fs_db)
	{
		ret_params[0] = ctx->session_id;
		*ret_params_size = 1;
		return MTP_RESPONSE_SESSION_ALREADY_OPEN;
	}

	ctx->session_id = id;

	ctx->fs_db = init_fs_db(ctx);
	if( !ctx->fs_db )
	{
		PRINT_DEBUG("Open session - init fs db failure");
		return MTP_RESPONSE_GENERAL_ERROR;
	}

	i = 0;
	while( (i < MAX_STORAGE_NB) && ctx->storages[i].root_path)
	{
		if( pthread_mutex_lock( &ctx->inotify_mutex ) )
			return MTP_RESPONSE_GENERAL_ERROR;

		alloc_root_entry(ctx->fs_db, ctx->storages[i].storage_id);

		pthread_mutex_unlock( &ctx->inotify_mutex );

		i++;
	}

	PRINT_DEBUG("Open session - ID 0x%.8x",ctx->session_id);

	return MTP_RESPONSE_OK;
}
