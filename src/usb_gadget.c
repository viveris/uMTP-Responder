/*
 * uMTP Responder
 * Copyright (c) 2018 - 2024 Viveris Technologies
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
 * @file   usb_gadget.c
 * @brief  USB GadgetFS & FunctionFS layer.
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

// GadgetFS support : Main inspiration from Grégory Soutadé (http://blog.soutade.fr/post/2016/07/create-your-own-usb-gadget-with-gadgetfs.html)

#include "buildconf.h"

#include <endian.h>
#include <inttypes.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/ioctl.h>

#include <linux/usb/ch9.h>
#include <linux/usb/gadgetfs.h>

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <signal.h>

#include <errno.h>
#ifdef CONFIG_USB_NON_BLOCKING_WRITE
#include <poll.h>
#endif

#include "fs_handles_db.h"
#include "mtp.h"
#include "mtp_constant.h"

#include "usbstring.h"
#include "usb_gadget.h"

#include "usb_gadget_fct.h"

#include "logs_out.h"

#define CONFIG_VALUE 1

static struct usb_gadget_strings strings = {
	.language = 0x0409, /* en-us */
	.strings = 0,
};

extern void* io_thread(void* arg);
extern mtp_ctx * mtp_context;

typedef struct mtp_device_status_ {
	uint16_t wLength;
	uint16_t wCode;
}mtp_device_status;

int read_usb(usb_gadget * ctx, unsigned char * buffer, int maxsize)
{
	int ret;

	ret = -1;
	if(ctx->ep_handles[EP_DESCRIPTOR_OUT] >= 0 && maxsize && buffer)
	{
		ret = read (ctx->ep_handles[EP_DESCRIPTOR_OUT], buffer, maxsize);
	}

	return ret;
}

int write_usb(usb_gadget * ctx, int channel, unsigned char * buffer, int size)
{
	int ret;
#ifdef CONFIG_USB_NON_BLOCKING_WRITE
	struct pollfd pfd;
#endif

	ret = -1;
	if ( channel < EP_NB_OF_DESCRIPTORS )
	{
		if(ctx->ep_handles[channel] >= 0 && buffer && !mtp_context->cancel_req)
		{

#ifdef CONFIG_USB_NON_BLOCKING_WRITE
			fcntl(ctx->ep_handles[channel], F_SETFL, fcntl(ctx->ep_handles[channel], F_GETFL) | O_NONBLOCK);
			do
			{
				pfd.fd = ctx->ep_handles[channel];
				pfd.events = POLLOUT;

				ret = poll(&pfd, 1, 2000);
				if(ret>0 && !mtp_context->cancel_req)
				{
					ret = write (ctx->ep_handles[channel], buffer, size);
				}
			}while( ret < 0 && ( (errno == EAGAIN) || (errno == EAGAIN) ) && !mtp_context->cancel_req );
			fcntl(ctx->ep_handles[channel], F_SETFL, fcntl(ctx->ep_handles[channel], F_GETFL) & ~O_NONBLOCK);
#else

			ret = write (ctx->ep_handles[channel], buffer, size);

#endif
		}
	}
	return ret;
}

int is_usb_up(usb_gadget * ctx)
{
	if(ctx->stop)
		return 0;
	else
		return 1;
}

void fill_config_descriptor(mtp_ctx * ctx , usb_gadget * usbctx,struct usb_config_descriptor * desc,int total_size, int hs)
{
	memset(desc,0,sizeof(struct usb_config_descriptor));

	desc->bLength = sizeof(struct usb_config_descriptor);
	desc->bDescriptorType = USB_DT_CONFIG;
	desc->wTotalLength = desc->bLength + total_size;
	desc->bNumInterfaces = 1;
	desc->bConfigurationValue = CONFIG_VALUE;
	if(hs)
		desc->iConfiguration = STRINGID_CONFIG_HS;
	else
		desc->iConfiguration = STRINGID_CONFIG_LS;
	desc->bmAttributes = USB_CONFIG_ATT_ONE;
	desc->bMaxPower = 1;

	PRINT_DEBUG("fill_config_descriptor: (Total Len : %u + %d = %d)", (unsigned int) sizeof(struct usb_config_descriptor), total_size, desc->wTotalLength);
	PRINT_DEBUG_BUF(desc, sizeof(struct usb_config_descriptor));

	return;
}

void fill_dev_descriptor(mtp_ctx * ctx, usb_gadget * usbctx,struct usb_device_descriptor * desc)
{
	memset(desc,0,sizeof(struct usb_device_descriptor));

	desc->bLength = USB_DT_DEVICE_SIZE;
	desc->bDescriptorType = USB_DT_DEVICE;
	desc->bDeviceClass =    ctx->usb_cfg.usb_class;
	desc->bDeviceSubClass = ctx->usb_cfg.usb_subclass;
	desc->bDeviceProtocol = ctx->usb_cfg.usb_protocol;
	desc->idVendor =        ctx->usb_cfg.usb_vendor_id;
	desc->idProduct =       ctx->usb_cfg.usb_product_id;
	desc->bcdDevice =       ctx->usb_cfg.usb_dev_version; // Version
	// Strings
	desc->iManufacturer =   STRINGID_MANUFACTURER;
	desc->iProduct =        STRINGID_PRODUCT;
	desc->iSerialNumber =   STRINGID_SERIAL;
	desc->bNumConfigurations = 1; // Only one configuration

	PRINT_DEBUG("fill_dev_descriptor:");
	PRINT_DEBUG_BUF(desc, sizeof(struct usb_device_descriptor));

	return;
}

