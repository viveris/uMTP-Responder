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
 * @file   mtp_ops_helpers.c
 * @brief  mtp operations helpers
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include "buildconf.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mtp.h"
#include "mtp_helpers.h"
#include "mtp_constant.h"
#include "mtp_operations.h"
#include "usb_gadget_fct.h"
#include "inotify.h"

#include "logs_out.h"

mtp_size send_file_data( mtp_ctx * ctx, fs_entry * entry,mtp_offset offset, mtp_size maxsize )
{
	mtp_size actualsize;
	mtp_size j;
	int ofs;
	mtp_size blocksize;
	int file,bytes_read;
	mtp_offset buf_index;
	int io_buffer_index;
	int first_part_size;
	unsigned char * usb_buffer_ptr;

	if( !ctx->read_file_buffer )
	{
		ctx->read_file_buffer = malloc( ctx->read_file_buffer_size );
		if(!ctx->read_file_buffer)
			return 0;

		memset(ctx->read_file_buffer, 0, ctx->read_file_buffer_size);
	}

	usb_buffer_ptr = NULL;

	buf_index = -1;

	if( offset >= entry->size )
	{
		actualsize = 0;
	}
	else
	{
		if( offset + maxsize > entry->size )
			actualsize = entry->size - offset;
		else
			actualsize = maxsize;
	}

	poke32(ctx->wrbuffer, 0, ctx->usb_wr_buffer_max_size, sizeof(MTP_PACKET_HEADER) + actualsize);

	ofs = sizeof(MTP_PACKET_HEADER);

	PRINT_DEBUG("send_file_data : Offset 0x%"SIZEHEX" - Maxsize 0x%"SIZEHEX" - Size 0x%"SIZEHEX" - ActualSize 0x%"SIZEHEX, offset,maxsize,entry->size,actualsize);

	file = entry_open(ctx->fs_db, entry);
	if( file != -1 )
	{
		ctx->transferring_file_data = 1;

		j = 0;
		do
		{
			if((j + ((mtp_size)(ctx->usb_wr_buffer_max_size) - ofs)) < actualsize)
				blocksize = ((mtp_size)(ctx->usb_wr_buffer_max_size) - ofs);
			else
				blocksize = actualsize - j;

			// Is the target page loaded ?
			if( buf_index != ((offset + j) & ~((mtp_offset)(ctx->read_file_buffer_size-1))) )
			{
				bytes_read = entry_read(ctx->fs_db, file, ctx->read_file_buffer, ((offset + j) & ~((mtp_offset)(ctx->read_file_buffer_size-1))) , (mtp_size)ctx->read_file_buffer_size);
				if( bytes_read < 0 )
				{
					entry_close( file );
					return -1;
				}

				buf_index = ((offset + j) & ~((mtp_offset)(ctx->read_file_buffer_size-1)));
			}

			io_buffer_index = (offset + j) & (mtp_offset)(ctx->read_file_buffer_size-1);

			// Is a new page needed ?
			if( io_buffer_index + blocksize < ctx->read_file_buffer_size )
			{
				// No, just use the io buffer

				if( !ofs )
				{
					// Use the I/O buffer directly
					usb_buffer_ptr = (unsigned char *)&ctx->read_file_buffer[io_buffer_index];
				}
				else
				{
					memcpy(&ctx->wrbuffer[ofs], &ctx->read_file_buffer[io_buffer_index], blocksize );
					usb_buffer_ptr = (unsigned char *)&ctx->wrbuffer[0];
				}
			}
			else
			{
				// Yes, new page needed. Get the first part in the io buffer and the load a new page to get the remaining data.
				first_part_size = blocksize - ( ( io_buffer_index + blocksize ) - (mtp_size)ctx->read_file_buffer_size);

				memcpy(&ctx->wrbuffer[ofs], &ctx->read_file_buffer[io_buffer_index], first_part_size  );

				buf_index += (mtp_offset)ctx->read_file_buffer_size;
				bytes_read = entry_read(ctx->fs_db, file, ctx->read_file_buffer, buf_index , ctx->read_file_buffer_size);
				if( bytes_read < 0 )
				{
					entry_close( file );
					return -1;
				}

				memcpy(&ctx->wrbuffer[ofs + first_part_size], &ctx->read_file_buffer[0], blocksize - first_part_size );

				usb_buffer_ptr = (unsigned char *)&ctx->wrbuffer[0];
			}

			j   += blocksize;
			ofs += blocksize;

			PRINT_DEBUG("---> 0x%"SIZEHEX" (0x%X)",j,ofs);

			write_usb(ctx->usb_ctx, EP_DESCRIPTOR_IN, usb_buffer_ptr, ofs);

			ofs = 0;

		}while( j < actualsize && !ctx->cancel_req );

		ctx->transferring_file_data = 0;

		entry_close( file );

		if( ctx->cancel_req )
		{
			PRINT_DEBUG("send_file_data : Cancelled ! Aborded...");

			// Force a ZLP
			check_and_send_USB_ZLP(ctx , ctx->max_packet_size );

			actualsize = -2;
			ctx->cancel_req = 0;
		}
		else
		{
			PRINT_DEBUG("send_file_data : Full transfert done !");

			check_and_send_USB_ZLP(ctx , sizeof(MTP_PACKET_HEADER) + actualsize );
		}

	}
	else
		actualsize = -1;

	return actualsize;
}

