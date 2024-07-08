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
 * @file   mtp_helpers.c
 * @brief  MTP messages creation helpers.
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include "buildconf.h"

#include <inttypes.h>
#include <inttypes.h>
#include <pthread.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/fsuid.h>
#include <unistd.h>
#include <errno.h>

#include "mtp.h"
#include "mtp_helpers.h"
#include "usbstring.h"

#include "mtp_constant.h"

#include "logs_out.h"

int poke32(void * buffer, int index, int maxsize,uint32_t data)
{
	unsigned char *ptr;

	if(index < 0)
		return index;

	if(index + 4 >= maxsize)
	{
#ifdef DEBUG
		PRINT_DEBUG("poke32 : buffer overrun operation attempt ! index : %d, maxise : %d",index,maxsize);
#endif
		return -1;
	}

	ptr = ((unsigned char *)buffer);

	ptr += index;

	*ptr++ = data & 0xFF;
	*ptr++ = (data>>8) & 0xFF;
	*ptr++ = (data>>16) & 0xFF;
	*ptr   = (data>>24) & 0xFF;

	return index + 4;
}

int poke16(void * buffer, int index, int maxsize, uint16_t data)
{
	unsigned char *ptr;

	if(index < 0)
		return index;

	if(index + 2 >= maxsize)
	{
#ifdef DEBUG
		PRINT_DEBUG("poke16 : buffer overrun operation attempt ! index : %d, maxise : %d",index,maxsize);
#endif
		return -1;
	}

	ptr = ((unsigned char *)buffer);

	ptr += index;

	*ptr++ = data & 0xFF;
	*ptr   = (data>>8) & 0xFF;

	return index + 2;
}

int poke08(void * buffer, int index, int maxsize, uint8_t data)
{
	if(index < 0)
		return index;

	if(index + 1 >= maxsize)
	{
#ifdef DEBUG
		PRINT_DEBUG("poke08 : buffer overrun operation attempt ! index : %d, maxise : %d",index,maxsize);
#endif
		return -1;
	}

	*(((unsigned char *)buffer) + index) = ((uint8_t)data);
	return index + 1;
}

uint32_t peek(void * buffer, int index, int typesize)
{
	unsigned char *ptr;
	uint32_t data;
	unsigned int shift;

	ptr = ((unsigned char *)buffer);

	shift = 0;
	data = 0x00000000;
	do
	{
		data |= (((uint32_t)ptr[index]) << shift);
		index++;
		typesize--;
		shift += 8;
	}while( typesize );

	return data;
}

uint64_t peek64(void * buffer, int index, int typesize)
{
	unsigned char *ptr;
	uint64_t data;
	unsigned int shift;

	ptr = ((unsigned char *)buffer);

	shift = 0;
	data = 0x0000000000000000;
	do
	{
		data |= (((uint64_t)ptr[index]) << shift);
		index++;
		typesize--;
		shift += 8;
	}while( typesize );

	return data;
}

int poke_string(void * buffer, int index, int maxsize, const char *str)
{
	unsigned char *ptr;
	int sizeposition;
	int len;

	if(index < 0)
		return index;

	ptr = ((unsigned char *)buffer);

	if( index + 1 >= maxsize )
	{
#ifdef DEBUG
		PRINT_DEBUG("poke_string : buffer overrun operation attempt ! index : %d, maxise : %d",index,maxsize);
#endif
		return -1;
	}

	// Reserve string size .
	sizeposition = index;
	ptr[index] = 0x00;

	index++;

	if( str[0] == '\0' )
		return index;  // Empty string case.

	// Char to unicode...
	len = char2unicodestring((char*)ptr, index, maxsize, (char*)str, 255);

	if(len < 0)
	{
#ifdef DEBUG
		PRINT_DEBUG("poke_string : char2unicodestring error %d !",len);
#endif
		return -1;
	}

	index += (len*2);

	// Update size position
	ptr[sizeposition] = len;

	return index;
}

int poke_array(void * buffer, int index, int maxsize, int size, int elementsize, const unsigned char *bufferin,int prefixed)
{
	unsigned char *ptr;
	int i,nbelement;

	if(index < 0)
		return index;

	if( index + (size + (prefixed*4)) >= maxsize )
	{
#ifdef DEBUG
		PRINT_DEBUG("poke_array : buffer overrun operation attempt ! prefixed : %d, index : %d, maxise : %d",prefixed,index,maxsize);
#endif
		return -1;
	}

	ptr = ((unsigned char *)buffer);

	nbelement = size / elementsize;

	if(prefixed)
	{
		ptr[index++] = nbelement&0xFF;
		ptr[index++] = (nbelement>>8)&0xFF;
		ptr[index++] = (nbelement>>16)&0xFF;
		ptr[index++] = (nbelement>>24)&0xFF;
	}

	i = 0;
	while( i < size )
	{
		ptr[index++] = bufferin[i];
		i++;
	}

	return index;
}

