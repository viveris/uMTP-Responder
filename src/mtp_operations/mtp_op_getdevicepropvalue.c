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
 * @file   mtp_op_getdevicepropvalue.c
 * @brief  get device prop value operation.
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include "buildconf.h"

#include <inttypes.h>
#include <pthread.h>

#include "mtp.h"
#include "mtp_helpers.h"
#include "mtp_constant.h"
#include "mtp_operations.h"
#include "mtp_properties.h"
#include "usb_gadget_fct.h"

#include "logs_out.h"

uint32_t mtp_op_GetDevicePropValue(mtp_ctx * ctx,MTP_PACKET_HEADER * mtp_packet_hdr, int * size,uint32_t * ret_params, int * ret_params_size)
{
	uint32_t response_code,prop_code;
	int sz,tmp_sz;

	if(!ctx->fs_db)
		return MTP_RESPONSE_SESSION_NOT_OPEN;

	pthread_mutex_lock( &ctx->inotify_mutex );

	prop_code = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER), 4);     // Get param 1 - PropCode

	PRINT_DEBUG("MTP_OPERATION_GET_DEVICE_PROP_VALUE : (Prop code : 0x%.4X )", prop_code);

	sz = build_response(ctx, mtp_packet_hdr->tx_id, MTP_CONTAINER_TYPE_DATA, mtp_packet_hdr->code, ctx->wrbuffer, ctx->usb_wr_buffer_max_size, 0,0);
	if(sz < 0)
		goto error;

	tmp_sz = build_DevicePropValue_dataset(ctx,ctx->wrbuffer + sizeof(MTP_PACKET_HEADER), ctx->usb_wr_buffer_max_size - sizeof(MTP_PACKET_HEADER), prop_code);
	if(tmp_sz < 0)
		goto error;

	sz += tmp_sz;

	if ( sz > sizeof(MTP_PACKET_HEADER) )
	{
		// Update packet size
		poke32(ctx->wrbuffer, 0, ctx->usb_wr_buffer_max_size, sz);

		PRINT_DEBUG("MTP_OPERATION_GET_DEVICE_PROP_VALUE response (%d Bytes):",sz);
		PRINT_DEBUG_BUF(ctx->wrbuffer, sz);

		write_usb(ctx->usb_ctx,EP_DESCRIPTOR_IN,ctx->wrbuffer,sz);

		check_and_send_USB_ZLP(ctx , sz );

		*size = sz;

		response_code = MTP_RESPONSE_OK;
	}
	else
	{
		response_code = MTP_RESPONSE_DEVICE_PROP_NOT_SUPPORTED;
	}

	pthread_mutex_unlock( &ctx->inotify_mutex );

	return response_code;

error:
	pthread_mutex_unlock( &ctx->inotify_mutex );

	return MTP_RESPONSE_GENERAL_ERROR;
}