void fill_if_descriptor(mtp_ctx * ctx, usb_gadget * usbctx, struct usb_interface_descriptor * desc)
{
	memset(desc,0,sizeof(struct usb_interface_descriptor));

	desc->bLength = sizeof(struct usb_interface_descriptor);
	desc->bDescriptorType = USB_DT_INTERFACE;
	desc->bInterfaceNumber = 0;
	desc->iInterface = 1;
	desc->bAlternateSetting = 0;
	desc->bNumEndpoints = 3;

	desc->bInterfaceClass =    ctx->usb_cfg.usb_class;
	desc->bInterfaceSubClass = ctx->usb_cfg.usb_subclass;
	desc->bInterfaceProtocol = ctx->usb_cfg.usb_protocol;
	if( ctx->usb_cfg.usb_functionfs_mode )
	{
		desc->iInterface = 1;
	}
	else
	{
		desc->iInterface = STRINGID_INTERFACE;
	}

	PRINT_DEBUG("fill_if_descriptor:");
	PRINT_DEBUG_BUF(desc, sizeof(struct usb_interface_descriptor));

	return;
}

void fill_ep_descriptor(mtp_ctx * ctx, usb_gadget * usbctx,struct usb_endpoint_descriptor_no_audio * desc,int index,unsigned int flags)
{
	memset(desc,0,sizeof(struct usb_endpoint_descriptor_no_audio));

	desc->bLength = USB_DT_ENDPOINT_SIZE;
	desc->bDescriptorType = USB_DT_ENDPOINT;

	if(flags & EP_OUT_DIR)
		desc->bEndpointAddress = USB_DIR_OUT | (index);
	else
		desc->bEndpointAddress = USB_DIR_IN  | (index);

	if(flags & EP_BULK_MODE)
	{
		desc->bmAttributes = USB_ENDPOINT_XFER_BULK;
		desc->wMaxPacketSize = flags & EP_SS_MODE ? 1024 : 512;
	}
	else
	{
		desc->bmAttributes =  USB_ENDPOINT_XFER_INT;
		desc->wMaxPacketSize = 28; // HS size
		desc->bInterval = 6;
	}

#if defined(CONFIG_USB_SS_SUPPORT)
	if(flags & EP_SS_MODE)
	{
		ep_cfg_descriptor * ss_descriptor;

		ss_descriptor = (ep_cfg_descriptor *)desc;
		memset(&ss_descriptor->ep_desc_comp,0,sizeof(struct usb_ss_ep_comp_descriptor));

		ss_descriptor->ep_desc_comp.bLength = sizeof(struct usb_ss_ep_comp_descriptor);
		ss_descriptor->ep_desc_comp.bDescriptorType = USB_DT_SS_ENDPOINT_COMP;
	}
#endif

	PRINT_DEBUG("fill_ep_descriptor:");
	PRINT_DEBUG_BUF(desc, sizeof(struct usb_endpoint_descriptor_no_audio));

	return;
}

