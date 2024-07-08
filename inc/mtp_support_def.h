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
 * @file   mtp_support_def.h
 * @brief  MTP support definitions.
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#ifndef _INC_MTS_SUPPORT_DEF_H_
#define _INC_MTS_SUPPORT_DEF_H_

#define MTP_VERSION 100

extern const unsigned short supported_op[];
extern const int supported_op_size;

extern const unsigned short supported_event[];
extern const int supported_event_size;

#endif
