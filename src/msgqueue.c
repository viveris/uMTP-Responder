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
 * @file   msgqueue.c
 * @brief  Message queue handler.
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include "buildconf.h"

#include <inttypes.h>
#include <pthread.h>
#include <fcntl.h>
#include <mqueue.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <sys/prctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>

#include "mtp.h"
#include "mtp_cfg.h"
#include "mtp_helpers.h"
#include "mtp_constant.h"
#include "mtp_datasets.h"
#include "mtp_ops_helpers.h"

#include "usb_gadget_fct.h"
#include "fs_handles_db.h"
#include "inotify.h"
#include "logs_out.h"

// minimum message size for POSIX queues on Linux
#define MAX_MSG_SIZE 128

static struct mq_attr qattrs = {
  0,  // flags
  20, // max number of messages on the queue
  MAX_MSG_SIZE,
  0   // number of messages currently on the queue
};

static void *msgqueue_gotsig(int sig, siginfo_t *info, void *ucontext)
{
	return NULL;
}

static void* msgqueue_thread( void* arg )
{
	mtp_ctx * ctx;
	char message[MAX_MSG_SIZE + 1];
	uint32_t handle[3];
	int store_index;
	struct sigaction sa;

	prctl(PR_SET_NAME, (unsigned long) __func__);

	ctx = (mtp_ctx *)arg;

	sa.sa_handler = NULL;
	sa.sa_sigaction =  (void *)msgqueue_gotsig;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);

	if (sigaction(SIGUSR1, &sa, NULL) < 0)
	{
		return (void *)-1;
	}

	PRINT_DEBUG("%s : Thread started", __func__);

	do
	{
		ssize_t numreceived = mq_receive(ctx->msgqueue_id, message, MAX_MSG_SIZE + 1, NULL);
		if (numreceived < 0)
		{
			PRINT_ERROR("error receiving: %s", strerror(errno));
		}
		else if (numreceived <= MAX_MSG_SIZE)
		{
			message[numreceived] = 0; // ensure zero termination
			PRINT_DEBUG("%s : New message received : %s", __func__, message);

			if (!strncmp(message,"addstorage:", 11))
			{
				mtp_add_storage_from_line(ctx, message + 11, 0);
			}

			if (!strncmp(message,"rmstorage:", 10))
			{
				mtp_remove_storage_from_line(ctx, message + 10, 0);
			}

			if(!strncmp(message,"mount:",6))
			{
				store_index = mtp_get_storage_index_by_name(ctx, message + 6);

				if(store_index >= 0)
				{
					if( !pthread_mutex_lock( &ctx->inotify_mutex ) )
					{
						mount_store( ctx, store_index, 1 );

						handle[0] = ctx->storages[store_index].storage_id;

						mtp_push_event( ctx, MTP_EVENT_STORE_ADDED, 1, (uint32_t *)&handle );

						if( pthread_mutex_unlock( &ctx->inotify_mutex ) )
						{
							goto error;
						}
					}
				}
				else
				{
					PRINT_ERROR("%s : Store not found : %s", __func__, message + 6);
				}
			}

			if(!strncmp(message,"unmount:",8))
			{
				store_index = mtp_get_storage_index_by_name(ctx, message + 8);
				if(store_index >= 0)
				{
					if( !pthread_mutex_lock( &ctx->inotify_mutex ) )
					{
						umount_store( ctx, store_index, 1 );

						handle[0] = ctx->storages[store_index].storage_id;

						mtp_push_event( ctx, MTP_EVENT_STORE_REMOVED, 1, (uint32_t *)&handle );

						if( pthread_mutex_unlock( &ctx->inotify_mutex ) )
						{
							goto error;
						}
					}
				}
				else
				{
					PRINT_ERROR("%s : Store not found : %s", __func__, message + 8);
				}
			}

			if(!strncmp(message,"lock",4))
			{
				store_index = 0;
				while(store_index < MAX_STORAGE_NB)
				{
					if( ctx->storages[store_index].root_path &&
						(ctx->storages[store_index].flags & UMTP_STORAGE_LOCKABLE) &&
						!(ctx->storages[store_index].flags & UMTP_STORAGE_LOCKED)
					)
					{
						if( !pthread_mutex_lock( &ctx->inotify_mutex ) )
						{
							umount_store( ctx, store_index, 0 );

							ctx->storages[store_index].flags |= UMTP_STORAGE_LOCKED;

							handle[0] = ctx->storages[store_index].storage_id;

							mtp_push_event( ctx, MTP_EVENT_STORE_REMOVED, 1, (uint32_t *)&handle );

							if( pthread_mutex_unlock( &ctx->inotify_mutex ) )
							{
								goto error;
							}
						}
					}

					store_index++;
				}
			}

			if(!strncmp(message,"unlock",6))
			{
				store_index = 0;
				while(store_index < MAX_STORAGE_NB)
				{
					if( ctx->storages[store_index].root_path &&
						(ctx->storages[store_index].flags & UMTP_STORAGE_LOCKABLE) &&
						(ctx->storages[store_index].flags & UMTP_STORAGE_LOCKED)
					)
					{
						if( !pthread_mutex_lock( &ctx->inotify_mutex ) )
						{
							mount_store( ctx, store_index, 0 );

							ctx->storages[store_index].flags &= ~UMTP_STORAGE_LOCKED;

							handle[0] = ctx->storages[store_index].storage_id;

							mtp_push_event( ctx, MTP_EVENT_STORE_ADDED, 1, (uint32_t *)&handle );

							if( pthread_mutex_unlock( &ctx->inotify_mutex ) )
							{
								goto error;
							}
						}
					}

					store_index++;
				}
			}
		}
		else
		{
			PRINT_DEBUG("received message of size %zd", numreceived);
		}

	} while(1);

	PRINT_DEBUG("%s : Leaving thread", __func__);

	return NULL;

error:
	PRINT_DEBUG("%s : General Error ! Leaving thread", __func__);

	return NULL;
}

mqd_t get_message_queue() {
	mqd_t qid = mq_open("/umtprd", O_RDWR | O_CREAT, 0600, &qattrs);
	if (qid < 0)
	{
		PRINT_ERROR("%s : mq_open error %d (%s)", __func__, errno, strerror(errno));
	}
	else
	{
		PRINT_DEBUG("%s : msgqueue_id = %d", __func__, qid);
	}
	return qid;
}

int send_message_queue(char * message )
{
	mqd_t msgqueue_id;

	msgqueue_id = get_message_queue();
	if (msgqueue_id != -1)
	{
		if (mq_send(msgqueue_id, message, strlen(message) + 1, 0) == 0 )
		{
			return 0;
		}
		else
		{
			PRINT_ERROR("%s : Couldn't send %s !", __func__, message);
		}
	}

	return -1;
}

int msgqueue_handler_init(mtp_ctx * ctx )
{
	int ret;

	if( ctx )
	{
		ctx->msgqueue_id = get_message_queue();
		if(ctx->msgqueue_id != -1)
		{
			ret = pthread_create(&ctx->msgqueue_thread, NULL, msgqueue_thread, ctx);
			if(ret != 0)
			{
				PRINT_ERROR("%s : msgqueue_thread thread creation failed ! (error %d)", __func__, ret);
				return -1;
			}
			else
			{
				return 0;
			}
		}
	}

	return -2;
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
			mq_close(ctx->msgqueue_id);
		}

		return 1;
	}

	return 0;
}