int init_ep(usb_gadget * ctx,int index,int ffs_mode)
{
	int fd,ret;
	void * descriptor_ptr;
	int descriptor_size;

	PRINT_DEBUG("Init end point %s (%d)",ctx->ep_path[index],index);
	fd = open(ctx->ep_path[index], O_RDWR);
	if ( fd < 0 )
	{
		PRINT_ERROR("init_ep : Endpoint %s (%d) init failed ! : Can't open the endpoint ! (error %d - %m)",ctx->ep_path[index],index,fd);
		goto init_ep_error;
	}

	ctx->ep_handles[index] = fd;

	ctx->ep_config[index]->head = 1;

	descriptor_size = 0;

	if( ctx->usb_ffs_config )
	{
#if defined(CONFIG_USB_SS_SUPPORT)
		descriptor_ptr = (void *)&ctx->usb_ffs_config->ep_desc_ss;
		descriptor_size = sizeof(ep_cfg_descriptor);
#elif defined(CONFIG_USB_HS_SUPPORT)
		descriptor_ptr = (void *)&ctx->usb_ffs_config->ep_desc_hs;
		descriptor_size = sizeof(struct usb_endpoint_descriptor_no_audio);
#elif defined(CONFIG_USB_FS_SUPPORT)
		descriptor_ptr = (void *)&ctx->usb_ffs_config->ep_desc_fs;
		descriptor_size = sizeof(struct usb_endpoint_descriptor_no_audio);
#else

#error Configuration Error ! At least one USB mode support must be enabled ! (CONFIG_USB_FS_SUPPORT/CONFIG_USB_HS_SUPPORT/CONFIG_USB_SS_SUPPORT)

#endif
	}
	else
	{
#if defined(CONFIG_USB_SS_SUPPORT)
		descriptor_ptr = (void *)&ctx->usb_config->ep_desc_ss;
		descriptor_size = sizeof(ep_cfg_descriptor);
#elif defined(CONFIG_USB_HS_SUPPORT)
		descriptor_ptr = (void *)&ctx->usb_config->ep_desc_hs;
		descriptor_size = sizeof(struct usb_endpoint_descriptor_no_audio);
#elif defined(CONFIG_USB_FS_SUPPORT)
		descriptor_ptr = (void *)&ctx->usb_config->ep_desc_fs;
		descriptor_size = sizeof(struct usb_endpoint_descriptor_no_audio);
#else

#error Configuration Error ! At least one USB mode support must be enabled ! (CONFIG_USB_FS_SUPPORT/CONFIG_USB_HS_SUPPORT/CONFIG_USB_SS_SUPPORT)

#endif
	}

#if defined(CONFIG_USB_SS_SUPPORT)
	switch(index)
	{
		case EP_DESCRIPTOR_IN:
			memcpy(&ctx->ep_config[index]->ep_desc[0], &((SSEndPointsDesc*)descriptor_ptr)->ep_desc_in,descriptor_size);
			memcpy(&ctx->ep_config[index]->ep_desc[1], &((SSEndPointsDesc*)descriptor_ptr)->ep_desc_in,descriptor_size);
		break;
		case EP_DESCRIPTOR_OUT:
			memcpy(&ctx->ep_config[index]->ep_desc[0], &((SSEndPointsDesc*)descriptor_ptr)->ep_desc_out,descriptor_size);
			memcpy(&ctx->ep_config[index]->ep_desc[1], &((SSEndPointsDesc*)descriptor_ptr)->ep_desc_out,descriptor_size);
		break;
		case EP_DESCRIPTOR_INT_IN:
			memcpy(&ctx->ep_config[index]->ep_desc[0], &((SSEndPointsDesc*)descriptor_ptr)->ep_desc_int_in,descriptor_size);
			memcpy(&ctx->ep_config[index]->ep_desc[1], &((SSEndPointsDesc*)descriptor_ptr)->ep_desc_int_in,descriptor_size);
		break;
	}
#else
	switch(index)
	{
		case EP_DESCRIPTOR_IN:
			memcpy(&ctx->ep_config[index]->ep_desc[0], &((EndPointsDesc*)descriptor_ptr)->ep_desc_in,descriptor_size);
			memcpy(&ctx->ep_config[index]->ep_desc[1], &((EndPointsDesc*)descriptor_ptr)->ep_desc_in,descriptor_size);
		break;
		case EP_DESCRIPTOR_OUT:
			memcpy(&ctx->ep_config[index]->ep_desc[0], &((EndPointsDesc*)descriptor_ptr)->ep_desc_out,descriptor_size);
			memcpy(&ctx->ep_config[index]->ep_desc[1], &((EndPointsDesc*)descriptor_ptr)->ep_desc_out,descriptor_size);
		break;
		case EP_DESCRIPTOR_INT_IN:
			memcpy(&ctx->ep_config[index]->ep_desc[0], &((EndPointsDesc*)descriptor_ptr)->ep_desc_int_in,descriptor_size);
			memcpy(&ctx->ep_config[index]->ep_desc[1], &((EndPointsDesc*)descriptor_ptr)->ep_desc_int_in,descriptor_size);
		break;
	}
#endif

	PRINT_DEBUG("init_ep (%d):",index);
	PRINT_DEBUG_BUF(ctx->ep_config[index], sizeof(ep_cfg));

	if(!ffs_mode)
	{
		ret = write(fd, ctx->ep_config[index], sizeof(ep_cfg));

		if (ret != sizeof(ep_cfg))
		{
			PRINT_ERROR("init_ep : Endpoint %s (%d) init failed ! : Write Error %d - %m",ctx->ep_path[index], index, ret);
			goto init_ep_error;
		}
	}
	else
	{
		PRINT_DEBUG("init_ep (%d): FunctionFS Mode - Don't write the endpoint descriptor.",index);
	}

	return fd;

init_ep_error:

	if(fd >= 0)
		close(fd);

	return -1;
}

int init_eps(usb_gadget * ctx, int ffs_mode)
{
	if( init_ep(ctx, EP_DESCRIPTOR_IN, ffs_mode) < 0 )
		goto init_eps_error;

	if( init_ep(ctx, EP_DESCRIPTOR_OUT, ffs_mode) < 0 )
		goto init_eps_error;

	if( init_ep(ctx, EP_DESCRIPTOR_INT_IN, ffs_mode) < 0 )
		goto init_eps_error;

	return 0;

init_eps_error:
	return 1;
}