int delete_tree(mtp_ctx * ctx,uint32_t handle)
{
	int ret;
	fs_entry * entry;
	char * path;
	ret = -1;

	entry = get_entry_by_handle(ctx->fs_db, handle);
	if(entry)
	{
		path = build_full_path(ctx->fs_db, mtp_get_storage_root(ctx, entry->storage_id), entry);

		if (path)
		{
			if(entry->flags & ENTRY_IS_DIR)
			{
				ret = fs_remove_tree( path );

				if(!ret)
				{
					entry->flags |= ENTRY_IS_DELETED;
					if( entry->watch_descriptor != -1 )
					{
						inotify_handler_rmwatch( ctx, entry->watch_descriptor );
						entry->watch_descriptor = -1;
					}
				}
				else
					scan_and_add_folder(ctx->fs_db, path, handle, entry->storage_id); // partially deleted ? update/sync the db.
			}
			else
			{
				ret = remove(path);
				if(!ret)
				{
					entry->flags |= ENTRY_IS_DELETED;
					if( entry->watch_descriptor != -1 )
					{
						inotify_handler_rmwatch( ctx, entry->watch_descriptor );
						entry->watch_descriptor = -1;
					}
				}
			}

			free(path);
		}
	}

	return ret;
}

int umount_store(mtp_ctx * ctx, int store_index, int update_flag)
{
	fs_entry * entry;

	if(store_index >= 0 && store_index < MAX_STORAGE_NB )
	{
		if( ctx->storages[store_index].root_path )
		{
			entry = NULL;
			do
			{
				entry = get_entry_by_storageid( ctx->fs_db, ctx->storages[store_index].storage_id, entry );
				if(entry)
				{
					entry->flags |= ENTRY_IS_DELETED;
					if( entry->watch_descriptor != -1 )
					{
						inotify_handler_rmwatch( ctx, entry->watch_descriptor );
						entry->watch_descriptor = -1;
					}
					entry = entry->next;
				}
			}while(entry);

			if( update_flag )
				ctx->storages[store_index].flags |= UMTP_STORAGE_NOTMOUNTED;
		}
	}

	return 0;
}

int mount_store(mtp_ctx * ctx, int store_index, int update_flag)
{
	if(store_index >= 0 && store_index < MAX_STORAGE_NB )
	{
		if( ctx->storages[store_index].root_path )
		{
			if( ctx->storages[store_index].flags & UMTP_STORAGE_NOTMOUNTED )
			{
				alloc_root_entry(ctx->fs_db, ctx->storages[store_index].storage_id);
			}

			if( update_flag )
				ctx->storages[store_index].flags &= ~UMTP_STORAGE_NOTMOUNTED;
		}
	}

	return 0;
}
