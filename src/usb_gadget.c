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
 * @file   usb_gadget.c
 * @brief  USB gadget layer - Main inspiration from Grégory Soutadé (http://blog.soutade.fr/post/2016/07/create-your-own-usb-gadget-with-gadgetfs.html)
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>

#include <linux/types.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadgetfs.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

#include <errno.h>

#include "fs_handles_db.h"
#include "mtp.h"

#include "usbstring.h"
#include "usb_gadget.h"

#include "logs_out.h"

#define CONFIG_VALUE 1

static struct usb_gadget_strings strings = {
	.language = 0x0409, /* en-us */
	.strings = 0,
};

extern void* io_thread(void* arg);

int read_usb(usb_gadget * ctx, unsigned char * buffer, int maxsize)
{
	int ret;
	int max_read_fd;
	fd_set read_set;

	max_read_fd = 0;

	if (ctx->ep_handles[EP_DESCRIPTOR_OUT] > max_read_fd)
		max_read_fd = ctx->ep_handles[EP_DESCRIPTOR_OUT];

	FD_ZERO(&read_set);
	FD_SET(ctx->ep_handles[EP_DESCRIPTOR_OUT], &read_set);

	ret = select(max_read_fd+1, &read_set, NULL, NULL, NULL);

	// Error
	if (ret < 0)
		return ret;

	ret = read (ctx->ep_handles[EP_DESCRIPTOR_OUT], buffer, maxsize);

	return ret;
}

int write_usb(usb_gadget * ctx, unsigned char * buffer, int size)
{
	fd_set write_set;
	int ret, max_write_fd;

	max_write_fd = 0;

	if (ctx->ep_handles[EP_DESCRIPTOR_IN] > max_write_fd)
		max_write_fd = ctx->ep_handles[EP_DESCRIPTOR_IN];

	FD_ZERO(&write_set);
	FD_SET(ctx->ep_handles[EP_DESCRIPTOR_IN], &write_set);

	ret = select(max_write_fd+1, NULL, &write_set, NULL, NULL);

	// Error
	if (ret < 0)
		return ret;

	PRINT_DEBUG("Send %d bytes :\n", size);
	PRINT_DEBUG_BUF(buffer,size);
	ret = write (ctx->ep_handles[EP_DESCRIPTOR_IN], buffer, size);
	PRINT_DEBUG("Write status %d (%m)", ret);

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

	PRINT_DEBUG("fill_config_descriptor: (Total Len : %lu + %d = %d)",sizeof(struct usb_config_descriptor) ,total_size,desc->wTotalLength);
	PRINT_DEBUG_BUF(desc, sizeof(struct usb_config_descriptor));

	return;
}

void fill_dev_descriptor(mtp_ctx * ctx, usb_gadget * usbctx,struct usb_device_descriptor * desc)
{
	memset(desc,0,sizeof(struct usb_device_descriptor));

	desc->bLength = USB_DT_DEVICE_SIZE;
	desc->bDescriptorType = USB_DT_DEVICE;
	desc->bDeviceClass =	ctx->usb_cfg.usb_class;
	desc->bDeviceSubClass = ctx->usb_cfg.usb_subclass;
	desc->bDeviceProtocol = ctx->usb_cfg.usb_protocol;
	desc->idVendor =        ctx->usb_cfg.usb_vendor_id;
	desc->idProduct =       ctx->usb_cfg.usb_product_id;
	desc->bcdDevice =       ctx->usb_cfg.usb_dev_version; // Version
	// Strings
	desc->iManufacturer =   STRINGID_MANUFACTURER;
	desc->iProduct =	    STRINGID_PRODUCT;
	desc->iSerialNumber =   STRINGID_SERIAL;
	desc->bNumConfigurations = 1; // Only one configuration

	PRINT_DEBUG("fill_dev_descriptor:\n");
	PRINT_DEBUG_BUF(desc, sizeof(struct usb_device_descriptor));

	return;
}

void fill_if_descriptor(mtp_ctx * ctx, usb_gadget * usbctx, struct usb_interface_descriptor * desc)
{
	memset(desc,0,sizeof(struct usb_interface_descriptor));

	desc->bLength = sizeof(struct usb_interface_descriptor);
	desc->bDescriptorType = USB_DT_INTERFACE;
	desc->bInterfaceNumber = 0;
	desc->bAlternateSetting = 0;
	desc->bNumEndpoints = 3;

	desc->bInterfaceClass =    ctx->usb_cfg.usb_class;
	desc->bInterfaceSubClass = ctx->usb_cfg.usb_subclass;
	desc->bInterfaceProtocol = ctx->usb_cfg.usb_protocol;
	desc->iInterface =         STRINGID_INTERFACE;

	PRINT_DEBUG("fill_if_descriptor:\n");
	PRINT_DEBUG_BUF(desc, sizeof(struct usb_interface_descriptor));

	return;
}


void fill_ep_descriptor(mtp_ctx * ctx, usb_gadget * usbctx,struct usb_endpoint_descriptor_noaudio * desc,int index,int bulk,int dir, int hs)
{
	memset(desc,0,sizeof(struct usb_endpoint_descriptor_noaudio));

	desc->bLength = USB_DT_ENDPOINT_SIZE;
	desc->bDescriptorType = USB_DT_ENDPOINT;
	if(!dir)
		desc->bEndpointAddress = USB_DIR_IN  | (index);
	else
		desc->bEndpointAddress = USB_DIR_OUT | (index);

	if(bulk)
	{
		desc->bmAttributes = USB_ENDPOINT_XFER_BULK;
		desc->wMaxPacketSize = ctx->usb_cfg.usb_max_packet_size;
	}
	else
	{
		desc->bmAttributes =  USB_ENDPOINT_XFER_INT;
		desc->wMaxPacketSize = 28; // HS size
		desc->bInterval = 6;
	}

	PRINT_DEBUG("fill_ep_descriptor:\n");
	PRINT_DEBUG_BUF(desc, sizeof(struct usb_endpoint_descriptor_noaudio));

	return;
}

int init_ep(usb_gadget * ctx,int index)
{
	int fd,ret;

	PRINT_DEBUG("Init end point %s (%d)",ctx->ep_path[index],index);
	fd = open(ctx->ep_path[index], O_RDWR);
	if ( fd <= 0 )
	{
		PRINT_ERROR("Can't open endpoint %s (error %d)",ctx->ep_path[index],fd);
		goto init_ep_error;
	}

	ctx->ep_handles[index] = fd;

	ctx->ep_config[index]->head = 1;

	memcpy(&ctx->ep_config[index]->ep_desc[0], &ctx->usb_config->ep_desc[index],sizeof(struct usb_endpoint_descriptor_noaudio));
	memcpy(&ctx->ep_config[index]->ep_desc[1], &ctx->usb_config->ep_desc[index],sizeof(struct usb_endpoint_descriptor_noaudio));

	PRINT_DEBUG("init_ep (%d):\n",index);
	PRINT_DEBUG_BUF(ctx->ep_config[index], sizeof(ep_cfg));

	ret = write(fd, ctx->ep_config[index], sizeof(ep_cfg));

	if (ret != sizeof(ep_cfg))
	{
		PRINT_DEBUG("Write error %d (%m)", ret);
		goto init_ep_error;
	}

	return fd;

init_ep_error:
	return 0;
}

int init_eps(usb_gadget * ctx)
{
	if( !init_ep(ctx, EP_DESCRIPTOR_IN) )
		goto init_eps_error;

	if( !init_ep(ctx, EP_DESCRIPTOR_OUT) )
		goto init_eps_error;

	if( !init_ep(ctx, EP_DESCRIPTOR_INT_IN) )
		goto init_eps_error;

	return 0;

init_eps_error:
	return 1;
}

static void handle_setup_request(usb_gadget * ctx, struct usb_ctrlrequest* setup)
{
	int status;
	uint8_t buffer[512];

	PRINT_DEBUG("Setup request %d", setup->bRequest);

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
					PRINT_DEBUG("String not found !!");
					break;
				}
				else
				{
					PRINT_DEBUG("Found %d bytes", status);
					PRINT_DEBUG_BUF(buffer, status);
				}
				write (ctx->usb_device, buffer, status);
				return;
		default:
			PRINT_DEBUG("Cannot return descriptor %d", (setup->wValue >> 8));
		}
		break;
	case USB_REQ_SET_CONFIGURATION:
		if (setup->bRequestType != USB_DIR_OUT)
		{
			PRINT_DEBUG("Bad dir");
			goto stall;
		}
		switch (setup->wValue) {
		case CONFIG_VALUE:
			PRINT_DEBUG("Set config value");
			if (!ctx->stop)
			{
				ctx->stop = 1;
				usleep(200000); // Wait for termination
			}
			if (ctx->ep_handles[EP_DESCRIPTOR_IN] <= 0)
			{
				status = init_eps(ctx);
			}
			else
				status = 0;
			if (!status)
			{
				ctx->stop = 0;
				pthread_create(&ctx->thread, NULL, io_thread, ctx);
				ctx->thread_started = 1;
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
	case USB_REQ_GET_INTERFACE:
		PRINT_DEBUG("GET_INTERFACE");
		buffer[0] = 0;
		write (ctx->usb_device, buffer, 1);
		return;
	case USB_REQ_SET_INTERFACE:
		PRINT_DEBUG("SET_INTERFACE");
		ioctl (ctx->ep_handles[EP_DESCRIPTOR_IN], GADGETFS_CLEAR_HALT);
		ioctl (ctx->ep_handles[EP_DESCRIPTOR_OUT], GADGETFS_CLEAR_HALT);
		ioctl (ctx->ep_handles[EP_DESCRIPTOR_INT_IN], GADGETFS_CLEAR_HALT);
		// ACK
		status = read (ctx->usb_device, &status, 0);
		return;
	}

stall:
	PRINT_DEBUG("Stalled");
	// Error
	if (setup->bRequestType & USB_DIR_IN)
		read (ctx->usb_device, &status, 0);
	else
		write (ctx->usb_device, &status, 0);
}