static void handle_setup_request(usb_gadget * ctx, struct usb_ctrlrequest* setup)
{
	int status,cnt;
	uint8_t buffer[512];
	mtp_device_status dstatus;

	PRINT_DEBUG("Setup request 0x%.2X", setup->bRequest);

	switch (setup->bRequest)
	{
		case USB_REQ_GET_DESCRIPTOR:
			if (setup->bRequestType != USB_DIR_IN)
				goto stall;

			switch (setup->wValue >> 8)
			{
				case USB_DT_STRING:
					PRINT_DEBUG("Get string id #%d (max length %d)", setup->wValue & 0xff,
						setup->wLength);
					status = usb_gadget_get_string (&strings, setup->wValue & 0xff, buffer);
					// Error
					if (status < 0)
					{
						PRINT_ERROR("handle_setup_request : String id #%d (max length %d) not found !",setup->wValue & 0xff, setup->wLength);
						break;
					}
					else
					{
						PRINT_DEBUG("Found %d bytes", status);
						PRINT_DEBUG_BUF(buffer, status);
					}

					if ( write (ctx->usb_device, buffer, status) < 0 )
					{
						PRINT_ERROR("handle_setup_request - USB_REQ_GET_DESCRIPTOR : usb device write error !");
						break;
					}

					return;
				break;
				default:
					PRINT_DEBUG("Cannot return descriptor %d", (setup->wValue >> 8));
				break;
			}
		break;

		case USB_REQ_SET_CONFIGURATION:
			if (setup->bRequestType != USB_DIR_OUT)
			{
				PRINT_DEBUG("Bad dir");
				goto stall;
			}

			switch (setup->wValue)
			{
				case CONFIG_VALUE:
					PRINT_DEBUG("Set config value");

					if (ctx->ep_handles[EP_DESCRIPTOR_IN] <= 0)
					{
						status = init_eps(ctx,0);
					}
					else
						status = 0;

					if (!status)
					{
						ctx->stop = 0;
						if( ctx->thread_not_started )
							ctx->thread_not_started = pthread_create(&ctx->thread, NULL, io_thread, ctx);
					}
					break;
				case 0:
					PRINT_DEBUG("Disable threads");
					ctx->stop = 1;
					break;

				default:
					PRINT_DEBUG("Unhandled configuration value %d", setup->wValue);
					break;
			}

			// Just ACK
			status = read (ctx->usb_device, &status, 0);
			return;
		break;

		case USB_REQ_GET_INTERFACE:
			PRINT_DEBUG("GET_INTERFACE");
			buffer[0] = 0;

			if ( write (ctx->usb_device, buffer, 1) < 0 )
			{
				PRINT_ERROR("handle_setup_request - USB_REQ_GET_INTERFACE : usb device write error !");
				break;
			}

			return;
		break;

		case USB_REQ_SET_INTERFACE:
			PRINT_DEBUG("SET_INTERFACE");
			ioctl (ctx->ep_handles[EP_DESCRIPTOR_IN], GADGETFS_CLEAR_HALT);
			ioctl (ctx->ep_handles[EP_DESCRIPTOR_OUT], GADGETFS_CLEAR_HALT);
			ioctl (ctx->ep_handles[EP_DESCRIPTOR_INT_IN], GADGETFS_CLEAR_HALT);
			// ACK
			status = read (ctx->usb_device, &status, 0);
			return;
		break;

		case MTP_REQ_CANCEL:
			PRINT_DEBUG("MTP_REQ_CANCEL !");

			status = read (ctx->usb_device, &status, 0);

			mtp_context->cancel_req = 1;

			cnt = 0;
			while( mtp_context->cancel_req )
			{
				// Wait the end of the current transfer
				if( cnt > 250 )
				{
					// Timeout... Unblock pending usb read/write.
					PRINT_DEBUG("MTP_REQ_CANCEL : Forcing read/write exit...");
					pthread_kill(ctx->thread, SIGUSR1);
					usleep(500);
					break;
				}
				else
					usleep(1000);

				cnt++;
			}

			while( mtp_context->cancel_req )
			{
				// Still Waiting the end of the current transfer...
				if( cnt > 500 )
				{
					// Still blocked... Killing the link
					PRINT_DEBUG("MTP_REQ_CANCEL : Stalled ... Killing the link...");
					usleep(500);

					ctx->stop = 1;
					break;
				}
				else
					usleep(1000);

				cnt++;
			}

			PRINT_DEBUG("MTP_REQ_CANCEL done !");

			return;
		break;

		case MTP_REQ_RESET:
			PRINT_DEBUG("MTP_REQ_RESET !");

			// Just ACK
			status = read (ctx->usb_device, &status, 0);

			return;
		break;

		case MTP_REQ_GET_DEVICE_STATUS:
			PRINT_DEBUG("MTP_REQ_GET_DEVICE_STATUS !");

			dstatus.wLength = sizeof(mtp_device_status);
			dstatus.wCode = MTP_RESPONSE_OK;

			if ( write (ctx->usb_device, (void*)&dstatus, sizeof(mtp_device_status) ) < 0 )
			{
				PRINT_ERROR("MTP_REQ_GET_DEVICE_STATUS : usb device write error !");
				break;
			}

			return;
		break;
	}

stall:
	PRINT_DEBUG("Stalled");
	// Error
	if (setup->bRequestType & USB_DIR_IN)
	{
		if ( read (ctx->usb_device, &status, 0) < 0 )
		{
			PRINT_DEBUG("handle_setup_request - stall : usb device read error !");
		}
	}
	else
	{
		if ( write (ctx->usb_device, &status, 0) < 0 )
		{
			PRINT_DEBUG("handle_setup_request - stall : usb device write error !");
		}
	}
}

