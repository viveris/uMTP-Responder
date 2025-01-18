/*
 * uMTP Responder
 * Copyright (c) 2018 - 2025 Viveris Technologies
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
 * @file   mtp_op_getstorageinfo.c
 * @brief  get storage info operation.
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include "buildconf.h"

#include <inttypes.h>
#include <pthread.h>

#include "mtp.h"
#include "mtp_helpers.h"
#include "mtp_constant.h"
#include "mtp_operations.h"
#include "mtp_datasets.h"

#include "usb_gadget_fct.h"

#include "logs_out.h"

uint32_t mtp_op_GetStorageInfo(mtp_ctx * ctx,MTP_PACKET_HEADER * mtp_packet_hdr, int * size,uint32_t * ret_params, int * ret_params_size)
{
	int ofs,tmp_ofs;
	uint32_t storageid;

	if(!ctx->fs_db)
		return MTP_RESPONSE_SESSION_NOT_OPEN;

	storageid = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER) + 0, 4); // Get param 1 - Storage ID
	if( mtp_get_storage_root(ctx,storageid) )
	{
		ofs = build_response(ctx, mtp_packet_hdr->tx_id, MTP_CONTAINER_TYPE_DATA, mtp_packet_hdr->code, ctx->wrbuffer, ctx->usb_wr_buffer_max_size, 0,0);
		if(ofs < 0)
			goto error;

		tmp_ofs = build_storageinfo_dataset(ctx, ctx->wrbuffer + sizeof(MTP_PACKET_HEADER), ctx->usb_wr_buffer_max_size - sizeof(MTP_PACKET_HEADER),storageid);
		if(tmp_ofs < 0)
			goto error;

		ofs += tmp_ofs;

		// Update packet size
		poke32(ctx->wrbuffer, 0, ctx->usb_wr_buffer_max_size, ofs);

		PRINT_DEBUG("MTP_OPERATION_GET_STORAGE_INFO response (%d Bytes):",ofs);
		PRINT_DEBUG_BUF(ctx->wrbuffer, ofs);

		write_usb(ctx->usb_ctx,EP_DESCRIPTOR_IN,ctx->wrbuffer,ofs);

		check_and_send_USB_ZLP(ctx , ofs );

		*size = ofs;

		return MTP_RESPONSE_OK;
	}
	else
	{
		return MTP_RESPONSE_INVALID_STORAGE_ID;
	}

error:
	return MTP_RESPONSE_GENERAL_ERROR;
}

