/*
 * uMTP Responder
 * Copyright (c) 2018 - 2026 Viveris Technologies
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
 * @file   mtp_sanitize.h
 * @brief  Detect bad / Fix MTP messages file / folder name.
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

/**
 * sanitize_name()
 *
 * @param src   Input string (UTF-8, NUL-terminated).
 *
 * @return      1 on success, negative error code otherwise.
 */
 
int sanitize_name(char *str, int max_len);

int check_realpath(char *rootpath, char *pathtocheck);
