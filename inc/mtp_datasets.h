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
 * @file   mtp_datasets.h
 * @brief  MTP datasets builders
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#ifndef _INC_MTP_DATASETS_H_
#define _INC_MTP_DATASETS_H_

#include "mtp.h"

int build_deviceinfo_dataset(mtp_ctx * ctx,void * buffer, int maxsize);
int build_storageinfo_dataset(mtp_ctx * ctx,void * buffer, int maxsize,uint32_t storageid);
int build_objectinfo_dataset(mtp_ctx * ctx, void * buffer, int maxsize,fs_entry * entry);
int build_event_dataset(mtp_ctx * ctx, void * buffer, int maxsize, uint32_t event, uint32_t session, uint32_t transaction, int nbparams, uint32_t * parameters);

void set_default_date(struct tm * date);

#endif
