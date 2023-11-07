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
 * @file   logs_out.c
 * @brief  log output functions
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include "buildconf.h"

#include <string.h>
#include <stdio.h>
#include <time.h>

#include "logs_out.h"

void timestamp(char * timestr, int maxsize)
{
	time_t ltime;
	struct tm * local_time;

	ltime = time(NULL);
	timestr[0] = 0;

	local_time = localtime(&ltime);
	if(local_time)
		snprintf(timestr, maxsize, "%.2d:%.2d:%.2d",local_time->tm_hour, local_time->tm_min, local_time->tm_sec );
	else
		snprintf(timestr, maxsize, "??:??:??");
}

#ifdef DEBUG
int is_printable_char(unsigned char c)
{
	int i;
	unsigned char specialchar[]={"&#{}()|_@=$!?;+*-"};

	if( (c >= 'A' && c <= 'Z') ||
		(c >= 'a' && c <= 'z') ||
		(c >= '0' && c <= '9') )
	{
		return 1;
	}

	i = 0;
	while(specialchar[i])
	{
		if(specialchar[i] == c)
		{
			return 1;
		}

		i++;
	}

	return 0;
}

void printbuf(void * buf,int size)
{
	#define PRINTBUF_HEXPERLINE 16
	#define PRINTBUF_MAXLINE_SIZE ((3*PRINTBUF_HEXPERLINE)+1+PRINTBUF_HEXPERLINE+2)

	int i,j;
	unsigned char *ptr = buf;
	char tmp[8];
	char str[PRINTBUF_MAXLINE_SIZE];

	memset(str, ' ', PRINTBUF_MAXLINE_SIZE);
	str[PRINTBUF_MAXLINE_SIZE-1] = 0;

	j = 0;
	for(i=0;i<size;i++)
	{
		if(!(i&(PRINTBUF_HEXPERLINE-1)) && i)
		{
			PRINT_DEBUG("%s", str);
			memset(str, ' ', PRINTBUF_MAXLINE_SIZE);
			str[PRINTBUF_MAXLINE_SIZE-1] = 0;
			j = 0;
		}

		snprintf(tmp, sizeof(tmp), "%02X", ptr[i]);
		memcpy(&str[j*3],tmp,2);

		if( is_printable_char(ptr[i]) )
			str[3*PRINTBUF_HEXPERLINE + 1 + j] = ptr[i];
		else
			str[3*PRINTBUF_HEXPERLINE + 1 + j] = '.';

		j++;
	}

	PRINT_DEBUG("%s", str);
}
#endif

