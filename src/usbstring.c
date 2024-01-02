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

#include "buildconf.h"

#include <inttypes.h>

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

int utf8_encode(char *out, uint32_t unicode)
{
	if (unicode <= 0x7F)
	{
		// ASCII
		*out++ = (char) unicode;
		*out++ = 0;
		return 1;
	}
	else if (unicode <= 0x07FF)
	{
		// 2-bytes unicode
		*out++ = (char) (((unicode >> 6) & 0x1F) | 0xC0);
		*out++ = (char) (((unicode >> 0) & 0x3F) | 0x80);
		*out++ = 0;
		return 2;
	}
	else if (unicode <= 0xFFFF)
	{
		// 3-bytes unicode
		*out++ = (char) (((unicode >> 12) & 0x0F) | 0xE0);
		*out++ = (char) (((unicode >>  6) & 0x3F) | 0x80);
		*out++ = (char) (((unicode >>  0) & 0x3F) | 0x80);
		*out++ = 0;
		return 3;
	}
	else if (unicode <= 0x10FFFF)
	{
		// 4-bytes unicode
		*out++ = (char) (((unicode >> 18) & 0x07) | 0xF0);
		*out++ = (char) (((unicode >> 12) & 0x3F) | 0x80);
		*out++ = (char) (((unicode >>  6) & 0x3F) | 0x80);
		*out++ = (char) (((unicode >>  0) & 0x3F) | 0x80);
		*out++ = 0;

		return 4;
	}
	else
	{
		// error
		return 0;
	}
}

int unicode2charstring(char * str, uint16_t * unicodestr, int maxstrsize)
{
	int i,j,ret;
	int chunksize;
	char tmpstr[8];
	unsigned char * byte_access;

	// Use bytes accesses instead of half word accesses to support unaligned addresses on pre ARMv7 CPUs.
	byte_access = (unsigned char *)unicodestr;

	ret = 0;
	i = 0;
	while( byte_access[0] || byte_access[1] )
	{
		chunksize = utf8_encode( (char*)&tmpstr, ( ((uint16_t)byte_access[1]) << 8 ) | byte_access[0] );
		byte_access += 2;

		if(!chunksize)
		{	// Error -> default character
			tmpstr[0] = '?';
			tmpstr[1] = 0;
			chunksize = 1;
		}

		if( (i + chunksize) < maxstrsize )
		{
			for( j = 0 ; j < chunksize ; j++ )
			{
				str[i] = tmpstr[j];
				i++;
			}
		}
		else
		{
			str[ maxstrsize - 1 ] = 0;
			ret = 1;
			break;
		}
	};

	if( i < maxstrsize )
		str[i] = 0;

	return ret;
}

static int utf8_size(char c)
{
	int count = 0;

	while (c & 0x80)
	{
		c = c << 1;
		count++;
	}

	if(!count)
		count = 1;

	return count;
}

uint16_t utf2unicode(const unsigned char* pInput, int * ofs)
{
	uint8_t b1, b2, b3;
	int utfsize;
	uint16_t unicode_out;

	*ofs = 0;

	utfsize = utf8_size(*pInput);

	switch ( utfsize )
	{
		case 1:
			unicode_out = *pInput;
			*ofs = 1;
			return unicode_out;
		break;

		case 2:
			b1 = *pInput++;
			b2 = *pInput++;

			if ( (b2 & 0xC0) != 0x80 )
				return 0x0000;

			unicode_out =  ( ( (b1 & 0x1F) << 6 ) | (b2 & 0x3F) ) & 0xFF;
			unicode_out |= (uint16_t)((b1 & 0x1F) >> 2) << 8;
			*ofs = 2;
			return unicode_out;
		break;

		case 3:
			b1 = *pInput++;
			b2 = *pInput++;
			b3 = *pInput++;

			if ( ((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80) )
				return 0x0000;

			unicode_out =    (uint16_t)( ((b2 & 0x3F) << 6) |  (b3 & 0x3F) ) & 0x00FF;
			unicode_out |=   (uint16_t)( ((b1 &  0xF) << 4) | ((b2 & 0x3F) >> 2 ) ) << 8;

			*ofs = 3;

			return unicode_out;
		break;

		default:
			*ofs = 0;

			return 0x0000;
		break;
	}

	return 0x0000;
}

int char2unicodestring(char * unicodestr, int index, int maxsize, char * str, int unicodestrsize)
{
	uint16_t unicode;
	int ofs, len, start;
	int unicode_size;

	start = index;
	len = 0;
	ofs = 0;
	unicode_size = 0;
	do
	{
		unicode = utf2unicode((unsigned char*)str, &ofs);
		str = str + ofs;

		if(index + 2 >= maxsize)
			return -1;

		unicodestr[index++] = unicode & 0xFF;
		unicodestr[index++] = (unicode >> 8) & 0xFF;

		unicode_size += 2;

		len++;
	} while(unicode && ofs && unicode_size < unicodestrsize*2 && unicode_size < 255 * 2 );

	if( unicode_size >= 255 * 2 )
	{
		if(start + ((unicodestrsize*2)-2) >= maxsize)
			return -1;

		unicodestr[start + ((255*2)-2)] = 0x00;
		unicodestr[start + ((255*2)-1)] = 0x00;

		len = 255;
	}

	return len;
}