// GadgetFS mode handler
int handle_ep0(usb_gadget * ctx)
{
	struct timeval timeout;
	int ret, nevents, i, cnt;
	fd_set read_set;
	struct usb_gadgetfs_event events[5];

	PRINT_DEBUG("handle_ep0 : Entering...");
	timeout.tv_sec = 4;
	timeout.tv_usec = 0;

	while (!ctx->stop)
	{
		FD_ZERO(&read_set);
		FD_SET(ctx->usb_device, &read_set);

		if(timeout.tv_sec)
		{
			ret = select(ctx->usb_device+1, &read_set, NULL, NULL, &timeout);
		}
		else
		{
			PRINT_DEBUG("handle_ep0 : Select without timeout");
			ret = select(ctx->usb_device+1, &read_set, NULL, NULL, NULL);
		}

		if(ctx->wait_connection && ret == 0 )
			continue;

		if( ret <= 0 )
			return ret;

		timeout.tv_sec = 0;

		ret = read(ctx->usb_device, &events, sizeof(events));

		if (ret < 0)
		{
			PRINT_ERROR("handle_ep0 : Read error %d (%m)", errno);
			goto end;
		}

		nevents = ret / sizeof(events[0]);

		PRINT_DEBUG("handle_ep0 : %d event(s)", nevents);

		for (i=0; i<nevents; i++)
		{
			switch (events[i].type)
			{
				case GADGETFS_CONNECT:
					PRINT_DEBUG("handle_ep0 : EP0 CONNECT event");
				break;

				case GADGETFS_DISCONNECT:
					PRINT_DEBUG("handle_ep0 : EP0 DISCONNECT event");

					// Set timeout for a reconnection during the enumeration...
					timeout.tv_sec = 1;
					timeout.tv_usec = 0;

					ctx->stop = 1;
					if( !ctx->thread_not_started )
					{
						pthread_cancel(ctx->thread);
						pthread_join(ctx->thread, NULL);
						ctx->thread_not_started = 1;
					}
				break;

				case GADGETFS_SETUP:
					PRINT_DEBUG("handle_ep0 : EP0 SETUP event");
					handle_setup_request(ctx, &events[i].u.setup);
				break;

				case GADGETFS_NOP:
					PRINT_DEBUG("handle_ep0 : EP0 NOP event");
				break;

				case GADGETFS_SUSPEND:
					PRINT_DEBUG("handle_ep0 : EP0 SUSPEND event");

					if(mtp_context->transferring_file_data)
					{
						// Cancel the ongoing file transfer
						mtp_context->cancel_req = 1;

						cnt = 0;
						while( mtp_context->cancel_req )
						{
							// Wait the end of the current transfer
							if( cnt > 250 )
							{
								// Timeout... Unblock pending usb read/write.
								PRINT_DEBUG("GADGETFS_SUSPEND : Forcing usb file transfer read/write exit...");
								pthread_kill(ctx->thread, SIGUSR1);
								usleep(500);
								break;
							}
							else
								usleep(1000);

							cnt++;
						}
					}
					break;

				default:
					PRINT_DEBUG("handle_ep0 : EP0 unknown event : %d",events[i].type);
				break;
			}
		}
	}

	ctx->stop = 1;

end:
	PRINT_DEBUG("handle_ep0 : Leaving (ctx->stop=%d)...",ctx->stop);

	return 1;
}

// Function FS mode handler
int handle_ffs_ep0(usb_gadget * ctx)
{
	struct timeval timeout;
	int ret, nevents, i;
	fd_set read_set;
	struct usb_functionfs_event event;
	int status;

	PRINT_DEBUG("handle_ffs_ep0 : Entering...");
	timeout.tv_sec = 40;
	timeout.tv_usec = 0;

	while (!ctx->stop)
	{
		FD_ZERO(&read_set);
		FD_SET(ctx->usb_device, &read_set);

		if(timeout.tv_sec)
		{
			ret = select(ctx->usb_device+1, &read_set, NULL, NULL, &timeout);
		}
		else
		{
			PRINT_DEBUG("Select without timeout");
			ret = select(ctx->usb_device+1, &read_set, NULL, NULL, NULL);
		}

		if(ctx->wait_connection && ret == 0 )
			continue;

		if( ret <= 0 )
			return ret;

		timeout.tv_sec = 0;

		ret = read(ctx->usb_device, &event, sizeof(event));

		if (ret < 0)
		{
			PRINT_ERROR("handle_ffs_ep0 : Read error %d (%m)", ret);
			goto end;
		}

		nevents = ret / sizeof(event);

		PRINT_DEBUG("%d event(s)", nevents);

		for (i=0; i<nevents; i++)
		{
			switch (event.type)
			{
			case FUNCTIONFS_ENABLE:
				PRINT_DEBUG("EP0 FFS ENABLE");

				if (ctx->ep_handles[EP_DESCRIPTOR_IN] <= 0)
				{
					status = init_eps(ctx,1);
				}
				else
					status = 0;

				if (!status)
				{
					ctx->stop = 0;
					if( ctx->thread_not_started )
						ctx->thread_not_started = pthread_create(&ctx->thread, NULL, io_thread, ctx);
				}

				break;
			case FUNCTIONFS_DISABLE:
				PRINT_DEBUG("EP0 FFS DISABLE");
				// Set timeout for a reconnection during the enumeration...
				timeout.tv_sec = 0;
				timeout.tv_usec = 0;

				// Stop the main rx thread.
				ctx->stop = 1;
				if( !ctx->thread_not_started )
				{
					pthread_join(ctx->thread, NULL);
					ctx->thread_not_started = 1;
				}
				// But don't close the endpoints !
				ctx->stop = 0;

				// Drop the file system db
				if ( !pthread_mutex_lock( &mtp_context->inotify_mutex ) )
				{
					deinit_fs_db(mtp_context->fs_db);
					mtp_context->fs_db = 0;
					if ( pthread_mutex_unlock( &mtp_context->inotify_mutex ) )
					{
						PRINT_ERROR("handle_ffs_ep0 : Mutex unlock error !");
					}
				}
				else
				{
					PRINT_ERROR("handle_ffs_ep0 : Mutex error !");
				}
				break;
			case FUNCTIONFS_SETUP:
				PRINT_DEBUG("EP0 FFS SETUP");
				handle_setup_request(ctx, &event.u.setup);
				break;
			case FUNCTIONFS_BIND:
				PRINT_DEBUG("EP0 FFS BIND");
				break;
			case FUNCTIONFS_UNBIND:
				PRINT_DEBUG("EP0 FFS UNBIND");
				break;
			case FUNCTIONFS_SUSPEND:
				PRINT_DEBUG("EP0 FFS SUSPEND");
				break;
			case FUNCTIONFS_RESUME:
				PRINT_DEBUG("EP0 FFS RESUME");
				break;
			}
		}
	}

	ctx->stop = 1;

end:
	PRINT_DEBUG("handle_ffs_ep0 : Leaving... (ctx->stop=%d)",ctx->stop);
	return 1;
}

