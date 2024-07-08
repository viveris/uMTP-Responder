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
 * @file   mtp_op_getobjectproplist.c
 * @brief  get object prop list operation.
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

uint32_t mtp_op_GetObjectPropList(mtp_ctx * ctx,MTP_PACKET_HEADER * mtp_packet_hdr, int * size,uint32_t * ret_params, int * ret_params_size)
{
	fs_entry * entry;
	uint32_t response_code;
	uint32_t handle;
	uint32_t format_id;
	uint32_t prop_code;
	uint32_t prop_group_code;
	uint32_t depth;
	int sz,tmp_sz;

	if(!ctx->fs_db)
		return MTP_RESPONSE_SESSION_NOT_OPEN;

	if( pthread_mutex_lock( &ctx->inotify_mutex ) )
		return MTP_RESPONSE_GENERAL_ERROR;

	handle = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER), 4);
	format_id = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER) + 4, 4);
	prop_code = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER) + 8, 4);
	prop_group_code = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER) + 12, 4);
	depth = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER) + 16, 4);

	PRINT_DEBUG("MTP_OPERATION_GET_OBJECT_PROP_LIST :(Handle: 0x%.8X FormatCode: 0x%.8X ObjPropCode: 0x%.8X ObjPropGroupCode: 0x%.8X Depth: %d)", handle, format_id, prop_code, prop_group_code, depth);

	if( format_id != 0x00000000 )
	{   // Specification by format not currently supported
		response_code = MTP_RESPONSE_SPECIFICATION_BY_FORMAT_UNSUPPORTED;
	}

	if( prop_code == 0x00000000 )
	{   // ObjectPropGroupCode not currently supported

		if( prop_group_code == 0x00000000 )
		{
			response_code = MTP_RESPONSE_PARAMETER_NOT_SUPPORTED;

			PRINT_DEBUG("MTP_OPERATION_GET_OBJECT_PROP_LIST : ObjectPropGroupCode not currently supported !");

			pthread_mutex_unlock( &ctx->inotify_mutex );

			return MTP_RESPONSE_PARAMETER_NOT_SUPPORTED;
		}
		else
		{
		}

	}

	entry = get_entry_by_handle(ctx->fs_db, handle);
	if( entry )
	{
		sz = build_response(ctx, mtp_packet_hdr->tx_id, MTP_CONTAINER_TYPE_DATA, mtp_packet_hdr->code, ctx->wrbuffer, ctx->usb_wr_buffer_max_size,0,0);
		if(sz < 0)
			goto error;

		tmp_sz = build_objectproplist_dataset(ctx, ctx->wrbuffer + sizeof(MTP_PACKET_HEADER),ctx->usb_wr_buffer_max_size - sizeof(MTP_PACKET_HEADER),entry, handle, format_id, prop_code, prop_group_code, depth);
		if( tmp_sz < 0)
			goto error;

		sz += tmp_sz;

		// Update packet size
		poke32(ctx->wrbuffer, 0, ctx->usb_wr_buffer_max_size, sz);

		PRINT_DEBUG("MTP_OPERATION_GET_OBJECT_PROP_LIST response (%d Bytes):",sz);
		PRINT_DEBUG_BUF(ctx->wrbuffer, sz);

		write_usb(ctx->usb_ctx,EP_DESCRIPTOR_IN,ctx->wrbuffer,sz);

		check_and_send_USB_ZLP(ctx , sz );

		*size = sz;

		response_code = MTP_RESPONSE_OK;
	}
	else
	{
		response_code = MTP_RESPONSE_INVALID_OBJECT_HANDLE;
	}
	pthread_mutex_unlock( &ctx->inotify_mutex );

	return response_code;

error:
	pthread_mutex_unlock( &ctx->inotify_mutex );

	return MTP_RESPONSE_GENERAL_ERROR;
}
