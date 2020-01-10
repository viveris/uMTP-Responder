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
 * @file   mtp_op_deleteobject.c
 * @brief  Delete object operation
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

#include "inotify.h"

#include "mtp_ops_helpers.h"

uint32_t mtp_op_DeleteObject(mtp_ctx * ctx,MTP_PACKET_HEADER * mtp_packet_hdr, int * size,uint32_t * ret_params, int * ret_params_size)
{
	uint32_t response_code;
	uint32_t handle;

	if(!ctx->fs_db)
		return MTP_RESPONSE_SESSION_NOT_OPEN;

	pthread_mutex_lock( &ctx->inotify_mutex );

	handle = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER), 4); // Get param 1 - object handle

	if( check_handle_access( ctx, NULL, handle, 1, &response_code) )
	{
		pthread_mutex_unlock( &ctx->inotify_mutex );

		return response_code;
	}

	if(delete_tree(ctx, handle))
	{
		response_code = MTP_RESPONSE_OBJECT_WRITE_PROTECTED;
	}
	else
	{
		response_code = MTP_RESPONSE_OK;
	}

	pthread_mutex_unlock( &ctx->inotify_mutex );

	return response_code;
}
