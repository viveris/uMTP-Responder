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
 * @file   umtprd.c
 * @brief  Main MTP Daemon program.
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include "buildconf.h"

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/file.h>

#ifdef SYSTEMD_NOTIFY
#include <systemd/sd-login.h>
#include <systemd/sd-daemon.h>
#endif

#include "mtp.h"

#include "usb_gadget.h"
#include "usb_gadget_fct.h"

#include "logs_out.h"

#include "msgqueue.h"

#include "default_cfg.h"

mtp_ctx * mtp_context;

void* io_thread(void* arg)
{
	usb_gadget * ctx;
	int ret;

	prctl(PR_SET_NAME, (unsigned long) __func__);

	ctx = (usb_gadget *)arg;

	while (is_usb_up(ctx))
	{
		ret = mtp_incoming_packet(mtp_context);
		if(ret < 0)
		{
			ctx->stop = 1;
		}
	}

	return NULL;
}

static int main_thread(const char *conffile)
{
	usb_gadget * usb_ctx;
	int retcode = 0;
	int loop_continue = 0;

	mtp_context = mtp_init_responder();
	if(!mtp_context)
	{
		return -1;
	}

	mtp_load_config_file(mtp_context, conffile);

	loop_continue = mtp_context->usb_cfg.loop_on_disconnect;

	do
	{
		usb_ctx = init_usb_mtp_gadget(mtp_context);
		if(usb_ctx)
		{
			mtp_set_usb_handle(mtp_context, usb_ctx, mtp_context->usb_cfg.usb_max_packet_size);

#ifdef SYSTEMD_NOTIFY
			/* Tell systemd that we are ready */
			sd_notify(0, "READY=1");
#endif

			if( mtp_context->usb_cfg.usb_functionfs_mode )
			{
				PRINT_DEBUG("uMTP Responder : FunctionFS Mode - entering handle_ffs_ep0");
				handle_ffs_ep0(usb_ctx);
			}
			else
			{
				PRINT_DEBUG("uMTP Responder : GadgetFS Mode - entering handle_ep0");
				handle_ep0(usb_ctx);
			}
			deinit_usb_mtp_gadget(usb_ctx);
		}
		else
		{
			PRINT_ERROR("USB Init failed !");
			retcode = -2;
			loop_continue = 0;
		}

		PRINT_MSG("uMTP Responder : Disconnected");

		if(mtp_context->fs_db)
		{
			deinit_fs_db(mtp_context->fs_db);
			mtp_context->fs_db = 0;
		}
	}while(loop_continue);

	mtp_deinit_responder(mtp_context);

	return retcode;
}

#define PARAMETER_IPCCMD "-cmd:"
#define PARAMETER_CONF "-conf"

int main(int argc, char *argv[])
{
	const char *conffile = UMTPR_CONF_FILE;
	int retcode;
	mqd_t fd = -1;
	int already_running = 0;

	PRINT_MSG("uMTP Responder");
	PRINT_MSG("Version: %s compiled %s@%s", APP_VERSION,
		  __DATE__, __TIME__);

	PRINT_MSG("(c) 2018 - 2025 Viveris Technologies");

	fd = get_message_queue();
	if (fd < 0)
	{
		exit(1);
	}
	retcode = flock(fd, LOCK_EX | LOCK_NB);
	if (retcode)
	{
		// lock failed, umtprd is already running
		already_running = 1;
	}

	if (argc <= 1 && already_running)
	{
		PRINT_MSG("Already running");
		return 0;
	}

	while(argc > 1)
	{
		if(!strncmp(argv[1], PARAMETER_IPCCMD, sizeof(PARAMETER_IPCCMD) - 1))
		{
			char *cmd = &argv[1][sizeof(PARAMETER_IPCCMD) - 1];
			PRINT_MSG("Sending command : %s", cmd);
			retcode = send_message_queue(cmd);
			if (retcode)
			{
				PRINT_ERROR("Error (%d) sending '%s'", retcode, cmd);
				exit(retcode);
			}
			argc--;
			argv++;
		}
		else if(!strcmp(argv[1], PARAMETER_CONF))
		{
			if (already_running)
			{
				PRINT_ERROR("Can't set config file when %s is already running", argv[0]);
				exit(1);
			}
			if (argc < 3)
			{
				PRINT_ERROR("%s option requires file", PARAMETER_CONF);
				exit(1);
			}
			PRINT_MSG("Using config file %s", argv[2]);
			conffile = argv[2];
			argc -= 2;
			argv += 2;
		}
		else
		{
			PRINT_ERROR("Unknown command line option: %s", argv[1]);
			exit(1);
		}
	}

	if (!already_running)
	{
		PRINT_DEBUG("starting main thread");
		retcode = main_thread(conffile);
		if( retcode )
			PRINT_ERROR("Error : Couldn't run the main thread... (%d)", retcode);
	};

	return -retcode;
}