int add_usb_string(usb_gadget * usbctx, int id, char * string)
{
	int i;

	i = 0;

	while( i < MAX_USB_STRING )
	{
		if( !usbctx->stringtab[i].id )
		{
			usbctx->stringtab[i].id = id;
			if(string)
			{
				usbctx->stringtab[i].str = malloc(strlen(string) + 1);
				if(usbctx->stringtab[i].str)
				{
					memset(usbctx->stringtab[i].str,0,strlen(string) + 1);
					strcpy(usbctx->stringtab[i].str,string);
					return i;
				}
				else
				{
					usbctx->stringtab[i].id = 0;
					return -2;
				}
			}
			else
			{
				return i;
			}
		}
		i++;
	}

	return -1;
}

usb_gadget * init_usb_mtp_gadget(mtp_ctx * ctx)
{
	usb_gadget * usbctx;
	int cfg_size;
	int ret,i;
	ffs_strings ffs_str;

	usbctx = NULL;

	if(ctx->wrbuffer)
		free(ctx->wrbuffer);

	ctx->wrbuffer = malloc( ctx->usb_wr_buffer_max_size );
	if(!ctx->wrbuffer)
		goto init_error;

	memset(ctx->wrbuffer,0,ctx->usb_wr_buffer_max_size);

	if(ctx->rdbuffer)
		free(ctx->rdbuffer);

	ctx->rdbuffer = malloc( ctx->usb_rd_buffer_max_size );
	if(!ctx->rdbuffer)
		goto init_error;

	memset(ctx->rdbuffer,0,ctx->usb_rd_buffer_max_size);

	if(ctx->rdbuffer2)
		free(ctx->rdbuffer2);

	ctx->rdbuffer2 = malloc( ctx->usb_rd_buffer_max_size );
	if(!ctx->rdbuffer2)
		goto init_error;

	memset(ctx->rdbuffer2,0,ctx->usb_rd_buffer_max_size);

	usbctx = malloc(sizeof(usb_gadget));
	if(usbctx)
	{
		memset(usbctx,0,sizeof(usb_gadget));

		usbctx->usb_device = -1;
		usbctx->thread_not_started = 1;

		i = 0;
		while( i < EP_NB_OF_DESCRIPTORS )
		{
			usbctx->ep_handles[i] = -1;
			i++;
		}

		add_usb_string(usbctx, STRINGID_MANUFACTURER, ctx->usb_cfg.usb_string_manufacturer);
		add_usb_string(usbctx, STRINGID_PRODUCT,      ctx->usb_cfg.usb_string_product);
		add_usb_string(usbctx, STRINGID_SERIAL,       ctx->usb_cfg.usb_string_serial);
		add_usb_string(usbctx, STRINGID_CONFIG_HS,    "High speed configuration");
		add_usb_string(usbctx, STRINGID_CONFIG_LS,    "Low speed configuration");
		add_usb_string(usbctx, STRINGID_INTERFACE,    ctx->usb_cfg.usb_string_interface);
		add_usb_string(usbctx, STRINGID_MAX,          NULL);

		strings.strings = usbctx->stringtab;

		usbctx->wait_connection = ctx->usb_cfg.wait_connection;

		for(i=0;i<3;i++)
		{
			usbctx->ep_config[i] = malloc(sizeof(ep_cfg));
			if(!usbctx->ep_config[i])
				goto init_error;

			memset(usbctx->ep_config[i],0,sizeof(ep_cfg));
		}

		usbctx->ep_path[0] = ctx->usb_cfg.usb_endpoint_in;
		usbctx->ep_path[1] = ctx->usb_cfg.usb_endpoint_out;
		usbctx->ep_path[2] = ctx->usb_cfg.usb_endpoint_intin;

		usbctx->usb_device = open(ctx->usb_cfg.usb_device_path, O_RDWR|O_SYNC);

		if (usbctx->usb_device < 0)
		{
			PRINT_ERROR("init_usb_mtp_gadget : Unable to open %s (%m)", ctx->usb_cfg.usb_device_path);
			goto init_error;
		}

		cfg_size = sizeof(struct usb_interface_descriptor) + (sizeof(struct usb_endpoint_descriptor_no_audio) * 3);

		if( ctx->usb_cfg.usb_functionfs_mode )
		{
			// FunctionFS mode

			usbctx->usb_ffs_config = malloc(sizeof(usb_ffs_cfg));
			if(!usbctx->usb_ffs_config)
				goto init_error;

			memset(usbctx->usb_ffs_config,0,sizeof(usb_ffs_cfg));

#ifdef OLD_FUNCTIONFS_DESCRIPTORS // Kernel < v3.15
			usbctx->usb_ffs_config->magic = htole32(FUNCTIONFS_DESCRIPTORS_MAGIC);
#else
			usbctx->usb_ffs_config->magic = htole32(FUNCTIONFS_DESCRIPTORS_MAGIC_V2);
			usbctx->usb_ffs_config->flags = htole32(0);

#ifdef CONFIG_USB_FS_SUPPORT
			usbctx->usb_ffs_config->flags |= htole32(FUNCTIONFS_HAS_FS_DESC);
#endif

#ifdef CONFIG_USB_HS_SUPPORT
			usbctx->usb_ffs_config->flags |= htole32(FUNCTIONFS_HAS_HS_DESC);
#endif

#ifdef CONFIG_USB_SS_SUPPORT
			usbctx->usb_ffs_config->flags |= htole32(FUNCTIONFS_HAS_SS_DESC);
#endif

#endif
			usbctx->usb_ffs_config->length = htole32(sizeof(usb_ffs_cfg));

#ifdef CONFIG_USB_FS_SUPPORT
			usbctx->usb_ffs_config->fs_count = htole32(1 + 3);

			fill_if_descriptor(ctx, usbctx, &usbctx->usb_ffs_config->ep_desc_fs.if_desc);
			fill_ep_descriptor(ctx, usbctx, &usbctx->usb_ffs_config->ep_desc_fs.ep_desc_in,1, EP_BULK_MODE | EP_IN_DIR);
			fill_ep_descriptor(ctx, usbctx, &usbctx->usb_ffs_config->ep_desc_fs.ep_desc_out,2, EP_BULK_MODE | EP_OUT_DIR);
			fill_ep_descriptor(ctx, usbctx, &usbctx->usb_ffs_config->ep_desc_fs.ep_desc_int_in,3, EP_INT_MODE | EP_IN_DIR);
#endif

#ifdef CONFIG_USB_HS_SUPPORT
			usbctx->usb_ffs_config->hs_count = htole32(1 + 3);
			fill_if_descriptor(ctx, usbctx, &usbctx->usb_ffs_config->ep_desc_hs.if_desc);
			fill_ep_descriptor(ctx, usbctx, &usbctx->usb_ffs_config->ep_desc_hs.ep_desc_in,1, EP_BULK_MODE | EP_IN_DIR | EP_HS_MODE);
			fill_ep_descriptor(ctx, usbctx, &usbctx->usb_ffs_config->ep_desc_hs.ep_desc_out,2, EP_BULK_MODE | EP_OUT_DIR | EP_HS_MODE);
			fill_ep_descriptor(ctx, usbctx, &usbctx->usb_ffs_config->ep_desc_hs.ep_desc_int_in,3, EP_INT_MODE | EP_IN_DIR | EP_HS_MODE);
#endif

#ifdef CONFIG_USB_SS_SUPPORT
			usbctx->usb_ffs_config->ss_count = htole32(1 + (3*2));
			fill_if_descriptor(ctx, usbctx, &usbctx->usb_ffs_config->ep_desc_ss.if_desc);
			fill_ep_descriptor(ctx, usbctx, &usbctx->usb_ffs_config->ep_desc_ss.ep_desc_in,1, EP_BULK_MODE | EP_IN_DIR | EP_SS_MODE);
			fill_ep_descriptor(ctx, usbctx, &usbctx->usb_ffs_config->ep_desc_ss.ep_desc_out,2, EP_BULK_MODE | EP_OUT_DIR | EP_SS_MODE);
			fill_ep_descriptor(ctx, usbctx, &usbctx->usb_ffs_config->ep_desc_ss.ep_desc_int_in,3, EP_INT_MODE | EP_IN_DIR | EP_SS_MODE);
#endif

			PRINT_DEBUG("init_usb_mtp_gadget :");
			PRINT_DEBUG_BUF(usbctx->usb_ffs_config, sizeof(usb_ffs_cfg));

			ret = write(usbctx->usb_device, usbctx->usb_ffs_config, sizeof(usb_ffs_cfg));

			if(ret != sizeof(usb_ffs_cfg))
			{
				PRINT_ERROR("FunctionFS USB Config write error (%d != %zu)",ret,sizeof(usb_ffs_cfg));
				goto init_error;
			}

			memset( &ffs_str, 0, sizeof(ffs_strings));
			ffs_str.header.magic = htole32(FUNCTIONFS_STRINGS_MAGIC);
			ffs_str.header.length = htole32(sizeof(struct usb_functionfs_strings_head) + sizeof(uint16_t) + strlen(ctx->usb_cfg.usb_string_interface) + 1);
			ffs_str.header.str_count = htole32(1);
			ffs_str.header.lang_count = htole32(1);
			ffs_str.code = htole16(0x0409); // en-us
			strcpy(ffs_str.string_data,ctx->usb_cfg.usb_string_interface);

			PRINT_DEBUG("write string :");
			PRINT_DEBUG_BUF(&ffs_str, sizeof(ffs_strings));

			ret = write(usbctx->usb_device, &ffs_str, ffs_str.header.length);

			if( ret != ffs_str.header.length )
			{
				PRINT_ERROR("FunctionFS String Config write error (%d != %zu)",ret,(size_t)ffs_str.header.length);
				goto init_error;
			}
		}
		else
		{
			usbctx->usb_config = malloc(sizeof(usb_cfg));
			if(!usbctx->usb_config)
				goto init_error;

			memset(usbctx->usb_config,0,sizeof(usb_cfg));

			usbctx->usb_config->head = 0x00000000;

#ifdef CONFIG_USB_FS_SUPPORT
			fill_config_descriptor(ctx, usbctx, &usbctx->usb_config->cfg_fs, cfg_size, 0);
			fill_if_descriptor(ctx, usbctx, &usbctx->usb_config->ep_desc_fs.if_desc);
			fill_ep_descriptor(ctx, usbctx, &usbctx->usb_config->ep_desc_fs.ep_desc_in,1, EP_BULK_MODE | EP_IN_DIR);
			fill_ep_descriptor(ctx, usbctx, &usbctx->usb_config->ep_desc_fs.ep_desc_out,2, EP_BULK_MODE | EP_OUT_DIR);
			fill_ep_descriptor(ctx, usbctx, &usbctx->usb_config->ep_desc_fs.ep_desc_int_in,3, EP_INT_MODE | EP_IN_DIR);
#endif

#ifdef CONFIG_USB_HS_SUPPORT
			fill_config_descriptor(ctx, usbctx, &usbctx->usb_config->cfg_hs, cfg_size, 1);
			fill_if_descriptor(ctx, usbctx, &usbctx->usb_config->ep_desc_hs.if_desc);
			fill_ep_descriptor(ctx, usbctx, &usbctx->usb_config->ep_desc_hs.ep_desc_in,1, EP_BULK_MODE | EP_IN_DIR | EP_HS_MODE);
			fill_ep_descriptor(ctx, usbctx, &usbctx->usb_config->ep_desc_hs.ep_desc_out,2, EP_BULK_MODE | EP_OUT_DIR | EP_HS_MODE);
			fill_ep_descriptor(ctx, usbctx, &usbctx->usb_config->ep_desc_hs.ep_desc_int_in,3, EP_INT_MODE | EP_IN_DIR | EP_HS_MODE);
#endif

#ifdef CONFIG_USB_SS_SUPPORT
			fill_config_descriptor(ctx, usbctx, &usbctx->usb_config->cfg_ss, cfg_size, 1);
			fill_if_descriptor(ctx, usbctx, &usbctx->usb_config->ep_desc_ss.if_desc);
			fill_ep_descriptor(ctx, usbctx, &usbctx->usb_config->ep_desc_ss.ep_desc_in,1, EP_BULK_MODE | EP_IN_DIR | EP_SS_MODE);
			fill_ep_descriptor(ctx, usbctx, &usbctx->usb_config->ep_desc_ss.ep_desc_out,2, EP_BULK_MODE | EP_OUT_DIR | EP_SS_MODE);
			fill_ep_descriptor(ctx, usbctx, &usbctx->usb_config->ep_desc_ss.ep_desc_int_in,3, EP_INT_MODE | EP_IN_DIR | EP_SS_MODE);
#endif

			fill_dev_descriptor(ctx, usbctx,&usbctx->usb_config->dev_desc);

			PRINT_DEBUG("init_usb_mtp_gadget :");
			PRINT_DEBUG_BUF(usbctx->usb_config, sizeof(usb_cfg));

			ret = write(usbctx->usb_device, usbctx->usb_config, sizeof(usb_cfg));

			if(ret != sizeof(usb_cfg))
			{
				PRINT_ERROR("GadgetFS USB Config write error (%d != %zu)",ret,sizeof(usb_cfg));
				goto init_error;
			}
		}

		PRINT_DEBUG("init_usb_mtp_gadget : USB config done");

		return usbctx;
	}

init_error:
	PRINT_ERROR("init_usb_mtp_gadget init error !");

	deinit_usb_mtp_gadget(usbctx);

	return NULL;
}

