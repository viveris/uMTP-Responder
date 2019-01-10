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
 * @file   logs_out.c
 * @brief  log output functions
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "logs_out.h"

#ifdef DEBUG
void printbuf(void * buf,int size)
{
	int i;
	char *str = NULL;
	char *ptr = buf;
	char tmp[10] = { '\0' };
	int strMaxSz = (size*4)+2;

	str = malloc(strMaxSz);
	if(NULL != str)
	{
		memset(str, '\0', strMaxSz);
		for(i=0;i<size;i++)
		{
			if(!(i&0xF) && i)
			{
				PRINT_DEBUG("%s", str);
				memset(str, '\0', strMaxSz);
			}
			snprintf(tmp, sizeof(tmp), "%02X ", ptr[i]);
			strncat(str, tmp, strMaxSz);
			memset(tmp, '\0', sizeof(tmp));
		}

		if(strlen(str))
			PRINT_DEBUG("%s", str);

		free(str);
	}
}
#endif

