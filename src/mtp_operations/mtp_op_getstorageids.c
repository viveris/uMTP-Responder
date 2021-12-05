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
 * @file   mtp_op_getstorageids.c
 * @brief  get storage ids operation.
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include "buildconf.h"

#include <inttypes.h>
#include <pthread.h>

#include "mtp.h"
#include "mtp_helpers.h"
#include "mtp_constant.h"
#include "mtp_operations.h"

#include "usb_gadget_fct.h"

#include "logs_out.h"

uint32_t mtp_op_GetStorageIDs(mtp_ctx * ctx,MTP_PACKET_HEADER * mtp_packet_hdr, int * size,uint32_t * ret_params, int * ret_params_size)
{
	int ofs,i,cnt;

	if(!ctx->fs_db)
		return MTP_RESPONSE_SESSION_NOT_OPEN;

	ofs = build_response(ctx, mtp_packet_hdr->tx_id, MTP_CONTAINER_TYPE_DATA, mtp_packet_hdr->code, ctx->wrbuffer, ctx->usb_wr_buffer_max_size, 0,0);
	if(ofs < 0)
		goto error;

	cnt = 0;
	i = 0;
	while( (i < MAX_STORAGE_NB) && ctx->storages[i].root_path)
	{
		if( !(ctx->storages[i].flags & UMTP_STORAGE_NOTMOUNTED) &&
			!(ctx->storages[i].flags & UMTP_STORAGE_LOCKED) )
		{
			ctx->temp_array[cnt] = ctx->storages[i].storage_id; // Storage ID
			cnt++;
		}
		i++;
	}

	ofs = poke_array(ctx->wrbuffer, ofs, ctx->usb_wr_buffer_max_size, 4 * cnt, 4, (void*)ctx->temp_array, 1);
	if(ofs < 0)
		goto error;

	// Update packet size
	poke32(ctx->wrbuffer, 0, ctx->usb_wr_buffer_max_size, ofs);

	PRINT_DEBUG("MTP_OPERATION_GET_STORAGE_IDS response (%d Bytes):",ofs);
	PRINT_DEBUG_BUF(ctx->wrbuffer, ofs);

	write_usb(ctx->usb_ctx,EP_DESCRIPTOR_IN,ctx->wrbuffer,ofs);

	check_and_send_USB_ZLP(ctx , ofs );

	*size = ofs;

	return MTP_RESPONSE_OK;

error:

	return MTP_RESPONSE_GENERAL_ERROR;
}
