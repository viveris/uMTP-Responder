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
 * @file   mtp_op_sendobject.c
 * @brief  send object operation.
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include "buildconf.h"

#include <inttypes.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/stat.h>

#include "logs_out.h"

#include "mtp.h"
#include "mtp_helpers.h"
#include "mtp_constant.h"
#include "mtp_operations.h"

#include "usb_gadget_fct.h"

uint32_t mtp_op_SendObject(mtp_ctx * ctx,MTP_PACKET_HEADER * mtp_packet_hdr, int * size,uint32_t * ret_params, int * ret_params_size)
{
	uint32_t response_code;
	fs_entry * entry;
	unsigned char * tmp_ptr;
	char * full_path;
	int file;
	int sz;

	if(!ctx->fs_db)
		return MTP_RESPONSE_SESSION_NOT_OPEN;

	pthread_mutex_lock( &ctx->inotify_mutex );

	response_code = MTP_RESPONSE_GENERAL_ERROR;

	if( mtp_packet_hdr->code == MTP_OPERATION_SEND_PARTIAL_OBJECT && mtp_packet_hdr->operation == MTP_CONTAINER_TYPE_COMMAND )
	{
		ctx->SendObjInfoHandle = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER), 4);         // Get param 1 - Object handle
		ctx->SendObjInfoOffset = peek64(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER) + 4, 8);   // Get param 2 - Offset in bytes
		ctx->SendObjInfoSize = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER) + 12, 4);      // Get param 3 - Max size in bytes
	}

	if( ctx->SendObjInfoHandle != 0xFFFFFFFF )
	{
		switch(mtp_packet_hdr->operation)
		{
			case MTP_CONTAINER_TYPE_DATA:
				entry = get_entry_by_handle(ctx->fs_db, ctx->SendObjInfoHandle);
				if(entry)
				{
					response_code = MTP_RESPONSE_GENERAL_ERROR;

					if( check_handle_access( ctx, entry, 0x00000000, 1, &response_code) )
					{
						pthread_mutex_unlock( &ctx->inotify_mutex );

						return response_code;
					}

					full_path = build_full_path(ctx->fs_db, mtp_get_storage_root(ctx, entry->storage_id), entry);
					if(full_path)
					{
						file = -1;

						if(!set_storage_giduid(ctx, entry->storage_id))
						{
							if( mtp_packet_hdr->code == MTP_OPERATION_SEND_PARTIAL_OBJECT )
								file = open(full_path,O_RDWR | O_LARGEFILE);
							else
								file = open(full_path,O_CREAT|O_WRONLY|O_TRUNC| O_LARGEFILE, S_IRUSR|S_IWUSR);
						}

						restore_giduid(ctx);

						if( file != -1 )
						{
							ctx->transferring_file_data = 1;

							lseek64(file, ctx->SendObjInfoOffset, SEEK_SET);

							sz = *size - sizeof(MTP_PACKET_HEADER);
							tmp_ptr = ((unsigned char*)mtp_packet_hdr) ;
							tmp_ptr += sizeof(MTP_PACKET_HEADER);

							if(sz > 0)
							{
								if( write(file, tmp_ptr, sz) != sz)
								{
									// TODO : Handle this error case properly
								}

								ctx->SendObjInfoSize -= sz;
							}

							tmp_ptr = ctx->rdbuffer2;
							if( sz == ( ctx->usb_rd_buffer_max_size - sizeof(MTP_PACKET_HEADER) ) )
							{
								sz = ctx->usb_rd_buffer_max_size;
							}

							if( mtp_packet_hdr->code == MTP_OPERATION_SEND_PARTIAL_OBJECT && ctx->SendObjInfoSize )
							{
								sz = ctx->usb_rd_buffer_max_size;
							}

							while( sz == ctx->usb_rd_buffer_max_size && !ctx->cancel_req)
							{
								sz = read_usb(ctx->usb_ctx, ctx->rdbuffer2, ctx->usb_rd_buffer_max_size);

								if( write(file, tmp_ptr, sz) != sz)
								{
									// TODO : Handle this error case properly
								}

								ctx->SendObjInfoSize -= sz;
							};

							entry->size = lseek64(file, 0, SEEK_END);

							ctx->transferring_file_data = 0;

							close(file);

							if(ctx->usb_cfg.val_umask >= 0)
								chmod(full_path, 0777 & (~ctx->usb_cfg.val_umask));

							if(ctx->cancel_req)
							{
								ctx->cancel_req = 0;

								free( full_path );

								pthread_mutex_unlock( &ctx->inotify_mutex );

								return MTP_RESPONSE_NO_RESPONSE;
							}

							response_code = MTP_RESPONSE_OK;
						}

						free( full_path );
					}
				}
				else
				{
					response_code = MTP_RESPONSE_INVALID_OBJECT_HANDLE;
				}
			break;
			case MTP_CONTAINER_TYPE_COMMAND:
				PRINT_DEBUG("SEND_OBJECT : Handle 0x%.8x, Offset 0x%"SIZEHEX", Size 0x%"SIZEHEX,ctx->SendObjInfoHandle,ctx->SendObjInfoOffset,ctx->SendObjInfoSize);

				if( check_handle_access( ctx, NULL, ctx->SendObjInfoHandle, 1, &response_code) )
				{
					pthread_mutex_unlock( &ctx->inotify_mutex );

					return response_code;
				}

				// no response to send, wait for the data...
				response_code = MTP_RESPONSE_NO_RESPONSE;
			break;
			default:
			break;
		}
	}
	else
	{
		PRINT_WARN("MTP_OPERATION_SEND_OBJECT ! : Invalid Handle (0x%.8X)", ctx->SendObjInfoHandle);

		response_code = MTP_RESPONSE_INVALID_OBJECT_HANDLE;
	}

	pthread_mutex_unlock( &ctx->inotify_mutex );

	return response_code;
}
