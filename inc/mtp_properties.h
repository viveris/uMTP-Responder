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
 * @file   mtp_properties.h
 * @brief  MTP properties datasets helpers
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#ifndef _INC_MTP_PROPERTIES_H_
#define _INC_MTP_PROPERTIES_H_

#include "mtp.h"

int build_properties_supported_dataset(mtp_ctx * ctx,void * buffer, int maxsize,uint32_t format_id);
int build_properties_dataset(mtp_ctx * ctx,void * buffer, int maxsize,uint32_t property_id,uint32_t format_id);
int build_ObjectPropValue_dataset(mtp_ctx * ctx,void * buffer, int maxsize,uint32_t handle,uint32_t prop_code);
int setObjectPropValue(mtp_ctx * ctx,MTP_PACKET_HEADER * mtp_packet_hdr, uint32_t handle,uint32_t prop_code);

int build_device_properties_dataset(mtp_ctx * ctx,void * buffer, int maxsize,uint32_t property_id);

#endif