int handle_ep0(usb_gadget * ctx)
{
	struct timeval timeout;
	int ret, nevents, i;
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
			PRINT_DEBUG("Select without timeout");
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
			PRINT_DEBUG("Read error %d (%m)", ret);
			goto end;
		}

		nevents = ret / sizeof(events[0]);

		PRINT_DEBUG("%d event(s)", nevents);

		for (i=0; i<nevents; i++)
		{
			switch (events[i].type)
			{
			case GADGETFS_CONNECT:
				PRINT_DEBUG("EP0 CONNECT");
				break;
			case GADGETFS_DISCONNECT:
				PRINT_DEBUG("EP0 DISCONNECT");
				// Set timeout for a reconnection during the enumeration...
				timeout.tv_sec = 1;
				timeout.tv_usec = 0;
				break;
			case GADGETFS_SETUP:
				PRINT_DEBUG("EP0 SETUP");
				handle_setup_request(ctx, &events[i].u.setup);
				break;
			case GADGETFS_NOP:
			case GADGETFS_SUSPEND:
				break;
			}
		}
	}
	if(ctx->thread_started)
	{
		pthread_join(ctx->thread, NULL);
	}

end:
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

	usbctx = malloc(sizeof(usb_gadget));
	if(usbctx)
	{
		memset(usbctx,0,sizeof(usb_gadget));
		usbctx->usb_device = -1;

		add_usb_string(usbctx, STRINGID_MANUFACTURER, ctx->usb_cfg.usb_string_manufacturer);
		add_usb_string(usbctx, STRINGID_PRODUCT,      ctx->usb_cfg.usb_string_product);
		add_usb_string(usbctx, STRINGID_SERIAL,       ctx->usb_cfg.usb_string_serial);
		add_usb_string(usbctx, STRINGID_CONFIG_HS,    "High speed configuration");
		add_usb_string(usbctx, STRINGID_CONFIG_LS,    "Low speed configuration");
		add_usb_string(usbctx, STRINGID_INTERFACE,    ctx->usb_cfg.usb_string_interface);
		add_usb_string(usbctx, STRINGID_MAX,          NULL);

		strings.strings = usbctx->stringtab;

		usbctx->wait_connection = ctx->usb_cfg.wait_connection;

		usbctx->usb_config = malloc(sizeof(usb_cfg));
		if(!usbctx->usb_config)
			goto init_error;

		memset(usbctx->usb_config,0,sizeof(usb_cfg));

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

		if (usbctx->usb_device <= 0)
		{
			PRINT_ERROR("Unable to open %s (%m)", ctx->usb_cfg.usb_device_path);
			goto init_error;
		}

		cfg_size = sizeof(struct usb_interface_descriptor) + (sizeof(struct usb_endpoint_descriptor_noaudio) * 3);

		usbctx->usb_config->head = 0x00000000;
		fill_config_descriptor(ctx, usbctx, &usbctx->usb_config->cfg, cfg_size, 0);
		fill_if_descriptor(ctx, usbctx, &usbctx->usb_config->if_desc);
		fill_ep_descriptor(ctx, usbctx, &usbctx->usb_config->ep_desc[EP_DESCRIPTOR_IN],EP_DESCRIPTOR_IN+1,1,0,0);
		fill_ep_descriptor(ctx, usbctx, &usbctx->usb_config->ep_desc[EP_DESCRIPTOR_OUT],EP_DESCRIPTOR_OUT+1,1,1,0);
		fill_ep_descriptor(ctx, usbctx, &usbctx->usb_config->ep_desc[EP_DESCRIPTOR_INT_IN],EP_DESCRIPTOR_INT_IN+1,0,0,0);

		fill_config_descriptor(ctx, usbctx, &usbctx->usb_config->cfg_hs, cfg_size, 1);
		fill_if_descriptor(ctx, usbctx, &usbctx->usb_config->if_desc_hs);
		fill_ep_descriptor(ctx, usbctx, &usbctx->usb_config->ep_desc_hs[EP_DESCRIPTOR_IN],EP_DESCRIPTOR_IN+1,1,0,1);
		fill_ep_descriptor(ctx, usbctx, &usbctx->usb_config->ep_desc_hs[EP_DESCRIPTOR_OUT],EP_DESCRIPTOR_OUT+1,1,1,1);
		fill_ep_descriptor(ctx, usbctx, &usbctx->usb_config->ep_desc_hs[EP_DESCRIPTOR_INT_IN],EP_DESCRIPTOR_INT_IN+1,0,0,1);

		fill_dev_descriptor(ctx, usbctx,&usbctx->usb_config->dev_desc);

		PRINT_DEBUG("init_usb_mtp_gadget :\n");
		PRINT_DEBUG_BUF(usbctx->usb_config, sizeof(usb_cfg));

		ret = write(usbctx->usb_device, usbctx->usb_config, sizeof(usb_cfg));

		if(ret != sizeof(usb_cfg))
		{
			PRINT_ERROR("USB Config write error (%d != %zu)",ret,sizeof(usb_cfg));
			goto init_error;
		}

		PRINT_MSG("init_usb_mtp_gadget : USB config done");

		return usbctx;
	}

init_error:
	PRINT_ERROR("init_usb_mtp_gadget init error !");

	if(usbctx->usb_config)
		free(usbctx->usb_config);

	if (usbctx->usb_device >= 0)
		close(usbctx->usb_device);

	return 0;
}

void deinit_usb_mtp_gadget(usb_gadget * usbctx)
{
	if(usbctx->usb_config)
		free(usbctx->usb_config);

	if (usbctx->usb_device >= 0)
		close(usbctx->usb_device);
}
