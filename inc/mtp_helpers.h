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
 * @file   mtp_helpers.h
 * @brief  MTP messages creation helpers.
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#ifndef _INC_MTP_HELPERS_H_
#define _INC_MTP_HELPERS_H_
int poke32(void * buffer, int index, int maxsize, uint32_t data);
int poke16(void * buffer, int index, int maxsize, uint16_t data);
int poke08(void * buffer, int index, int maxsize, uint8_t  data);

uint32_t peek(void * buffer, int index, int typesize);
uint64_t peek64(void * buffer, int index, int typesize);
int poke_string(void * buffer, int index, int maxsize, const char *str);
int poke_array(void * buffer, int index, int maxsize, int size, int elementsize, const unsigned char *bufferin,int prefixed);
uint16_t posix_to_mtp_errcode(int err);

int set_giduid(mtp_ctx * ctx,int uid,int gid);

int set_storage_giduid(mtp_ctx * ctx,uint32_t storage_id);
int restore_giduid(mtp_ctx * ctx);

#endif
