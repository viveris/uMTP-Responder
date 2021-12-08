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
 * @file   mtp.c
 * @brief  Main MTP protocol functions.
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include "buildconf.h"

#include <inttypes.h>
#include <pthread.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include <sys/stat.h>
#include <unistd.h>

#include "mtp.h"
#include "mtp_helpers.h"
#include "mtp_constant.h"
#include "mtp_constant_strings.h"
#include "mtp_datasets.h"
#include "mtp_properties.h"
#include "mtp_operations.h"

#include "usb_gadget_fct.h"
#include "mtp_support_def.h"
#include "fs_handles_db.h"

#include "inotify.h"
#include "msgqueue.h"

#include "logs_out.h"

mtp_ctx * mtp_init_responder()
{
	mtp_ctx * ctx;

	PRINT_DEBUG("init_mtp_responder");

	ctx = malloc(sizeof(mtp_ctx));
	if(ctx)
	{
		memset(ctx,0,sizeof(mtp_ctx));

		ctx->usb_wr_buffer_max_size = CONFIG_MAX_TX_USB_BUFFER_SIZE;
		ctx->wrbuffer = NULL;

		ctx->usb_rd_buffer_max_size = CONFIG_MAX_RX_USB_BUFFER_SIZE;
		ctx->rdbuffer = NULL;
		ctx->rdbuffer2 = NULL;

		ctx->read_file_buffer_size = CONFIG_READ_FILE_BUFFER_SIZE;
		ctx->read_file_buffer = NULL;

		ctx->temp_array = malloc( MAX_STORAGE_NB * sizeof(uint32_t) );
		if(!ctx->temp_array)
			goto init_error;

		ctx->uid = getuid();
		ctx->euid = geteuid();
		ctx->gid = getgid();
		ctx->egid = getegid();

		ctx->default_uid = -1;
		ctx->default_gid = -1;

		ctx->SetObjectPropValue_Handle = 0xFFFFFFFF;

		pthread_mutex_init ( &ctx->inotify_mutex, NULL);

		inotify_handler_init( ctx );
		msgqueue_handler_init( ctx );

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
		msgqueue_handler_deinit( ctx );
		inotify_handler_deinit( ctx );

		if(ctx->wrbuffer)
			free(ctx->wrbuffer);

		if(ctx->rdbuffer)
			free(ctx->rdbuffer);

		if(ctx->rdbuffer2)
			free(ctx->rdbuffer2);

		if(ctx->temp_array)
			free(ctx->temp_array);

		if(ctx->read_file_buffer)
			free(ctx->read_file_buffer);

		free(ctx);
	}
}

