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
 * @file   usbstring.c
 * @brief  USB Strings
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

/*
 *
 * Note : This source file is based on the Linux kernel usbstring.c
 * with the following Licence/copyright :
 *
 */

/*
 * Copyright (C) 2003 David Brownell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <stdint.h>

#include <linux/types.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadgetfs.h>

#include <errno.h>
#include <string.h>

#include "usbstring.h"

/* From usbstring.[ch] */

static inline void put_unaligned_le16(uint16_t val, uint16_t *cp)
{
	uint8_t *p = (void *)cp;

	*p++ = (uint8_t) val;
	*p++ = (uint8_t) (val >> 8);
}

static int utf8_to_utf16le(const char *s, uint16_t *cp, unsigned len)
{
	int count = 0;
	uint8_t c;
	uint16_t uchar;

	/* this insists on correct encodings, though not minimal ones.
	 * BUT it currently rejects legit 4-byte UTF-8 code points,
	 * which need surrogate pairs.  (Unicode 3.1 can use them.)
	 */
	while (len != 0 && (c = (uint8_t) *s++) != 0) {
		if (c & 0x80) {
			// 2-byte sequence:
			// 00000yyyyyxxxxxx = 110yyyyy 10xxxxxx
			if ((c & 0xe0) == 0xc0) {
				uchar = (c & 0x1f) << 6;

				c = (uint8_t) *s++;
				if ((c & 0xc0) != 0xc0)
					goto fail;
				c &= 0x3f;
				uchar |= c;

			// 3-byte sequence (most CJKV characters):
			// zzzzyyyyyyxxxxxx = 1110zzzz 10yyyyyy 10xxxxxx
			} else if ((c & 0xf0) == 0xe0) {
				uchar = (c & 0x0f) << 12;

				c = (uint8_t) *s++;
				if ((c & 0xc0) != 0xc0)
					goto fail;
				c &= 0x3f;
				uchar |= c << 6;

				c = (uint8_t) *s++;
				if ((c & 0xc0) != 0xc0)
					goto fail;
				c &= 0x3f;
				uchar |= c;

				/* no bogus surrogates */
				if (0xd800 <= uchar && uchar <= 0xdfff)
					goto fail;

			// 4-byte sequence (surrogate pairs, currently rare):
			// 11101110wwwwzzzzyy + 110111yyyyxxxxxx
			//     = 11110uuu 10uuzzzz 10yyyyyy 10xxxxxx
			// (uuuuu = wwww + 1)
			// FIXME accept the surrogate code points (only)

			} else
				goto fail;
		} else
			uchar = c;
		put_unaligned_le16 (uchar, cp++);
		count++;
		len--;
	}
	return count;
fail:
	return -1;
}

int usb_gadget_get_string (struct usb_gadget_strings *table, int id, uint8_t *buf)
{
	struct usb_string *s;
	int len;

	/* descriptor 0 has the language id */
	if (id == 0) {
		buf [0] = 4;
		buf [1] = USB_DT_STRING;
		buf [2] = (uint8_t) table->language;
		buf [3] = (uint8_t) (table->language >> 8);
		return 4;
	}
	for (s = table->strings; s && s->str; s++)
		if (s->id == id)
			break;

	/* unrecognized: stall. */
	if (!s || !s->str)
		return -EINVAL;

	/* string descriptors have length, tag, then UTF16-LE text */
	len = strlen (s->str);
	if (len > 126)
		len = 126;
	memset (buf + 2, 0, 2 * len); /* zero all the bytes */
	len = utf8_to_utf16le(s->str, (uint16_t *)&buf[2], len);
	if (len < 0)
		return -EINVAL;
	buf [0] = (len + 1) * 2;
	buf [1] = USB_DT_STRING;
	return buf [0];
}