uint16_t posix_to_mtp_errcode(int err)
{
	uint16_t code;

	code = MTP_RESPONSE_GENERAL_ERROR;

	switch(err)
	{
		case EBUSY:
			return MTP_RESPONSE_DEVICE_BUSY;
		break;
		case ETXTBSY:
			return MTP_RESPONSE_DEVICE_BUSY;
		break;
		case EACCES:
			return MTP_RESPONSE_ACCESS_DENIED;
		break;
		case EPERM:
			return MTP_RESPONSE_ACCESS_DENIED;
		break;
		case EINPROGRESS:
			return MTP_RESPONSE_DEVICE_BUSY;
		break;
		case EAGAIN:
			return MTP_RESPONSE_DEVICE_BUSY;
		break;
		case EBADF:
			return MTP_RESPONSE_INVALID_OBJECT_HANDLE;
		break;
		case EBADFD:
			return MTP_RESPONSE_INVALID_OBJECT_HANDLE;
		break;
		case ENOENT:
			return MTP_RESPONSE_INVALID_OBJECT_HANDLE;
		break;
		case ECANCELED:
			return MTP_RESPONSE_INCOMPLETE_TRANSFER;
		break;
		case EDQUOT:
			return MTP_RESPONSE_STORAGE_FULL;
		break;
		case EEXIST:
		break;
		case EFBIG:
			return MTP_RESPONSE_STORAGE_FULL;
		break;
		case EHWPOISON:
			return MTP_RESPONSE_GENERAL_ERROR;
		break;
		case EINTR:
			return MTP_RESPONSE_GENERAL_ERROR;
		break;
		case EINVAL:
			return MTP_RESPONSE_INVALID_PARAMETER;
		break;
		case EREMOTEIO:
		case EIO:
			return MTP_RESPONSE_GENERAL_ERROR;
		break;
		case EISDIR:
			return MTP_RESPONSE_INVALID_PARAMETER;
		break;
		case ELIBACC:
		case ELIBBAD:
		case ELIBSCN:
		case ELIBMAX:
		case ELIBEXEC:
		case ENOEXEC:
		case ENOPKG:
		case ENOSYS:
		case ENOTRECOVERABLE:
		case ENOTSUP:
		case EPIPE:
			return MTP_RESPONSE_GENERAL_ERROR;
		break;
		case ELOOP:
			return MTP_RESPONSE_INCOMPLETE_TRANSFER;
		break;
		case EMEDIUMTYPE:
			return MTP_RESPONSE_INVALID_OBJECT_HANDLE;
		break;
		case EMFILE:
			return MTP_RESPONSE_STORAGE_FULL;
		break;
		case EMLINK:
			return MTP_RESPONSE_STORAGE_FULL;
		break;
		case ENOSPC:
			return MTP_RESPONSE_STORAGE_FULL;
		break;
		case ENOMEM:
			return MTP_RESPONSE_GENERAL_ERROR;
		break;
		case ENAMETOOLONG:
			return MTP_RESPONSE_INVALID_PARAMETER;
		break;
		case ENFILE:
			return MTP_RESPONSE_GENERAL_ERROR;
		break;
		case ENODEV:
			return MTP_RESPONSE_INVALID_PARAMETER;
		break;
		case ENOLINK:
			return MTP_RESPONSE_INVALID_OBJECT_HANDLE;
		break;
		case ENOMEDIUM:
			return MTP_RESPONSE_INVALID_OBJECT_HANDLE;
		break;
		case ENOTBLK:
			return MTP_RESPONSE_INVALID_OBJECT_HANDLE;
		break;
		case ENOTDIR:
			return MTP_RESPONSE_ACCESS_DENIED;
		break;
		case ENOTEMPTY:
			return MTP_RESPONSE_PARTIAL_DELETION;
		break;
		case EROFS:
			return MTP_RESPONSE_STORE_READ_ONLY;
		break;
		case ESPIPE:
			return MTP_RESPONSE_GENERAL_ERROR;
		break;
		case ESTALE:
			return MTP_RESPONSE_INVALID_OBJECT_HANDLE;
		break;
	}

	return code;
}

int set_giduid(mtp_ctx * ctx,int uid,int gid)
{
	int ret;

	ret = 0;

	if(uid >= 0)
	{
		if(setfsuid(uid))
			ret = -1;
	}

	if(gid >= 0)
	{
		if(setfsgid(gid))
			ret = -1;
	}

	return ret;
}

int restore_giduid(mtp_ctx * ctx)
{
	return set_giduid( ctx, ctx->uid, ctx->gid );
}

int set_storage_giduid(mtp_ctx * ctx,uint32_t storage_id)
{
	int i,uid,gid;

	i = mtp_get_storage_index_by_id(ctx, storage_id);

	if( i >= 0 && i < MAX_STORAGE_NB )
	{
		uid = ctx->default_uid;
		gid = ctx->default_gid;

		if(ctx->storages[i].uid != -1)
			uid = ctx->storages[i].uid;

		if(ctx->storages[i].gid != -1)
			gid = ctx->storages[i].uid;

		return set_giduid(ctx,uid,gid);
	}

	return -1;
}
