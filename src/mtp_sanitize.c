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
 * @file   mtp_sanitize.c
 * @brief  Detect bad / Fix MTP messages file / folder name.
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include "buildconf.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

/**
 * utf8_validate()
 * Returns 1 if src is a valid UTF-8 string, 0 otherwise.
 * Rejects overlong encodings, surrogates (U+D800–U+DFFF),
 * and code points above U+10FFFF.
 */
static int utf8_validate(const char *src)
{
	const unsigned char *s = (const unsigned char *)src;

   while (*s)
   {
		int bytes;
		unsigned long cp;

		if (*s < 0x80)
		{  /* 0xxxxxxx — ASCII */
			s++;
			continue;
		}
		else if ((*s & 0xE0) == 0xC0)
		{  /* 110xxxxx — 2-byte */
			bytes = 2;
			cp = *s & 0x1F;
		}
		else if ((*s & 0xF0) == 0xE0)
		{  /* 1110xxxx — 3-byte */
			bytes = 3;
			cp = *s & 0x0F;
		}
		else if ((*s & 0xF8) == 0xF0)
		{  /* 11110xxx — 4-byte */
			bytes = 4;
			cp = *s & 0x07;
		}
		else
		{
			return 0;	/* invalid lead byte  */
		}

		for (int i = 1; i < bytes; i++)
		{
			if ((s[i] & 0xC0) != 0x80)
				return 0; /* bad continuation   */

			cp = (cp << 6) | (s[i] & 0x3F);
		}

		/* Overlong: the code point must need exactly `bytes` bytes */
		if (bytes == 2 && cp < 0x80)
			return 0;

		if (bytes == 3 && cp < 0x800)
			return 0;

		if (bytes == 4 && cp < 0x10000)
			return 0;

		/* Surrogates and out-of-range */
		if (cp >= 0xD800 && cp <= 0xDFFF)
			return 0;

		if (cp > 0x10FFFF)
			return 0;

		s += bytes;
	}

	return 1;
}

static int is_forbidden_ascii(unsigned char c)
{
	if (c < 0x20 || c == 0x7F)  return 1;   /* control chars */

	switch (c)
	{
		case '/':
		case '\\':                /* path separators */
		case ':':
		case '*':
		case '?':
		case '"':
		case '<':
		case '>':
		case '|':
		case ';':
			return 1;
		default:
			return 0;
	}
}

/**
 * sanitize_name()
 *
 * @param src   Input string (UTF-8, NUL-terminated).
 *
 * @return      SANITIZE_OK on success, negative error code otherwise.
 */
int sanitize_name(char *str, int max_len)
{
	if (!str)
		return -1;

	/* ---- Step 1: reject path traversal sequences ------------------- */
	/*
	 * We check for ".." anywhere in the component.  A legitimate file
	 * name component should never contain ".." — if one does it is either
	 * an attack or a mistake, so we treat it as an error rather than
	 * silently stripping, forcing the caller to supply a clean name.
	 *
	 * Patterns caught: "..", "../", "..\\", "%2e%2e" is NOT decoded here
	 * (URL decoding must happen before calling this function).
	 */
	if (strstr(str, "..") != NULL)
		return -1;

	if (!utf8_validate(str))
		return -1;

	/* ---- Step 2: copy, replacing forbidden ASCII chars -------------- */
	size_t wi = 0;   /* write index into dst */

	for (size_t ri = 0; str[ri] != '\0'; ri++)
	{
		if( max_len > 0 )
		{
			if (wi >= max_len)
			{
				// truncate ...
				str[wi - 1] = '\0';

				return -1;
			}
		}

		unsigned char c = (unsigned char)str[ri];

		if (c >= 0x80)
		{
			/* Multi-byte UTF-8 continuation or lead byte: pass through. */
			str[wi++] = (char)c;
		}
		else
		{
			if (is_forbidden_ascii(c) )
			{
				str[wi++] = '_';
			}
			else
			{
				str[wi++] = (char)c;
			}
		}
	}

	str[wi] = '\0';

	/* ---- Step 3: strip leading spaces --------------------- */
	/*
	 *  Spaces at the start are almost always user error.  Strip them.
	 *  keep possible leading '.' for hidden files.
	 */
	size_t start = 0;
	while (str[start] == ' ' || str[start] == '/')
		start++;

	if (start > 0)
	{
		int i = start;
		wi = 0;

		while( str[i] )
		{
			str[wi] = str[i];
			i++;
			wi++;
		}
		str[wi] = '\0';
	}

	/* ---- Step 4: strip trailing dots and spaces -------------------- */

	/*
	 * Windows silently strips trailing dots and spaces, which can cause
	 * "file.txt." and "file.txt" to map to the same file.
	 */
	while (wi > 0 && (str[wi - 1] == '.' || str[wi - 1] == ' '))
	{
		wi--;
	}
	str[wi] = '\0';

	/* ---- Step 5: reject empty result ------------------------------- */
	if (wi == 0)
		return -1;

	return 1;
}

int check_realpath(char *rootpath, char *pathtocheck)
{
	int i;
	char * realp;

	realp = realpath( pathtocheck, NULL);
	if(!realp)
		return 0;

	i = 0;
	while( rootpath[i] )
	{
		if( rootpath[i] != realp[i] )
		{
			free(realp);
			return 0;
		}
	}

	free(realp);

	return 1;
}
