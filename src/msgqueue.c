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
 * @file   msgqueue.c
 * @brief  Message queue handler.
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include "buildconf.h"

#include <inttypes.h>
#include <pthread.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <signal.h>

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>

#include "mtp.h"
#include "mtp_helpers.h"
#include "mtp_constant.h"
#include "mtp_datasets.h"

#include "usb_gadget_fct.h"
#include "fs_handles_db.h"
#include "inotify.h"
#include "logs_out.h"

typedef struct mesg_buffer_ {
	long mesg_type;
	char mesg_text[100];
} queue_msg_buf;

void *msgqueue_gotsig(int sig, siginfo_t *info, void *ucontext)
{
	return NULL;
}

void* msgqueue_thread( void* arg )
{
	mtp_ctx * ctx;
	queue_msg_buf msg_buf;
	uint32_t handle[3];
	fs_entry * entry;
	int store_index;
	struct sigaction sa;

	ctx = (mtp_ctx *)arg;

	sa.sa_handler = NULL;
	sa.sa_sigaction =  (void *)msgqueue_gotsig;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);

	if (sigaction(SIGUSR1, &sa, NULL) < 0)
	{
		return (void *)-1;
	}

	PRINT_DEBUG("msgqueue_thread : Thread started");

	do
	{
		if( msgrcv(ctx->msgqueue_id, &msg_buf, sizeof(msg_buf), 1, 0) > 0 )
		{
			PRINT_DEBUG("msgqueue_thread : New message received : %s",msg_buf.mesg_text);

			if(!strncmp((char*)&msg_buf.mesg_text,"mount:",6))
			{
				store_index = mtp_get_storage_index_by_name(ctx, (char*)&msg_buf.mesg_text + 6);

				if(store_index >= 0)
				{
					pthread_mutex_lock( &ctx->inotify_mutex );

					if( ctx->storages[store_index].flags & UMTP_STORAGE_NOTMOUNTED )
					{
						alloc_root_entry(ctx->fs_db, ctx->storages[store_index].storage_id);
					}

					ctx->storages[store_index].flags &= ~UMTP_STORAGE_NOTMOUNTED;

					handle[0] = ctx->storages[store_index].storage_id;
					mtp_push_event( ctx, MTP_EVENT_STORE_ADDED, 1, (uint32_t *)&handle );

					pthread_mutex_unlock( &ctx->inotify_mutex );
				}
				else
				{
					PRINT_ERROR("msgqueue_thread : Store not found : %s",(char*)&msg_buf.mesg_text + 6);
				}
			}

			if(!strncmp((char*)&msg_buf.mesg_text,"unmount:",8))
			{
				store_index = mtp_get_storage_index_by_name(ctx, (char*)&msg_buf.mesg_text + 8);
				if(store_index >= 0)
				{
					pthread_mutex_lock( &ctx->inotify_mutex );

					entry = NULL;
					do
					{
						entry = get_entry_by_storageid( ctx->fs_db, ctx->storages[store_index].storage_id, entry );
						if(entry)
						{
							entry->flags |= ENTRY_IS_DELETED;
							if( entry->watch_descriptor != -1 )
							{
								inotify_handler_rmwatch( ctx, entry->watch_descriptor );
								entry->watch_descriptor = -1;
							}
							entry = entry->next;
						}
					}while(entry);

					ctx->storages[store_index].flags |= UMTP_STORAGE_NOTMOUNTED;

					handle[0] = ctx->storages[store_index].storage_id;
					mtp_push_event( ctx, MTP_EVENT_STORE_REMOVED, 1, (uint32_t *)&handle );

					pthread_mutex_unlock( &ctx->inotify_mutex );
				}
				else
				{
					PRINT_ERROR("msgqueue_thread : Store not found : %s",(char*)&msg_buf.mesg_text + 8);
				}
			}
		}
		else
		{
			break;
		}

	}while(1);

	msgctl(ctx->msgqueue_id, IPC_RMID, NULL);

	PRINT_DEBUG("msgqueue_thread : Leaving msgqueue_thread...");

	return NULL;
}

int get_current_exec_path( char * exec_path, int maxsize )
{
	pid_t pid;
	char path[PATH_MAX];
	char tmp_exec_path[PATH_MAX + 1];

	memset(tmp_exec_path,0,sizeof(tmp_exec_path));

	pid = getpid();

	sprintf(path, "/proc/%d/exe", pid);

	if (readlink(path, tmp_exec_path, PATH_MAX) == -1)
	{
		return -1;
	}

	tmp_exec_path[PATH_MAX] = 0;

	if(strlen(tmp_exec_path) < maxsize)
	{
		strcpy(exec_path,tmp_exec_path);
		return 0;
	}
	else
		return -2;
}

int send_message_queue( char * message )
{
	key_t key;
	int msgqueue_id;
	char exec_path[PATH_MAX + 1];
	queue_msg_buf msg_buf;

	if(get_current_exec_path(exec_path, sizeof(exec_path)) >= 0)
	{
		PRINT_DEBUG("send_message_queue : current exec path : %s", exec_path);

		key = ftok(exec_path, 44);

		msgqueue_id = msgget(key, 0666 | IPC_CREAT);

		PRINT_DEBUG("send_message_queue : msgqueue_id = %d", msgqueue_id);

		msg_buf.mesg_type = 1;
		strncpy(msg_buf.mesg_text,message,sizeof(msg_buf.mesg_text));
		if( msgsnd(msgqueue_id, &msg_buf, sizeof(msg_buf), 0) == 0 )
		{
			return 0;
		}
	}

	PRINT_ERROR("send_message_queue : Couldn't send %s !", message);

	return -1;
}

int msgqueue_handler_init( mtp_ctx * ctx )
{
	key_t key;
	char exec_path[PATH_MAX + 1];
	int ret;

	if( ctx )
	{
		if(get_current_exec_path(exec_path, sizeof(exec_path)) >= 0)
		{
			PRINT_DEBUG("msgqueue_handler_init : current exec path : %s", exec_path);

			key = ftok(exec_path, 44);

			ctx->msgqueue_id = msgget(key, 0666 | IPC_CREAT);

			if(ctx->msgqueue_id != -1)
			{
				PRINT_DEBUG("msgqueue_handler_init : msgqueue_id = %d", ctx->msgqueue_id);

				ret = pthread_create(&ctx->msgqueue_thread, NULL, msgqueue_thread, ctx);
				if(ret != 0)
				{
					PRINT_ERROR("msgqueue_handler_init : msgqueue_thread thread creation failed ! (error %d)", ret);
					return -1;
				}
				else
				{
					return 1;
				}
			}
			else
			{
				PRINT_ERROR("msgqueue_handler_init : msgget error %d", errno);
			}
		}
	}

	return 0;
}

int msgqueue_handler_deinit( mtp_ctx * ctx )
{
	void * ret;

	if( ctx )
	{
		if( ctx->msgqueue_id != -1 )
		{
			pthread_kill( ctx->msgqueue_thread, SIGUSR1);
			pthread_join( ctx->msgqueue_thread, &ret);
			ctx->msgqueue_id = -1;
		}

		return 1;
	}

	return 0;
}