int build_response(mtp_ctx * ctx, uint32_t tx_id, uint16_t type, uint16_t status, void * buffer, int maxsize, void * datain,int size)
{
	MTP_PACKET_HEADER tmp_hdr;
	int ofs;

	ofs = 0;

	tmp_hdr.length = sizeof(tmp_hdr) + size;
	tmp_hdr.operation = type;
	tmp_hdr.code = status;
	tmp_hdr.tx_id = tx_id;

	ofs = poke_array(buffer, ofs, maxsize, sizeof(MTP_PACKET_HEADER), 1, (unsigned char*)&tmp_hdr,0);

	if(size)
		ofs = poke_array(buffer, ofs, maxsize, size, 1, (unsigned char*)datain,0);

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
	uint16_t unicode_str[256+1];
	char * parent_folder;
	char * tmp_path;
	uint32_t storage_flags;
	int i,ret_code,ret;
	fs_entry * entry;
	int file;
	filefoundinfo tmp_file_entry;

	ret_code = MTP_RESPONSE_GENERAL_ERROR;

	tmp_hdr = (MTP_PACKET_HEADER *)datain;

	if(parent_handle == 0xFFFFFFFF)
		parent_handle = 0x00000000;

	PRINT_DEBUG("Incoming dataset : %d bytes (raw) %d bytes, operation 0x%x, code 0x%x, tx_id: %x",size,tmp_hdr->length,tmp_hdr->operation,tmp_hdr->code ,tmp_hdr->tx_id );

	storage_flags = mtp_get_storage_flags(ctx, storage_id);
	if( storage_flags == 0xFFFFFFFF )
	{
		PRINT_DEBUG("parse_incomming_dataset : Storage 0x%.8x is Invalid !",storage_id);
		return MTP_RESPONSE_INVALID_STORAGE_ID;
	}

	if( (storage_flags & UMTP_STORAGE_READONLY) )
	{
		PRINT_DEBUG("parse_incomming_dataset : Storage 0x%.8x is Read only !", storage_id);
		return MTP_RESPONSE_STORE_READ_ONLY;
	}

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
				peek(dataset_ptr,0x2A, 2);                 // Association Type
#endif

				string_len = peek(dataset_ptr,0x34, 1);    // File name

				if(string_len > 255)
					string_len = 255;

				memset(unicode_str,0,sizeof(unicode_str));
				for(i=0;i<string_len;i++)
				{
					unicode_str[i] = peek(dataset_ptr,0x35 + (i*2), 2);
				}
				unicode2charstring(tmp_str, unicode_str, sizeof(tmp_str));

				PRINT_DEBUG("MTP_OPERATION_SEND_OBJECT_INFO : 0x%x objectformat Size %d, Parent 0x%.8x, type: %x, strlen %d str:%s",objectformat,objectsize,parent_handle,type,string_len,tmp_str);

				entry = get_entry_by_handle_and_storageid(ctx->fs_db, parent_handle,storage_id);
				if(entry)
				{
					if(entry->flags & ENTRY_IS_DIR)
					{
						tmp_path = NULL;

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

							ret = -1;

							if(!set_storage_giduid(ctx, entry->storage_id))
							{
								ret = mkdir(tmp_path, 0700);
							}

							restore_giduid(ctx);

							if( ret )
							{
								PRINT_WARN("MTP_OPERATION_SEND_OBJECT_INFO : Can't create %s ...",tmp_path);

								if(parent_folder)
									free(parent_folder);

								free(tmp_path);

								ret_code = MTP_RESPONSE_ACCESS_DENIED;

								return ret_code;
							}

							if(ctx->usb_cfg.val_umask >= 0)
								chmod(tmp_path, 0777 & (~ctx->usb_cfg.val_umask));

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

				memset(unicode_str,0,sizeof(unicode_str));
				for(i=0;i<string_len;i++)
				{
					unicode_str[i] = peek(dataset_ptr,0x35 + (i*2), 2);
				}
				unicode2charstring(tmp_str, unicode_str, sizeof(tmp_str));

				PRINT_DEBUG("MTP_OPERATION_SEND_OBJECT_INFO : 0x%x objectformat Size %d, Parent 0x%.8x, type: %x, strlen %d str:%s",objectformat,objectsize,parent_handle,type,string_len,tmp_str);

				entry = get_entry_by_handle_and_storageid(ctx->fs_db, parent_handle,storage_id);
				if(entry)
				{
					if(entry->flags & ENTRY_IS_DIR)
					{
						parent_folder = build_full_path(ctx->fs_db, mtp_get_storage_root(ctx, entry->storage_id), entry);

						tmp_path = NULL;
						entry = NULL;

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

							file = -1;

							if(!set_storage_giduid(ctx, storage_id))
							{
								file = open(tmp_path,O_WRONLY | O_CREAT | O_TRUNC | O_LARGEFILE, S_IRUSR|S_IWUSR);
							}

							restore_giduid(ctx);

							if( file == -1)
							{
								PRINT_WARN("MTP_OPERATION_SEND_OBJECT_INFO : Can't create %s ...",tmp_path);

								if(parent_folder)
									free(parent_folder);

								free(tmp_path);

								ret_code = MTP_RESPONSE_ACCESS_DENIED;

								return ret_code;
							}
							close( file );

							entry = add_entry(ctx->fs_db, &tmp_file_entry, parent_handle, storage_id);

							free(tmp_path);
						}

						if(entry)
						{
							ctx->SendObjInfoHandle = entry->handle;
							ctx->SendObjInfoSize = objectsize;
							ctx->SendObjInfoOffset = 0;
							*newhandle = entry->handle;
						}
						else
						{
							ctx->SendObjInfoHandle = 0xFFFFFFFF;
							ctx->SendObjInfoSize = 0;
							ctx->SendObjInfoOffset = 0;
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

int check_handle_access( mtp_ctx * ctx, fs_entry * entry, uint32_t handle, int wraccess, uint32_t * response)
{
	uint32_t storage_flags;

	if( !entry )
	{
		entry = get_entry_by_handle(ctx->fs_db, handle);
	}

	if(entry)
	{
		storage_flags = mtp_get_storage_flags(ctx, entry->storage_id);
		if( storage_flags == 0xFFFFFFFF )
		{
			PRINT_DEBUG("check_handle_access : Storage 0x%.8x is Invalid !",entry->storage_id);

			if( response )
				*response = MTP_RESPONSE_INVALID_STORAGE_ID;

			return 1;
		}

		if( (storage_flags & UMTP_STORAGE_LOCKED) )
		{
			PRINT_DEBUG("check_handle_access : Storage 0x%.8x is locked !",entry->storage_id);

			if( response )
				*response = MTP_RESPONSE_STORE_NOT_AVAILABLE;

			return 1;
		}

		if( (storage_flags & UMTP_STORAGE_READONLY) && wraccess )
		{
			PRINT_DEBUG("check_handle_access : Storage 0x%.8x is Read only !",entry->storage_id);

			if( response )
				*response = MTP_RESPONSE_STORE_READ_ONLY;

			return 1;
		}
	}
	else
	{
		PRINT_DEBUG("check_handle_access : Handle 0x%.8x is invalid !",handle);

		if( response )
			*response = MTP_RESPONSE_INVALID_OBJECT_HANDLE;

		return 1;
	}

	return 0;
}

int process_in_packet(mtp_ctx * ctx,MTP_PACKET_HEADER * mtp_packet_hdr, int rawsize)
{
	uint32_t params[5];
	int params_size;
	uint32_t response_code;
	int size;

	params[0] = 0x000000;
	params[1] = 0x000000;
	params[2] = 0x000000;
	params[3] = 0x000000;
	params[4] = 0x000000;

	params_size = 0; // No response parameter by default

	size = rawsize;

	switch( mtp_packet_hdr->code )
	{
		case MTP_OPERATION_OPEN_SESSION:
			response_code = mtp_op_OpenSession(ctx,mtp_packet_hdr,&size,(uint32_t*)&params,&params_size);
		break;

		case MTP_OPERATION_CLOSE_SESSION:
			response_code = mtp_op_CloseSession(ctx,mtp_packet_hdr,&size,(uint32_t*)&params,&params_size);
		break;

		case MTP_OPERATION_GET_DEVICE_INFO:
			response_code = mtp_op_GetDeviceInfos(ctx,mtp_packet_hdr,&size,(uint32_t*)&params,&params_size);
		break;

		case MTP_OPERATION_GET_STORAGE_IDS:
			response_code = mtp_op_GetStorageIDs(ctx,mtp_packet_hdr,&size,(uint32_t*)&params,&params_size);
		break;

		case MTP_OPERATION_GET_STORAGE_INFO:
			response_code = mtp_op_GetStorageInfo(ctx,mtp_packet_hdr,&size,(uint32_t*)&params,&params_size);
		break;

		case MTP_OPERATION_GET_DEVICE_PROP_DESC:
			response_code = mtp_op_GetDevicePropDesc(ctx,mtp_packet_hdr,&size,(uint32_t*)&params,&params_size);
		break;

		case MTP_OPERATION_GET_DEVICE_PROP_VALUE:
			response_code = mtp_op_GetDevicePropValue(ctx,mtp_packet_hdr,&size,(uint32_t*)&params,&params_size);
		break;

		case MTP_OPERATION_GET_OBJECT_HANDLES:
			response_code = mtp_op_GetObjectHandles(ctx,mtp_packet_hdr,&size,(uint32_t*)&params,&params_size);
		break;

		case MTP_OPERATION_GET_OBJECT_INFO:
			response_code = mtp_op_GetObjectInfo(ctx,mtp_packet_hdr,&size,(uint32_t*)&params,&params_size);
		break;

		case MTP_OPERATION_GET_PARTIAL_OBJECT_64:
		case MTP_OPERATION_GET_PARTIAL_OBJECT:
			response_code = mtp_op_GetPartialObject(ctx,mtp_packet_hdr,&size,(uint32_t*)&params,&params_size);
		break;

		case MTP_OPERATION_GET_OBJECT:
			response_code = mtp_op_GetObject(ctx,mtp_packet_hdr,&size,(uint32_t*)&params,&params_size);
		break;

		case MTP_OPERATION_SEND_OBJECT_INFO:
			response_code = mtp_op_SendObjectInfo(ctx,mtp_packet_hdr,&size,(uint32_t*)&params,&params_size);
		break;

		case MTP_OPERATION_SEND_PARTIAL_OBJECT:
		case MTP_OPERATION_SEND_OBJECT:
			response_code = mtp_op_SendObject(ctx,mtp_packet_hdr,&size,(uint32_t*)&params,&params_size);
		break;

		case MTP_OPERATION_DELETE_OBJECT:
			response_code = mtp_op_DeleteObject(ctx,mtp_packet_hdr,&size,(uint32_t*)&params,&params_size);
		break;

		case MTP_OPERATION_GET_OBJECT_PROPS_SUPPORTED:
			response_code = mtp_op_GetObjectPropsSupported(ctx,mtp_packet_hdr,&size,(uint32_t*)&params,&params_size);
		break;

		case MTP_OPERATION_GET_OBJECT_PROP_DESC:
			response_code = mtp_op_GetObjectPropDesc(ctx,mtp_packet_hdr,&size,(uint32_t*)&params,&params_size);
		break;

		case MTP_OPERATION_GET_OBJECT_PROP_VALUE:
			response_code = mtp_op_GetObjectPropValue(ctx,mtp_packet_hdr,&size,(uint32_t*)&params,&params_size);
		break;

		case MTP_OPERATION_SET_OBJECT_PROP_VALUE:
			response_code = mtp_op_SetObjectPropValue(ctx,mtp_packet_hdr,&size,(uint32_t*)&params,&params_size);
		break;

		case MTP_OPERATION_GET_OBJECT_PROP_LIST:
			response_code = mtp_op_GetObjectPropList(ctx,mtp_packet_hdr,&size,(uint32_t*)&params,&params_size);
		break;

		case MTP_OPERATION_GET_OBJECT_REFERENCES:
			response_code = mtp_op_GetObjectReferences(ctx,mtp_packet_hdr,&size,(uint32_t*)&params,&params_size);
		break;

		case MTP_OPERATION_BEGIN_EDIT_OBJECT:
			response_code = mtp_op_BeginEditObject(ctx,mtp_packet_hdr,&size,(uint32_t*)&params,&params_size);
		break;

		case MTP_OPERATION_END_EDIT_OBJECT:
			response_code = mtp_op_EndEditObject(ctx,mtp_packet_hdr,&size,(uint32_t*)&params,&params_size);
		break;

		case MTP_OPERATION_TRUNCATE_OBJECT :
			response_code = mtp_op_TruncateObject(ctx,mtp_packet_hdr,&size,(uint32_t*)&params,&params_size);
		break;

		default:

			PRINT_WARN("MTP code unsupported ! : 0x%.4X (%s)", mtp_packet_hdr->code,mtp_get_operation_string(mtp_packet_hdr->code));

			response_code = MTP_RESPONSE_OPERATION_NOT_SUPPORTED;

		break;
	}

	// Send the status response
	if(response_code != MTP_RESPONSE_NO_RESPONSE)
	{
		size = build_response(ctx, mtp_packet_hdr->tx_id,MTP_CONTAINER_TYPE_RESPONSE, response_code, ctx->wrbuffer, ctx->usb_wr_buffer_max_size , &params,params_size);

		if(size >= 0)
		{
			PRINT_DEBUG("Status response (%d Bytes):",size);
			PRINT_DEBUG_BUF(ctx->wrbuffer, size);

			write_usb(ctx->usb_ctx,EP_DESCRIPTOR_IN,ctx->wrbuffer,size);
		}
	}
	else
	{
		PRINT_DEBUG("No Status response sent");
	}

	return 0; // TODO Return usb error code.
}

int mtp_incoming_packet(mtp_ctx * ctx)
{
	int size;
	MTP_PACKET_HEADER * mtp_packet_hdr;

	if(!ctx)
		return 0;

	size = read_usb(ctx->usb_ctx, ctx->rdbuffer, ctx->usb_rd_buffer_max_size);

	if(size>=0)
	{
		PRINT_DEBUG("--------------------------------------------------");
		PRINT_DEBUG("Incoming_packet : %p - rawsize : %d",ctx->rdbuffer,size);

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

uint32_t mtp_add_storage(mtp_ctx * ctx, char * path, char * description, int uid, int gid, uint32_t flags)
{
	int i;

	PRINT_DEBUG("mtp_add_storage : %s", path );

	if (mtp_get_storage_id_by_name(ctx, description) != 0xFFFFFFFF)
		return 0x00000000;

	i = 0;
	while(i < MAX_STORAGE_NB)
	{
		if( !ctx->storages[i].root_path )
		{
			ctx->storages[i].root_path = malloc(strlen(path) + 1);
			ctx->storages[i].description = malloc(strlen(description) + 1);

			if(ctx->storages[i].root_path && ctx->storages[i].description)
			{
				ctx->storages[i].uid = uid;
				ctx->storages[i].gid = gid;

				strcpy(ctx->storages[i].root_path,path);
				strcpy(ctx->storages[i].description,description);
				ctx->storages[i].flags = flags;

				ctx->storages[i].storage_id = 0xFFFF0000 + (i + 1);
				PRINT_DEBUG("mtp_add_storage : Storage %.8X mapped to %s (%s) (Flags: 0x%.8X)",
					    ctx->storages[i].storage_id,
					    ctx->storages[i].root_path,
					    ctx->storages[i].description,
					    ctx->storages[i].flags);

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
				ctx->storages[i].flags =  0x00000000;
				ctx->storages[i].storage_id = 0x00000000;
				ctx->storages[i].uid = -1;
				ctx->storages[i].gid = -1;

				return ctx->storages[i].storage_id;
			}
		}
		i++;
	}

	return 0x00000000;
}

int mtp_remove_storage(mtp_ctx * ctx, char * name)
{
	int index = mtp_get_storage_index_by_name(ctx, name);

	if (index < 0)
		return index;

	free(ctx->storages[index].root_path);
	free(ctx->storages[index].description);

	ctx->storages[index].root_path = NULL;
	ctx->storages[index].description = NULL;
	ctx->storages[index].flags = 0x00000000;
	ctx->storages[index].storage_id = 0x00000000;
	ctx->storages[index].uid = -1;
	ctx->storages[index].gid = -1;

	return 0;
}

uint32_t mtp_get_storage_id_by_name(mtp_ctx * ctx, char * name)
{
	int i;

	PRINT_DEBUG("mtp_get_storage_id_by_name : %s", name );

	i = 0;
	while(i < MAX_STORAGE_NB)
	{
		if( ctx->storages[i].root_path )
		{
			if( !strcmp(ctx->storages[i].description, name ) )
			{
				PRINT_DEBUG("mtp_get_storage_id_by_name : %s -> %.8X",
					    ctx->storages[i].root_path,
						ctx->storages[i].storage_id);

				return ctx->storages[i].storage_id;
			}
		}
		i++;
	}

	return 0xFFFFFFFF;
}

int mtp_get_storage_index_by_name(mtp_ctx * ctx, char * name)
{
	int i;

	PRINT_DEBUG("mtp_get_storage_index_by_name : %s", name );

	i = 0;
	while(i < MAX_STORAGE_NB)
	{
		if( ctx->storages[i].root_path )
		{
			if( !strcmp(ctx->storages[i].description, name ) )
			{
				PRINT_DEBUG("mtp_get_storage_index_by_name : %s -> %.8X",
					    ctx->storages[i].root_path,
						i);

				return i;
			}
		}
		i++;
	}

	return -1;
}

int mtp_get_storage_index_by_id(mtp_ctx * ctx, uint32_t storage_id)
{
	int i;

	PRINT_DEBUG("mtp_get_storage_index_by_id : 0x%X", storage_id );

	i = 0;
	while(i < MAX_STORAGE_NB)
	{
		if( ctx->storages[i].root_path )
		{
			if( ctx->storages[i].storage_id == storage_id )
			{
				PRINT_DEBUG("mtp_get_storage_index_by_id : %.8X -> %d",
					    storage_id,
					    i );
				return i;
			}
		}
		i++;
	}

	return -1;
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

uint32_t mtp_get_storage_flags(mtp_ctx * ctx, uint32_t storage_id)
{
	int i;

	PRINT_DEBUG("mtp_get_storage_flags : %.8X", storage_id );

	i = 0;
	while(i < MAX_STORAGE_NB)
	{
		if( ctx->storages[i].root_path )
		{
			if( ctx->storages[i].storage_id == storage_id )
			{
				PRINT_DEBUG("mtp_get_storage_flags : %.8X -> 0x%.8X",
					    storage_id,
					    ctx->storages[i].flags );
				return ctx->storages[i].flags;
			}
		}
		i++;
	}

	return 0xFFFFFFFF;
}

int mtp_push_event(mtp_ctx * ctx, uint32_t event, int nbparams, uint32_t * parameters )
{
	unsigned char event_buffer[64];
	int size;
	int ret;

	size = build_event_dataset( ctx, event_buffer, sizeof(event_buffer), event , ctx->session_id, 0x00000000, nbparams, parameters);
	if(size < 0)
		return -1;

	PRINT_DEBUG("mtp_push_event : Event packet buffer - %d Bytes :",size);
	PRINT_DEBUG_BUF(event_buffer, size);

	ret = write_usb(ctx->usb_ctx,EP_DESCRIPTOR_INT_IN,event_buffer,size);

	PRINT_DEBUG("write_usb return: %d", ret );

	return ret;
}
