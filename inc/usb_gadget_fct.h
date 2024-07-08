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
 * @file   usb_gadget_fct.h
 * @brief  USB gadget / FunctionFS layer - Public functions declarations.
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#ifndef _INC_USB_GADGET_FCT_H_
#define _INC_USB_GADGET_FCT_H_

#include "usb_gadget.h"

usb_gadget * init_usb_mtp_gadget(mtp_ctx * ctx);

int read_usb(usb_gadget * ctx, unsigned char * buffer, int maxsize);
int write_usb(usb_gadget * ctx, int channel, unsigned char * buffer, int size);

int handle_ep0(usb_gadget * ctx);
int handle_ffs_ep0(usb_gadget * ctx);
int is_usb_up(usb_gadget * ctx);

void deinit_usb_mtp_gadget(usb_gadget * usbctx);
#endif
