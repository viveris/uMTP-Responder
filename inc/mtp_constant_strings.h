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
 * @file   mtp_constant_strings.h
 * @brief  MTP Codes to string decoder (Debug purpose)
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#ifndef _INC_MTP_CONSTANT_STRINGS_H_
extern const char DevInfos_MTP_Extensions[];

const char * mtp_get_operation_string(uint16_t operation);
const char * mtp_get_property_string(uint16_t property);
const char * mtp_get_format_string(uint16_t format);
const char * mtp_get_type_string(uint16_t type);
#endif
