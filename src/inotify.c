/*
 * uMTP Responder
 * Copyright (c) 2018 Viveris Technologies
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
 * @file   inotify.c
 * @brief  inotify file system events handler.
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <errno.h>

#include <poll.h>
#include <sys/inotify.h>

#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>

#include "logs_out.h"

#include "mtp_helpers.h"

#include "fs_handles_db.h"

#include "mtp.h"
#include "mtp_datasets.h"

#include "usb_gadget_fct.h"

#include "inotify.h"

#define INOTIFY_RD_BUF_SIZE ( 1024 * (  ( sizeof (struct inotify_event) ) + 16 ) )

void* inotify_thread(void* arg)
{
	mtp_ctx * ctx;
	int i,length;

	char buffer[INOTIFY_RD_BUF_SIZE] __attribute__ ((aligned(__alignof__(struct inotify_event))));
	const struct inotify_event *event;

	ctx = (mtp_ctx *)arg;

	i = 0;

	for (;;)
	{
		length = read(ctx->inotify_fd, buffer, sizeof buffer);
		if (length >= 0)
		{
			while ( i < length )
			{
				event = ( struct inotify_event * ) &buffer[ i ];
				if ( event->len )
				{
					if ( event->mask & IN_CREATE )
					{
						if ( event->mask & IN_ISDIR )
						{
							printf( "New directory %s created.\n", event->name );
						}
						else
						{
							printf( "New file %s created.\n", event->name );
						}
					}
					else
					{
						if ( event->mask & IN_DELETE )
						{
							if ( event->mask & IN_ISDIR )
							{
								printf( "Directory %s deleted.\n", event->name );
							}
							else
							{
								printf( "File %s deleted.\n", event->name );
							}
						}
					}

					i +=  (( sizeof (struct inotify_event) ) + event->len);
				}
			}
		}
	}

	return NULL;

}

int inotify_handler_init( mtp_ctx * ctx )
{
	if( ctx )
	{
		ctx->inotify_fd = inotify_init1(0x00);

		PRINT_DEBUG("init_inotify_handler : inotify_fd = %d", ctx->inotify_fd);

		//pthread_create(&ctx->inotify_thread, NULL, inotify_thread, ctx);

		return 1;
	}

	return 0;
}

int inotify_handler_deinit( mtp_ctx * ctx )
{
	if( ctx )
	{
		if( ctx->inotify_fd != -1 )
		{
			close( ctx->inotify_fd );
			ctx->inotify_fd = -1;
		}

		return 1;
	}

	return 0;
}

int inotify_handler_addwatch( mtp_ctx * ctx, char * path )
{
	int wd;

	wd = inotify_add_watch( ctx->inotify_fd, path, IN_CREATE | IN_DELETE );

	if( wd < 0 )
		return 0;
	else
		return 1;
}

