/*
 * uMTP Responder
 * Copyright (c) 2018 - 2020 Viveris Technologies
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

#include "buildconf.h"

#include <inttypes.h>
#include <pthread.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <signal.h>

#include "mtp.h"
#include "mtp_helpers.h"
#include "mtp_constant.h"
#include "mtp_datasets.h"

#include "usb_gadget_fct.h"
#include "fs_handles_db.h"
#include "inotify.h"
#include "logs_out.h"

#define INOTIFY_RD_BUF_SIZE ( 32*1024 )

static int get_file_info(mtp_ctx * ctx, const struct inotify_event *event, fs_entry * entry, filefoundinfo * fileinfo, int deleted)
{
	char * path;
	char * tmp_path;

	if( entry && fileinfo )
	{
		PRINT_DEBUG( "Entry %x - %s", entry->handle, entry->name );

		path = build_full_path( ctx->fs_db, mtp_get_storage_root( ctx, entry->storage_id), entry);
		if( path )
		{
			tmp_path = malloc( strlen(path) + event->len + 3 );
			if( tmp_path )
			{
				strcpy( tmp_path, path );
				strcat( tmp_path, "/" );
				strncat( tmp_path, event->name, event->len );

				if( !deleted )
				{
					fs_entry_stat( tmp_path, fileinfo );
				}
				else
				{
					fileinfo->isdirectory = 0;
					fileinfo->size = 0;
					strncpy( fileinfo->filename, event->name, 256 );
				}

				free( tmp_path );

				return 1;
			}
			free( path );
		}
	}

	return 0;
}

void *inotify_gotsig(int sig, siginfo_t *info, void *ucontext)
{
	return NULL;
}

void* inotify_thread(void* arg)
{
	mtp_ctx * ctx;
	int i,length;
	fs_entry * entry;
	fs_entry * deleted_entry;
	fs_entry * modified_entry;
	fs_entry * new_entry;
	fs_entry * old_entry;
	filefoundinfo fileinfo;
	uint32_t handle[3];
	char inotify_buffer[INOTIFY_RD_BUF_SIZE] __attribute__ ((aligned(__alignof__(struct inotify_event))));
	const struct inotify_event *event;
	struct sigaction sa;

	ctx = (mtp_ctx *)arg;

	sa.sa_handler = NULL;
	sa.sa_sigaction =  (void *)inotify_gotsig;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);

	if (sigaction(SIGUSR1, &sa, NULL) < 0)
	{
		return (void *)-1;
	}

	for (;;)
	{
		memset(inotify_buffer,0,sizeof(inotify_buffer));
		length = read(ctx->inotify_fd, inotify_buffer, sizeof(inotify_buffer));

		if ( length >= 0 )
		{
			if(!length)
				PRINT_DEBUG( "inotify_thread : Null sized packet ?");

			i = 0;
			while ( i < length && i < INOTIFY_RD_BUF_SIZE )
			{
				event = ( struct inotify_event * ) &inotify_buffer[ i ];

				// Sanity check to prevent possible buffer overrun/overflow.
				if ( event->len && (i + (( sizeof (struct inotify_event) ) + event->len) < sizeof(inotify_buffer)) )
				{
					if ( event->mask & IN_CREATE )
					{
						entry = NULL;
						do
						{
							pthread_mutex_lock( &ctx->inotify_mutex );
							entry = get_entry_by_wd( ctx->fs_db, event->wd, entry );
							if ( get_file_info( ctx, event, entry, &fileinfo, 0 ) )
							{
								old_entry = search_entry(ctx->fs_db, &fileinfo, entry->handle, entry->storage_id);
								if( !old_entry )
								{
									// If the entry is not in the db, add it and trigger an MTP_EVENT_OBJECT_ADDED event
									new_entry = add_entry( ctx->fs_db, &fileinfo, entry->handle, entry->storage_id );

									// Send an "ObjectAdded" (0x4002) MTP event message with the entry handle.
									handle[0] = new_entry->handle;
									mtp_push_event( ctx, MTP_EVENT_OBJECT_ADDED, 1, (uint32_t *)&handle );

									PRINT_DEBUG( "inotify_thread (IN_CREATE): Entry %s created (Handle 0x%.8X)", event->name, new_entry->handle );
								}
								else
								{
									PRINT_DEBUG( "inotify_thread (IN_CREATE): Entry %s already in the db ! (Handle 0x%.8X)", event->name, old_entry->handle );
								}
							}
							else
							{
								PRINT_DEBUG( "inotify_thread (IN_CREATE): Watch point descriptor not found in the db ! (Descriptor 0x%.8X)", event->wd );
							}

							if(entry)
							{
								entry = entry->next;
							}

							pthread_mutex_unlock( &ctx->inotify_mutex );
						}while(entry);
					}

					if ( event->mask & IN_MODIFY )
					{
						entry = NULL;

						do
						{
							pthread_mutex_lock( &ctx->inotify_mutex );

							entry = get_entry_by_wd( ctx->fs_db, event->wd, entry );
							if ( get_file_info( ctx, event, entry, &fileinfo, 1 ) )
							{
								modified_entry = search_entry(ctx->fs_db, &fileinfo, entry->handle, entry->storage_id);
								if( modified_entry )
								{
									// Send an "ObjectInfoChanged" (0x4007) MTP event message with the entry handle.
									handle[0] = modified_entry->handle;
									mtp_push_event( ctx, MTP_EVENT_OBJECT_INFO_CHANGED, 1, (uint32_t *)&handle );

									PRINT_DEBUG( "inotify_thread (IN_MODIFY): Entry %s modified (Handle 0x%.8X)", event->name, modified_entry->handle);
								}
							}
							else
							{
								PRINT_DEBUG( "inotify_thread (IN_MODIFY): Watch point descriptor not found in the db ! (Descriptor 0x%.8X)", event->wd );
							}

							if(entry)
							{
								entry = entry->next;
							}

							pthread_mutex_unlock( &ctx->inotify_mutex );
						}while(entry);
					}

					if ( event->mask & IN_DELETE )
					{
						entry = NULL;

						do
						{
							pthread_mutex_lock( &ctx->inotify_mutex );

							entry = get_entry_by_wd( ctx->fs_db, event->wd, entry );
							if ( get_file_info( ctx, event, entry, &fileinfo, 1 ) )
							{
								deleted_entry = search_entry(ctx->fs_db, &fileinfo, entry->handle, entry->storage_id);
								if( deleted_entry )
								{
									deleted_entry->flags |= ENTRY_IS_DELETED;
									if( deleted_entry->watch_descriptor != -1 )
									{
										inotify_handler_rmwatch( ctx, deleted_entry->watch_descriptor );
										deleted_entry->watch_descriptor = -1;
									}

									// Send an "ObjectRemoved" (0x4003) MTP event message with the entry handle.
									handle[0] = deleted_entry->handle;
									mtp_push_event( ctx, MTP_EVENT_OBJECT_REMOVED, 1, (uint32_t *)&handle );

									PRINT_DEBUG( "inotify_thread (IN_DELETE): Entry %s deleted (Handle 0x%.8X)", event->name, deleted_entry->handle);
								}
							}
							else
							{
								PRINT_DEBUG( "inotify_thread (IN_DELETE): Watch point descriptor not found in the db ! (Descriptor 0x%.8X)", event->wd );
							}

							if(entry)
							{
								entry = entry->next;
							}

							pthread_mutex_unlock( &ctx->inotify_mutex );
						}while(entry);
					}
				}

				i +=  (( sizeof (struct inotify_event) ) + event->len);
			}
		}
		else
		{
			PRINT_DEBUG( "inotify_thread : read error %d",length );
			return NULL;
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

		pthread_create(&ctx->inotify_thread, NULL, inotify_thread, ctx);

		return 1;
	}

	return 0;
}

int inotify_handler_deinit( mtp_ctx * ctx )
{
	void * ret;

	if( ctx )
	{
		if( ctx->inotify_fd != -1 )
		{
			pthread_kill( ctx->inotify_thread, SIGUSR1);
			pthread_join( ctx->inotify_thread, &ret);
			close( ctx->inotify_fd );
			ctx->inotify_fd = -1;
		}

		return 1;
	}

	return 0;
}

int inotify_handler_addwatch( mtp_ctx * ctx, char * path )
{
	if( ctx->inotify_fd != -1 )
	{
		if( !ctx->no_inotify )
		{
			return inotify_add_watch( ctx->inotify_fd, path, IN_CREATE | IN_DELETE | IN_MODIFY );
		}
	}

	return -1;
}

int inotify_handler_rmwatch( mtp_ctx * ctx, int wd )
{
	if( ctx->inotify_fd != -1 && wd != -1 )
	{
		return inotify_rm_watch( ctx->inotify_fd, wd );
	}

	return -1;
}
