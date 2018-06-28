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
 * @brief  USB gadget layer
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#ifndef _INC_USB_GADGET_H_
#define _INC_USB_GADGET_H_

#include <linux/usb/ch9.h>
#include <linux/usb/gadgetfs.h>
#include <pthread.h>
#include "usbstring.h"

enum
{
	EP_DESCRIPTOR_IN = 0,
	EP_DESCRIPTOR_OUT,
	EP_DESCRIPTOR_INT_IN,

	EP_NB_OF_DESCRIPTORS
};

/* USB_DT_ENDPOINT: Endpoint descriptor */
// TODO : Find a better solution than redefine this structure to remove the audio related members ...
struct usb_endpoint_descriptor_noaudio {
	uint8_t  bLength;
	uint8_t  bDescriptorType;

	uint8_t  bEndpointAddress;
	uint8_t  bmAttributes;
	uint16_t wMaxPacketSize;
	uint8_t  bInterval;

} __attribute__ ((packed));


typedef struct _usb_cfg
{
	uint32_t head;

	struct usb_config_descriptor cfg;
	struct usb_interface_descriptor if_desc;
	struct usb_endpoint_descriptor_noaudio ep_desc[3];

	struct usb_config_descriptor cfg_hs;
	struct usb_interface_descriptor if_desc_hs;
	struct usb_endpoint_descriptor_noaudio ep_desc_hs[3];

	struct usb_device_descriptor dev_desc;

} __attribute__ ((packed)) usb_cfg;


typedef struct _ep_cfg
{
	uint32_t head;

	struct usb_endpoint_descriptor_noaudio ep_desc[2];

} __attribute__ ((packed)) ep_cfg;

enum {
	STRINGID_MANUFACTURER = 1,
	STRINGID_PRODUCT,
	STRINGID_SERIAL,
	STRINGID_CONFIG_HS,
	STRINGID_CONFIG_LS,
	STRINGID_INTERFACE,
	STRINGID_MAX
};

#define MAX_USB_STRING 16

typedef struct _usb_gadget
{
	int usb_device;

	usb_cfg * usb_config;
	ep_cfg *  ep_config[3];

	int ep_handles[EP_NB_OF_DESCRIPTORS];

	char * ep_path[3];

	int stop;

	struct usb_string stringtab[MAX_USB_STRING];

	int wait_connection;
	pthread_t thread;
	int thread_started;

}usb_gadget;

#endif
