/*
 * uMTP Responder
 * Copyright (c) 2018 - 2019 Viveris Technologies
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
 * @file   mtp.c
 * @brief  Main MTP protocol functions.
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <errno.h>

#include <sys/stat.h>

#include <sys/types.h>

#include <pthread.h>

#include "logs_out.h"

#include "mtp_helpers.h"

#include "fs_handles_db.h"

#include "mtp.h"
#include "mtp_datasets.h"

#include "usb_gadget_fct.h"

#include "mtp_constant.h"
#include "mtp_constant_strings.h"

#include "mtp_support_def.h"

#include "inotify.h"

#define MAX_RX_BUFFER_SIZE (2*1024)
#define MAX_TX_BUFFER_SIZE (2*1024)

mtp_ctx * mtp_init_responder()
{
	mtp_ctx * ctx;

	PRINT_DEBUG("init_mtp_responder");

	ctx = malloc(sizeof(mtp_ctx));
	if(ctx)
	{
		memset(ctx,0,sizeof(mtp_ctx));

		ctx->wrbuffer = malloc(MAX_TX_BUFFER_SIZE);
		if(!ctx->wrbuffer)
			goto init_error;

		ctx->rdbuffer = malloc( MAX_RX_BUFFER_SIZE );
		if(!ctx->rdbuffer)
			goto init_error;

		ctx->rdbuffer2 = malloc( MAX_RX_BUFFER_SIZE );
		if(!ctx->rdbuffer2)
			goto init_error;

		ctx->temp_array = malloc( MAX_STORAGE_NB * sizeof(uint32_t) );
		if(!ctx->temp_array)
			goto init_error;

		pthread_mutex_init ( &ctx->inotify_mutex, NULL);

		inotify_handler_init( ctx );

		PRINT_DEBUG("init_mtp_responder : Ok !");

		return ctx;
	}

init_error:
	if(ctx)
	{
		if(ctx->wrbuffer)
			free(ctx->wrbuffer);

		if(ctx->rdbuffer)
			free(ctx->rdbuffer);

		if(ctx->rdbuffer2)
			free(ctx->rdbuffer2);

		if(ctx->temp_array)
			free(ctx->temp_array);

		free(ctx);
	}

	PRINT_ERROR("init_mtp_responder : Failed !");

	return 0;
}

void mtp_deinit_responder(mtp_ctx * ctx)
{
	if( ctx )
	{
		inotify_handler_deinit( ctx );

		if(ctx->wrbuffer)
			free(ctx->wrbuffer);

		if(ctx->rdbuffer)
			free(ctx->rdbuffer);

		if(ctx->rdbuffer2)
			free(ctx->rdbuffer2);

		if(ctx->temp_array)
			free(ctx->temp_array);

		free(ctx);
	}
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

int build_response(mtp_ctx * ctx, uint32_t tx_id, uint16_t type, uint16_t status, void * buffer,void * datain,int size)
{
	MTP_PACKET_HEADER tmp_hdr;
	int ofs;

	ofs = 0;

	tmp_hdr.length = sizeof(tmp_hdr) + size;
	tmp_hdr.operation = type;
	tmp_hdr.code = status;
	tmp_hdr.tx_id = tx_id;

	poke_array(buffer, &ofs, sizeof(MTP_PACKET_HEADER), 1, (unsigned char*)&tmp_hdr,0);
	if(size)
		poke_array(buffer, &ofs, size, 1, (unsigned char*)datain,0);

	return ofs;
}

int parse_incomming_dataset(mtp_ctx * ctx,void * datain,int size,uint32_t * newhandle, uint32_t parent_handle, uint32_t storage_id)
{
	MTP_PACKET_HEADER * tmp_hdr;
	unsigned char *dataset_ptr;
	uint32_t objectformat,objectsize;
#ifdef DEBUG
	uint32_t type;
#endif
	unsigned char string_len;
	char tmp_str[256+1];
	char * parent_folder;
	char * tmp_path;
	int i,ret_code;
	fs_entry * entry;
	FILE * f;
	filefoundinfo tmp_file_entry;


	ret_code = MTP_RESPONSE_GENERAL_ERROR;

	tmp_hdr = (MTP_PACKET_HEADER *)datain;

	if(parent_handle == 0xFFFFFFFF)
		parent_handle = 0x00000000;

	PRINT_DEBUG("Incoming dataset : %d bytes (raw) %d bytes, operation 0x%x, code 0x%x, tx_id: %x",size,tmp_hdr->length,tmp_hdr->operation,tmp_hdr->code ,tmp_hdr->tx_id );

	dataset_ptr = (datain + sizeof(MTP_PACKET_HEADER));

	switch( tmp_hdr->code )
	{
		case MTP_OPERATION_SEND_OBJECT_INFO:
			objectformat = peek(dataset_ptr, 0x04, 2);     // ObjectFormat Code
			if(objectformat==MTP_FORMAT_ASSOCIATION)
			{
				objectsize = peek(dataset_ptr, 0x08, 4);   // Object Compressed Size
				//parent = peek(dataset_ptr,0x26, 4);      // Parent Object (NR)
#ifdef DEBUG
				type = peek(dataset_ptr,0x2A, 2);          // Association Type
#else
				peek(dataset_ptr,0x2A, 2);          // Association Type
#endif

				string_len = peek(dataset_ptr,0x34, 1);    // File name

				if(string_len > 255)
					string_len = 255;

				memset(tmp_str,0,sizeof(tmp_str));

				for(i=0;i<string_len;i++)
				{
					tmp_str[i] = peek(dataset_ptr,0x35 + (i*2), 1);
				}

				PRINT_DEBUG("MTP_OPERATION_SEND_OBJECT_INFO : 0x%x objectformat Size %d, Parent 0x%.8x, type: %x, strlen %d str:%s",objectformat,objectsize,parent_handle,type,string_len,tmp_str);

				entry = get_entry_by_handle_and_storageid(ctx->fs_db, parent_handle,storage_id);
				if(entry)
				{
					if(entry->flags & ENTRY_IS_DIR)
					{
						tmp_path = 0;

						parent_folder = build_full_path(ctx->fs_db, mtp_get_storage_root(ctx, entry->storage_id), entry);

						if(parent_folder)
						{
							PRINT_DEBUG("MTP_OPERATION_SEND_OBJECT_INFO : Parent folder %s",parent_folder);

							tmp_path = malloc(strlen(parent_folder) + 1 + strlen(tmp_str) + 1);
						}

						if(tmp_path)
						{
							sprintf(tmp_path,"%s/%s",parent_folder,tmp_str);
							PRINT_DEBUG("MTP_OPERATION_SEND_OBJECT_INFO : Creating %s ...",tmp_path);

							if( mkdir(tmp_path, 0700) )
							{
								PRINT_WARN("MTP_OPERATION_SEND_OBJECT_INFO : Can't create %s ...",tmp_path);

								if(parent_folder)
									free(parent_folder);

								free(tmp_path);

								ret_code = MTP_RESPONSE_ACCESS_DENIED;

								return ret_code;
							}

							tmp_file_entry.isdirectory = 1;
							strcpy(tmp_file_entry.filename,tmp_str);
							tmp_file_entry.size = 0;

							entry = add_entry(ctx->fs_db, &tmp_file_entry, parent_handle, storage_id);

							if(entry)
							{
								*newhandle = entry->handle;
							}

							free(tmp_path);

							ret_code = MTP_RESPONSE_OK;
						}

						if(parent_folder)
							free(parent_folder);
					}
				}
			}
			else
			{
				objectsize = peek(dataset_ptr, 0x08, 4);   // Object Compressed Size
				//parent = peek(dataset_ptr,0x26, 4);      // Parent Object (NR)
#ifdef DEBUG
				type = peek(dataset_ptr,0x2A, 2);          // Association Type
#else
				peek(dataset_ptr,0x2A, 2);                 // Association Type
#endif

				string_len = peek(dataset_ptr,0x34, 1);    // File name

				memset(tmp_str,0,sizeof(tmp_str));

				for(i=0;i<string_len;i++)
				{
					tmp_str[i] = peek(dataset_ptr,0x35 + (i*2), 1);
				}

				PRINT_DEBUG("MTP_OPERATION_SEND_OBJECT_INFO : 0x%x objectformat Size %d, Parent 0x%.8x, type: %x, strlen %d str:%s",objectformat,objectsize,parent_handle,type,string_len,tmp_str);

				entry = get_entry_by_handle_and_storageid(ctx->fs_db, parent_handle,storage_id);
				if(entry)
				{
					if(entry->flags & ENTRY_IS_DIR)
					{
						parent_folder = build_full_path(ctx->fs_db, mtp_get_storage_root(ctx, entry->storage_id), entry);

						tmp_path = 0;
						entry = 0;

						if(parent_folder)
						{
							PRINT_DEBUG("MTP_OPERATION_SEND_OBJECT_INFO : Parent folder %s",parent_folder);
							tmp_path = malloc(strlen(parent_folder) + 1 + strlen(tmp_str) + 1);
						}

						if( tmp_path )
						{
							sprintf(tmp_path,"%s/%s",parent_folder,tmp_str);
							PRINT_DEBUG("MTP_OPERATION_SEND_OBJECT_INFO : Creating %s ...",tmp_path);

							tmp_file_entry.isdirectory = 0;
							strcpy(tmp_file_entry.filename,tmp_str);
							tmp_file_entry.size = objectsize;

							f = fopen(tmp_path,"wb");
							if(!f)
							{
								PRINT_WARN("MTP_OPERATION_SEND_OBJECT_INFO : Can't create %s ...",tmp_path);

								if(parent_folder)
									free(parent_folder);

								free(tmp_path);

								ret_code = MTP_RESPONSE_ACCESS_DENIED;

								return ret_code;
							}
							fclose(f);

							entry = add_entry(ctx->fs_db, &tmp_file_entry, parent_handle, storage_id);

							free(tmp_path);
						}

						if(entry)
						{
							ctx->SendObjInfoHandle = entry->handle;
							ctx->SendObjInfoSize = objectsize;
							*newhandle = entry->handle;
						}
						else
						{
							ctx->SendObjInfoHandle = 0xFFFFFFFF;
							ctx->SendObjInfoSize = 0x00000000;
						}

						if( parent_folder )
							free(parent_folder);

						ret_code = MTP_RESPONSE_OK;
					}
				}
			}

		break;

		default :
		break;
	}

	return MTP_RESPONSE_OK;
}

int check_and_send_USB_ZLP(mtp_ctx * ctx , int size)
{
	// USB ZLP needed ?
	if( (size >= ctx->max_packet_size) && !(size % ctx->max_packet_size) )
	{
		PRINT_DEBUG("%d bytes transfert ended - ZLP packet needed", size);

		// Yes - Send zero lenght packet.
		write_usb(ctx->usb_ctx,EP_DESCRIPTOR_IN,ctx->wrbuffer,0);

		return 1;
	}

	return 0;
}

int send_file_data( mtp_ctx * ctx, fs_entry * entry,uint32_t offset, uint32_t maxsize )
{
	int actualsize;
	int j,k,ofs;
	int blocksize;
	FILE *f;

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

	ofs = 0;
	poke(ctx->wrbuffer, &ofs, 4, sizeof(MTP_PACKET_HEADER) + actualsize);

	ofs = sizeof(MTP_PACKET_HEADER);

	k = ctx->max_packet_size;

	PRINT_DEBUG("send_file_data : Offset 0x%.8X - Maxsize 0x%.8X - Size 0x%.8X - ActualSize 0x%.8X", offset,maxsize,entry->size,actualsize);

	f = entry_open(ctx->fs_db, entry);
	if( f )
	{
		j = 0;
		do
		{
			if((j + (k - ofs)) < actualsize)
				blocksize = (k - ofs);
			else
				blocksize = actualsize - j;

			entry_read(ctx->fs_db, f, &ctx->wrbuffer[ofs], offset + j, blocksize);
			j   += blocksize;
			ofs += blocksize;

			PRINT_DEBUG("---> %d (%d)",j,ofs);

			write_usb(ctx->usb_ctx,EP_DESCRIPTOR_IN,ctx->wrbuffer,ofs);

			ofs = 0;

		}while( j < actualsize );

		check_and_send_USB_ZLP(ctx , sizeof(MTP_PACKET_HEADER) + actualsize );

		entry_close( f );
	}
	else
		actualsize = -1;

	return actualsize;
}

int process_in_packet(mtp_ctx * ctx,MTP_PACKET_HEADER * mtp_packet_hdr, int rawsize)
{
	fs_entry * entry;
	uint32_t params[5],storageid;
	uint32_t handle,parent_handle,new_handle;
	uint32_t response_code;
	int i,size,offset,maxsize,actualsize;
	int handle_index;
	int nb_of_handles;
	int ofs, no_response;
	unsigned char * tmp_ptr;
	char * full_path;
	char * tmp_str;
	FILE * f;

	params[0] = 0x000000;
	params[1] = 0x000000;
	params[2] = 0x000000;
	params[3] = 0x000000;
	params[4] = 0x000000;

	no_response = 0;

	switch( mtp_packet_hdr->code )
	{

		case MTP_OPERATION_OPEN_SESSION:

			ctx->session_id = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER), 4); // Get param 1

			ctx->fs_db = init_fs_db(ctx);

			i = 0;
			while( (i < MAX_STORAGE_NB) && ctx->storages[i].root_path)
			{
				pthread_mutex_lock( &ctx->inotify_mutex );
				alloc_root_entry(ctx->fs_db, ctx->storages[i].storage_id);
				pthread_mutex_unlock( &ctx->inotify_mutex );

				i++;
			}

			PRINT_DEBUG("Open session - ID 0x%.8x",ctx->session_id);

			response_code = MTP_RESPONSE_OK;

		break;

		case MTP_OPERATION_CLOSE_SESSION:

			deinit_fs_db(ctx->fs_db);

			ctx->fs_db = 0;

			response_code = MTP_RESPONSE_OK;

		break;

		case MTP_OPERATION_GET_DEVICE_INFO:

			size = build_response(ctx, mtp_packet_hdr->tx_id, MTP_CONTAINER_TYPE_DATA, mtp_packet_hdr->code, ctx->wrbuffer,0,0);
			size += build_deviceinfo_dataset(ctx, ctx->wrbuffer + sizeof(MTP_PACKET_HEADER), 2048);

			// Update packet size
			ofs = 0;
			poke(ctx->wrbuffer, &ofs, 4, size);

			PRINT_DEBUG("MTP_OPERATION_GET_DEVICE_INFO response (%d Bytes):",size);
			PRINT_DEBUG_BUF(ctx->wrbuffer, size);

			write_usb(ctx->usb_ctx,EP_DESCRIPTOR_IN,ctx->wrbuffer,size);

			check_and_send_USB_ZLP(ctx , size );

			response_code = MTP_RESPONSE_OK;

		break;

		case MTP_OPERATION_GET_STORAGE_IDS:

			ofs = build_response(ctx, mtp_packet_hdr->tx_id, MTP_CONTAINER_TYPE_DATA, mtp_packet_hdr->code, ctx->wrbuffer,0,0);

			i = 0;
			while( (i < MAX_STORAGE_NB) && ctx->storages[i].root_path)
			{
				ctx->temp_array[i] = ctx->storages[i].storage_id; // Storage ID
				i++;
			}

			poke_array(ctx->wrbuffer, &ofs, 4 * i, 4, (void*)ctx->temp_array, 1);
			size = ofs;

			// Update packet size
			ofs = 0;
			poke(ctx->wrbuffer, &ofs, 4, size);

			PRINT_DEBUG("MTP_OPERATION_GET_STORAGE_IDS response (%d Bytes):",size);
			PRINT_DEBUG_BUF(ctx->wrbuffer, size);

			write_usb(ctx->usb_ctx,EP_DESCRIPTOR_IN,ctx->wrbuffer,size);

			check_and_send_USB_ZLP(ctx , size );

			response_code = MTP_RESPONSE_OK;

		break;

		case MTP_OPERATION_GET_STORAGE_INFO:

			storageid = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER) + 0, 4); // Get param 1 - Storage ID
			if( mtp_get_storage_root(ctx,storageid) )
			{
				size = build_response(ctx, mtp_packet_hdr->tx_id, MTP_CONTAINER_TYPE_DATA, mtp_packet_hdr->code, ctx->wrbuffer,0,0);

				size += build_storageinfo_dataset(ctx, ctx->wrbuffer + sizeof(MTP_PACKET_HEADER), 2048,storageid);

				// Update packet size
				ofs = 0;
				poke(ctx->wrbuffer, &ofs, 4, size);

				PRINT_DEBUG("MTP_OPERATION_GET_STORAGE_INFO response (%d Bytes):",size);
				PRINT_DEBUG_BUF(ctx->wrbuffer, size);

				write_usb(ctx->usb_ctx,EP_DESCRIPTOR_IN,ctx->wrbuffer,size);

				check_and_send_USB_ZLP(ctx , size );

				response_code = MTP_RESPONSE_OK;
			}
			else
			{
				response_code = MTP_RESPONSE_INVALID_STORAGE_ID;
			}
		break;

		case MTP_OPERATION_GET_OBJECT_HANDLES:

			pthread_mutex_lock( &ctx->inotify_mutex );

			storageid = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER) + 0, 4);        // Get param 1 - Storage ID

			i = build_response(ctx, mtp_packet_hdr->tx_id, MTP_CONTAINER_TYPE_DATA, mtp_packet_hdr->code, ctx->wrbuffer,0,0);

			parent_handle = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER)+ 8, 4);     // Get param 3 - parent handle

			PRINT_DEBUG("MTP_OPERATION_GET_OBJECT_HANDLES - Parent Handle 0x%.8x, Storage ID 0x%.8x",parent_handle,storageid);

			if(!mtp_get_storage_root(ctx,storageid))
			{
				PRINT_WARN("MTP_OPERATION_GET_OBJECT_HANDLES : INVALID STORAGE ID!");

				response_code = MTP_RESPONSE_INVALID_STORAGE_ID;
				break;
			}

			tmp_str = 0;
			full_path = 0;
			entry = 0;
			if(parent_handle && parent_handle!=0xFFFFFFFF)
			{
				entry = get_entry_by_handle(ctx->fs_db, parent_handle);
				if(entry)
				{
					tmp_str = build_full_path(ctx->fs_db, mtp_get_storage_root(ctx, entry->storage_id), entry);
					full_path = tmp_str;
				}
			}
			else
			{
				// root folder
				parent_handle = 0x00000000;
				full_path = mtp_get_storage_root(ctx,storageid);
				entry = get_entry_by_handle(ctx->fs_db, parent_handle);
			}

			nb_of_handles = 0;

			if( full_path )
			{
				// Count the number of files...
				scan_and_add_folder(ctx->fs_db, full_path, parent_handle, storageid);
				init_search_handle(ctx->fs_db, parent_handle, storageid);

				while( get_next_child_handle(ctx->fs_db) )
				{
					nb_of_handles++;
				}

				// Restart
				init_search_handle(ctx->fs_db, parent_handle, storageid);

				// Register a watch point.
				if( entry )
				{
					if ( entry->flags & ENTRY_IS_DIR )
					{
						entry->watch_descriptor = inotify_handler_addwatch( ctx, full_path );
					}
				}

				if (tmp_str)
					free(tmp_str);
			}

			// Update packet size
			ofs = 0;
			poke(ctx->wrbuffer, &ofs, 4, sizeof(MTP_PACKET_HEADER) + ((1+nb_of_handles)*4) );

			// Build and send the handles array
			ofs = sizeof(MTP_PACKET_HEADER);

			poke(ctx->wrbuffer, &ofs, 4, nb_of_handles);

			PRINT_DEBUG("MTP_OPERATION_GET_OBJECT_HANDLES response :");

			handle_index = 0;
			do
			{
				do
				{
					entry = get_next_child_handle(ctx->fs_db);
					if(entry)
					{
						PRINT_DEBUG("File : %s Handle:%.8x",entry->name,entry->handle);
						poke(ctx->wrbuffer, &ofs, 4, entry->handle);

						handle_index++;
					}
				}while( ofs < ctx->max_packet_size && handle_index < nb_of_handles);

				PRINT_DEBUG_BUF(ctx->wrbuffer, ofs);

				// Current usb packet full, need to send it.
				write_usb(ctx->usb_ctx,EP_DESCRIPTOR_IN,ctx->wrbuffer,ofs);

				ofs = 0;

			}while( handle_index < nb_of_handles);

			// Total size = Header size + nb of handles field (uint32_t) + all handles
			check_and_send_USB_ZLP(ctx , sizeof(MTP_PACKET_HEADER) + sizeof(uint32_t) + (nb_of_handles * sizeof(uint32_t)) );

			pthread_mutex_unlock( &ctx->inotify_mutex );

			response_code = MTP_RESPONSE_OK;

		break;

		case MTP_OPERATION_GET_OBJECT_INFO:

			pthread_mutex_lock( &ctx->inotify_mutex );

			handle = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER), 4); // Get param 1 - object handle
			entry = get_entry_by_handle(ctx->fs_db, handle);
			if( entry )
			{
				size = build_response(ctx, mtp_packet_hdr->tx_id, MTP_CONTAINER_TYPE_DATA, mtp_packet_hdr->code, ctx->wrbuffer,0,0);
				size += build_objectinfo_dataset(ctx, ctx->wrbuffer + sizeof(MTP_PACKET_HEADER),2048,entry);

				i = 0;
				poke(ctx->wrbuffer, &i, 4, size);

				PRINT_DEBUG("MTP_OPERATION_GET_OBJECT_INFO response (%d Bytes):",size);
				PRINT_DEBUG_BUF(ctx->wrbuffer, size);

				write_usb(ctx->usb_ctx,EP_DESCRIPTOR_IN,ctx->wrbuffer,size);

				check_and_send_USB_ZLP(ctx , size );

				response_code = MTP_RESPONSE_OK;
			}
			else
			{
				PRINT_WARN("MTP_OPERATION_GET_OBJECT_INFO ! : Entry/Handle not found (0x%.8X)", handle);

				response_code = MTP_RESPONSE_INVALID_OBJECT_HANDLE;
			}

			pthread_mutex_unlock( &ctx->inotify_mutex );

		break;

		case MTP_OPERATION_GET_PARTIAL_OBJECT:

			pthread_mutex_lock( &ctx->inotify_mutex );

			handle = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER), 4);      // Get param 1 - Object handle
			offset = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER) + 4, 4);  // Get param 2 - Offset in bytes
			maxsize = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER) + 8, 4); // Get param 2 - Max size in bytes
			entry = get_entry_by_handle(ctx->fs_db, handle);

			PRINT_DEBUG("MTP_OPERATION_GET_PARTIAL_OBJECT : handle 0x%.8X - Offset 0x%.8X - Maxsize 0x%.8X", handle,offset,maxsize);

			if(entry)
			{
				size = build_response(ctx, mtp_packet_hdr->tx_id, MTP_CONTAINER_TYPE_DATA, mtp_packet_hdr->code, ctx->wrbuffer,0,0);

				actualsize = send_file_data( ctx, entry, offset, maxsize );
				if( actualsize >= 0 )
				{
					params[0] = actualsize;
				}

				response_code = MTP_RESPONSE_OK;
			}
			else
			{
				PRINT_WARN("MTP_OPERATION_GET_PARTIAL_OBJECT ! : Entry/Handle not found (0x%.8X)", handle);

				response_code = MTP_RESPONSE_INVALID_OBJECT_HANDLE;
			}

			pthread_mutex_unlock( &ctx->inotify_mutex );

		break;

		case MTP_OPERATION_GET_OBJECT:

			pthread_mutex_lock( &ctx->inotify_mutex );

			handle = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER), 4); // Get param 1 - object handle
			entry = get_entry_by_handle(ctx->fs_db, handle);
			if(entry)
			{
				size = build_response(ctx, mtp_packet_hdr->tx_id, MTP_CONTAINER_TYPE_DATA, mtp_packet_hdr->code, ctx->wrbuffer,0,0);

				actualsize = send_file_data( ctx, entry, 0, entry->size );

				response_code = MTP_RESPONSE_OK;
			}
			else
			{
				PRINT_WARN("MTP_OPERATION_GET_OBJECT ! : Entry/Handle not found (0x%.8X)", handle);

				response_code = MTP_RESPONSE_INVALID_OBJECT_HANDLE;
			}

			pthread_mutex_unlock( &ctx->inotify_mutex );

		break;

		case MTP_OPERATION_SEND_OBJECT_INFO:

			pthread_mutex_lock( &ctx->inotify_mutex );

			storageid = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER), 4);         // Get param 1 - storage id
			parent_handle = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER) + 4, 4); // Get param 2 - parent handle

			PRINT_DEBUG("MTP_OPERATION_SEND_OBJECT_INFO : Rx dataset...");

			size = read_usb(ctx->usb_ctx, ctx->rdbuffer2, MAX_RX_BUFFER_SIZE);
			PRINT_DEBUG_BUF(ctx->rdbuffer2, size);

			response_code = parse_incomming_dataset(ctx,ctx->rdbuffer2,size,&new_handle,parent_handle,storageid);
			if( response_code == MTP_RESPONSE_OK )
			{
				PRINT_DEBUG("MTP_OPERATION_SEND_OBJECT_INFO : Response - storageid: 0x%.8X, parent_handle: 0x%.8X, new_handle: 0x%.8X ",storageid,parent_handle,new_handle);
				params[0] = storageid;
				params[1] = parent_handle;
				params[2] = new_handle;
			}

			pthread_mutex_unlock( &ctx->inotify_mutex );

		break;

		case MTP_OPERATION_SEND_OBJECT:

			pthread_mutex_lock( &ctx->inotify_mutex );

			response_code = MTP_RESPONSE_GENERAL_ERROR;

			if( ctx->SendObjInfoHandle != 0xFFFFFFFF )
			{
				switch(mtp_packet_hdr->operation)
				{
					case MTP_CONTAINER_TYPE_DATA:
						entry = get_entry_by_handle(ctx->fs_db, ctx->SendObjInfoHandle);
						if(entry)
						{
							full_path = build_full_path(ctx->fs_db, mtp_get_storage_root(ctx, entry->storage_id), entry);
							if(full_path)
							{
								f = fopen(full_path,"wb");
								if(f)
								{
									size = rawsize - sizeof(MTP_PACKET_HEADER);
									tmp_ptr = ((unsigned char*)mtp_packet_hdr) ;
									tmp_ptr += sizeof(MTP_PACKET_HEADER);

									if(size)
									{
										fwrite(tmp_ptr,size, 1 ,f);
										ctx->SendObjInfoSize -= size;
									}

									tmp_ptr = ctx->rdbuffer2;
									if( size == ( MAX_RX_BUFFER_SIZE - sizeof(MTP_PACKET_HEADER) ) )
									{
										size = MAX_RX_BUFFER_SIZE;
									}

									while( size == MAX_RX_BUFFER_SIZE )
									{
										size = read_usb(ctx->usb_ctx, ctx->rdbuffer2, MAX_RX_BUFFER_SIZE);

										fwrite(tmp_ptr,size, 1 ,f);

										ctx->SendObjInfoSize -= size;
									};

									fclose(f);

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
						// no response to send, wait for the data...
						response_code = MTP_RESPONSE_OK;
						no_response = 1;
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

		break;

		case MTP_OPERATION_DELETE_OBJECT:

			pthread_mutex_lock( &ctx->inotify_mutex );

			handle = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER), 4); // Get param 1 - object handle

			if(delete_tree(ctx, handle))
			{
				response_code = MTP_RESPONSE_OBJECT_WRITE_PROTECTED;
			}
			else
			{
				response_code = MTP_RESPONSE_OK;
			}

			pthread_mutex_unlock( &ctx->inotify_mutex );

		break;

		default:

			PRINT_WARN("MTP code unsupported ! : 0x%.4X (%s)", mtp_packet_hdr->code,mtp_get_operation_string(mtp_packet_hdr->code));

			response_code = MTP_RESPONSE_OPERATION_NOT_SUPPORTED;

		break;
	}

	// Send the status response
	if(!no_response)
	{
		size = build_response(ctx, mtp_packet_hdr->tx_id,MTP_CONTAINER_TYPE_RESPONSE, response_code, ctx->wrbuffer,&params,sizeof(params));

		PRINT_DEBUG("Status response (%d Bytes):",size);
		PRINT_DEBUG_BUF(ctx->wrbuffer, size);

		write_usb(ctx->usb_ctx,EP_DESCRIPTOR_IN,ctx->wrbuffer,size);
	}

	return 0; // TODO Return usb error code.
}

int mtp_incoming_packet(mtp_ctx * ctx)
{
	int size;
	MTP_PACKET_HEADER * mtp_packet_hdr;

	if(!ctx)
		return 0;

	size = read_usb(ctx->usb_ctx, ctx->rdbuffer, MAX_RX_BUFFER_SIZE);

	if(size>=0)
	{
		PRINT_DEBUG("incoming_packet : %p - rawsize : %d",ctx->rdbuffer,size);

		if(!size)
			return 0; // ZLP

		mtp_packet_hdr = (MTP_PACKET_HEADER *)ctx->rdbuffer;

		PRINT_DEBUG("MTP Packet size : %d bytes", mtp_packet_hdr->length);
		PRINT_DEBUG("MTP Operation   : 0x%.4X (%s)", mtp_packet_hdr->operation, mtp_get_type_string(mtp_packet_hdr->operation) );
		PRINT_DEBUG("MTP code        : 0x%.4X (%s)", mtp_packet_hdr->code,mtp_get_operation_string(mtp_packet_hdr->code));
		PRINT_DEBUG("MTP Tx ID       : 0x%.8X", mtp_packet_hdr->tx_id);

		if( mtp_packet_hdr->length != size )
		{
			PRINT_DEBUG("Header Packet size and rawsize are not equal !: %d != %d",mtp_packet_hdr->length,size);
		}

		PRINT_DEBUG("Header : ");
		PRINT_DEBUG_BUF(ctx->rdbuffer, sizeof(MTP_PACKET_HEADER));

		PRINT_DEBUG("Payload : ");
		PRINT_DEBUG_BUF(ctx->rdbuffer + sizeof(MTP_PACKET_HEADER),size - sizeof(MTP_PACKET_HEADER));

		process_in_packet(ctx,mtp_packet_hdr,size);

		return 0;
	}
	else
	{
		PRINT_ERROR("incoming_packet : Read Error (%d)!",size);
	}

	return -1;
}

void mtp_set_usb_handle(mtp_ctx * ctx, void * handle, uint32_t max_packet_size)
{
	ctx->usb_ctx = handle;
	ctx->max_packet_size = max_packet_size;
}

uint32_t mtp_add_storage(mtp_ctx * ctx, char * path, char * description)
{
	int i;

	PRINT_DEBUG("mtp_add_storage : %s", path );

	i = 0;
	while(i < MAX_STORAGE_NB)
	{
		if( !ctx->storages[i].root_path )
		{
			ctx->storages[i].root_path = malloc(strlen(path) + 1);
			ctx->storages[i].description = malloc(strlen(description) + 1);

			if(ctx->storages[i].root_path && ctx->storages[i].description)
			{
				strcpy(ctx->storages[i].root_path,path);
				strcpy(ctx->storages[i].description,description);

				ctx->storages[i].storage_id = i + 1;
				PRINT_DEBUG("mtp_add_storage : Storage %.8X mapped to %s (%s)",
					    ctx->storages[i].storage_id,
					    ctx->storages[i].root_path,
					    ctx->storages[i].description );
				return ctx->storages[i].storage_id;
			}
			else
			{
				if(ctx->storages[i].root_path)
					free(ctx->storages[i].root_path);

				if(ctx->storages[i].description)
					free(ctx->storages[i].description);

				ctx->storages[i].root_path =  NULL;
				ctx->storages[i].description =  NULL;
				ctx->storages[i].storage_id = 0x00000000;

				return ctx->storages[i].storage_id;
			}
		}
		i++;
	}

	return 0x00000000;
}

char * mtp_get_storage_root(mtp_ctx * ctx, uint32_t storage_id)
{
	int i;

	PRINT_DEBUG("mtp_get_storage_root : %.8X", storage_id );

	i = 0;
	while(i < MAX_STORAGE_NB)
	{
		if( ctx->storages[i].root_path )
		{
			if( ctx->storages[i].storage_id == storage_id )
			{
				PRINT_DEBUG("mtp_get_storage_root : %.8X -> %s",
					    storage_id,
					    ctx->storages[i].root_path );
				return ctx->storages[i].root_path;
			}
		}
		i++;
	}

	return NULL;
}

char * mtp_get_storage_description(mtp_ctx * ctx, uint32_t storage_id)
{
	int i;

	PRINT_DEBUG("mtp_get_storage_description : %.8X", storage_id );

	i = 0;
	while(i < MAX_STORAGE_NB)
	{
		if( ctx->storages[i].root_path )
		{
			if( ctx->storages[i].storage_id == storage_id )
			{
				PRINT_DEBUG("mtp_get_storage_description : %.8X -> %s",
					    storage_id,
					    ctx->storages[i].description );
				return ctx->storages[i].description;
			}
		}
		i++;
	}

	return NULL;
}

int mtp_push_event(mtp_ctx * ctx, uint32_t event, int nbparams, uint32_t * parameters )
{
	unsigned char event_buffer[64];
	int size;
	int ret;

	size = build_event_dataset( ctx, event_buffer, sizeof(event_buffer), event , ctx->session_id, 0x00000000, nbparams, parameters);

	PRINT_DEBUG("mtp_push_event : Event packet buffer - %d Bytes :",size);
	PRINT_DEBUG_BUF(event_buffer, size);

	ret = write_usb(ctx->usb_ctx,EP_DESCRIPTOR_INT_IN,event_buffer,size);

	PRINT_DEBUG("write_usb return: %d", ret );

	return ret;
}
