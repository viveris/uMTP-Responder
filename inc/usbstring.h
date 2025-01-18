/*
 * uMTP Responder
 * Copyright (c) 2018 - 2025 Viveris Technologies
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
 * @file   usbstring.h
 * @brief  USB strings - Function & structures declarations.
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#ifndef _INC_USBSTRING_H_
#define _INC_USBSTRING_H_

struct usb_string {
	uint8_t id;
	char *str;
};

struct usb_gadget_strings {
	uint16_t language;            /* 0x0409 for en-us */
	struct usb_string *strings;
};

int usb_gadget_get_string (struct usb_gadget_strings *table, int id, uint8_t *buf);
int unicode2charstring(char * str, uint16_t * unicodestr, int maxstrsize);
int char2unicodestring(char * unicodestr, int index, int maxsize, char * str, int unicodestrsize);
#endif
