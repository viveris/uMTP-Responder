/*
 * uMTP Responder
 * Copyright (c) 2018 - 2019 Viveris Technologies
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
 * @file   mtp.h
 * @brief  Main MTP protocol functions.
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#ifndef _INC_MTP_H_
#define _INC_MTP_H_

#define MAX_STORAGE_NB 16
#define MAX_CFG_STRING_SIZE 512

#pragma pack(1)

typedef struct _MTP_PACKET_HEADER
{
	uint32_t length;
	uint16_t operation;
	uint16_t code;
	uint32_t tx_id;
}MTP_PACKET_HEADER;

#pragma pack()

#include "fs_handles_db.h"

typedef struct mtp_usb_cfg_
{
	uint16_t usb_vendor_id;
	uint16_t usb_product_id;
	uint8_t  usb_class;
	uint8_t  usb_subclass;
	uint8_t  usb_protocol;
	uint16_t usb_dev_version;
	uint16_t usb_max_packet_size;
	uint8_t  usb_functionfs_mode;

	char usb_device_path[MAX_CFG_STRING_SIZE];
	char usb_endpoint_in[MAX_CFG_STRING_SIZE];
	char usb_endpoint_out[MAX_CFG_STRING_SIZE];
	char usb_endpoint_intin[MAX_CFG_STRING_SIZE];

	char usb_string_manufacturer[MAX_CFG_STRING_SIZE];
	char usb_string_product[MAX_CFG_STRING_SIZE];
	char usb_string_serial[MAX_CFG_STRING_SIZE];

	char usb_string_interface[MAX_CFG_STRING_SIZE];

	int wait_connection;
	int loop_on_disconnect;

	int show_hidden_files;

}mtp_usb_cfg;

typedef struct mtp_storage_
{
	char * root_path;
	char * description;
	uint32_t storage_id;
}mtp_storage;

typedef struct mtp_ctx_
{
	uint32_t session_id;

	mtp_usb_cfg usb_cfg;

	void * usb_ctx;

	unsigned char * wrbuffer;
	unsigned char * rdbuffer;

	unsigned char * rdbuffer2;

	uint32_t *temp_array;

	fs_handles_db * fs_db;

	uint32_t SendObjInfoHandle;
	uint32_t SendObjInfoSize;

	uint32_t max_packet_size;

	mtp_storage storages[MAX_STORAGE_NB];

	int inotify_fd;
	pthread_t inotify_thread;
	pthread_mutex_t inotify_mutex;

	int no_inotify;

}mtp_ctx;

mtp_ctx * mtp_init_responder();

int  mtp_incoming_packet(mtp_ctx * ctx);
void mtp_set_usb_handle(mtp_ctx * ctx, void * handle, uint32_t max_packet_size);

int mtp_load_config_file(mtp_ctx * context);

uint32_t mtp_add_storage(mtp_ctx * ctx, char * path, char * description);
char * mtp_get_storage_description(mtp_ctx * ctx, uint32_t storage_id);
char * mtp_get_storage_root(mtp_ctx * ctx, uint32_t storage_id);

int mtp_push_event(mtp_ctx * ctx, uint32_t event, int nbparams, uint32_t * parameters );

void mtp_deinit_responder(mtp_ctx * ctx);

#define APP_VERSION "v0.9.7"

#endif