void deinit_usb_mtp_gadget(usb_gadget * usbctx)
{
	int i;

	PRINT_DEBUG("entering deinit_usb_mtp_gadget");

	if( usbctx )
	{
		usbctx->stop = 1;

		i = 0;
		while( i < EP_NB_OF_DESCRIPTORS )
		{

			if( usbctx->ep_handles[i] >= 0 )
			{
				PRINT_DEBUG("Closing End Point %d...",i);
				close(usbctx->ep_handles[i] );
			}
			i++;
		}

		if (usbctx->usb_device >= 0)
		{
			PRINT_DEBUG("Closing usb device...");
			close(usbctx->usb_device);
			usbctx->usb_device = - 1;
		}

		if( !usbctx->thread_not_started )
		{
			PRINT_DEBUG("Stopping USB Thread...");
			pthread_cancel (usbctx->thread);
			pthread_join(usbctx->thread, NULL);
			usbctx->thread_not_started = 1;
		}

		if(usbctx->usb_config)
		{
			free(usbctx->usb_config);
			usbctx->usb_config = 0;
		}

		if(usbctx->usb_ffs_config)
		{
			free(usbctx->usb_ffs_config);
			usbctx->usb_ffs_config = 0;
		}

		for(i=0;i<3;i++)
		{
			if( usbctx->ep_config[i] )
				free( usbctx->ep_config[i] );
		}

		i = 0;
		while( i < MAX_USB_STRING )
		{
			if( usbctx->stringtab[i].str )
			{
				free ( usbctx->stringtab[i].str );
			}
			i++;
		}

		free( usbctx );
	}

	if(mtp_context->wrbuffer)
	{
		free(mtp_context->wrbuffer);
		mtp_context->wrbuffer = NULL;
	}

	if(mtp_context->rdbuffer)
	{
		free(mtp_context->rdbuffer);
		mtp_context->rdbuffer = NULL;
	}

	if(mtp_context->rdbuffer2)
	{
		free(mtp_context->rdbuffer2);
		mtp_context->rdbuffer2 = NULL;
	}

	PRINT_DEBUG("leaving deinit_usb_mtp_gadget");
}

