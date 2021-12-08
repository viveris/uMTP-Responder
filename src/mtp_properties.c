/*
 * uMTP Responder
 * Copyright (c) 2018 - 2021 Viveris Technologies
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
 * @file   mtp_properties.c
 * @brief  MTP properties datasets helpers
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include "buildconf.h"

#include <inttypes.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#include "mtp.h"
#include "mtp_helpers.h"
#include "mtp_constant.h"
#include "mtp_constant_strings.h"
#include "mtp_properties.h"

#include "fs_handles_db.h"
#include "usb_gadget_fct.h"

#include "logs_out.h"

formats_property fmt_properties[]=
{   // prop_code                       data_type         getset    default value          group code
	{ MTP_FORMAT_UNDEFINED    , (uint16_t[]){   MTP_PROPERTY_STORAGE_ID, MTP_PROPERTY_OBJECT_FORMAT, MTP_PROPERTY_PROTECTION_STATUS, MTP_PROPERTY_OBJECT_SIZE,
												MTP_PROPERTY_OBJECT_FILE_NAME, MTP_PROPERTY_DATE_MODIFIED, MTP_PROPERTY_PARENT_OBJECT, MTP_PROPERTY_PERSISTENT_UID,
												MTP_PROPERTY_NAME, MTP_PROPERTY_DISPLAY_NAME, MTP_PROPERTY_DATE_CREATED,
												0xFFFF}
	},
	{ MTP_FORMAT_ASSOCIATION  , (uint16_t[]){   MTP_PROPERTY_STORAGE_ID, MTP_PROPERTY_OBJECT_FORMAT, MTP_PROPERTY_PROTECTION_STATUS, MTP_PROPERTY_OBJECT_SIZE,
												MTP_PROPERTY_OBJECT_FILE_NAME, MTP_PROPERTY_DATE_MODIFIED, MTP_PROPERTY_PARENT_OBJECT, MTP_PROPERTY_PERSISTENT_UID,
												MTP_PROPERTY_NAME, MTP_PROPERTY_DISPLAY_NAME, MTP_PROPERTY_DATE_CREATED,
												0xFFFF}
	}
#if 0
	{ MTP_FORMAT_TEXT         , (uint16_t[]){   MTP_PROPERTY_STORAGE_ID, MTP_PROPERTY_OBJECT_FORMAT, MTP_PROPERTY_PROTECTION_STATUS, MTP_PROPERTY_OBJECT_SIZE,
												MTP_PROPERTY_OBJECT_FILE_NAME, MTP_PROPERTY_DATE_MODIFIED, MTP_PROPERTY_PARENT_OBJECT, MTP_PROPERTY_PERSISTENT_UID,
												MTP_PROPERTY_NAME, MTP_PROPERTY_DISPLAY_NAME, MTP_PROPERTY_DATE_CREATED,
												0xFFFF}
	},
	{ MTP_FORMAT_HTML         , (uint16_t[]){   MTP_PROPERTY_STORAGE_ID, MTP_PROPERTY_OBJECT_FORMAT, MTP_PROPERTY_PROTECTION_STATUS, MTP_PROPERTY_OBJECT_SIZE,
												MTP_PROPERTY_OBJECT_FILE_NAME, MTP_PROPERTY_DATE_MODIFIED, MTP_PROPERTY_PARENT_OBJECT, MTP_PROPERTY_PERSISTENT_UID,
												MTP_PROPERTY_NAME, MTP_PROPERTY_DISPLAY_NAME, MTP_PROPERTY_DATE_CREATED,
												0xFFFF}
	},
	{ MTP_FORMAT_MP4_CONTAINER, (uint16_t[]){   MTP_PROPERTY_STORAGE_ID, MTP_PROPERTY_OBJECT_FORMAT, MTP_PROPERTY_PROTECTION_STATUS, MTP_PROPERTY_OBJECT_SIZE,
												MTP_PROPERTY_OBJECT_FILE_NAME, MTP_PROPERTY_DATE_MODIFIED, MTP_PROPERTY_PARENT_OBJECT, MTP_PROPERTY_PERSISTENT_UID,
												MTP_PROPERTY_NAME, MTP_PROPERTY_DISPLAY_NAME, MTP_PROPERTY_DATE_CREATED, MTP_PROPERTY_ARTIST, MTP_PROPERTY_ALBUM_NAME,
												MTP_PROPERTY_DURATION, MTP_PROPERTY_DESCRIPTION, MTP_PROPERTY_WIDTH, MTP_PROPERTY_HEIGHT, MTP_PROPERTY_DATE_AUTHORED,
												0xFFFF}
	},
	{ MTP_FORMAT_3GP_CONTAINER, (uint16_t[]){   MTP_PROPERTY_STORAGE_ID, MTP_PROPERTY_OBJECT_FORMAT, MTP_PROPERTY_PROTECTION_STATUS, MTP_PROPERTY_OBJECT_SIZE,
												MTP_PROPERTY_OBJECT_FILE_NAME, MTP_PROPERTY_DATE_MODIFIED, MTP_PROPERTY_PARENT_OBJECT, MTP_PROPERTY_PERSISTENT_UID,
												MTP_PROPERTY_NAME, MTP_PROPERTY_DISPLAY_NAME, MTP_PROPERTY_DATE_CREATED, MTP_PROPERTY_ARTIST, MTP_PROPERTY_ALBUM_NAME,
												MTP_PROPERTY_DURATION, MTP_PROPERTY_DESCRIPTION, MTP_PROPERTY_WIDTH, MTP_PROPERTY_HEIGHT, MTP_PROPERTY_DATE_AUTHORED,
												0xFFFF}
	},
	{ MTP_FORMAT_WAV          , (uint16_t[]){   MTP_PROPERTY_STORAGE_ID, MTP_PROPERTY_OBJECT_FORMAT, MTP_PROPERTY_PROTECTION_STATUS, MTP_PROPERTY_OBJECT_SIZE,
												MTP_PROPERTY_OBJECT_FILE_NAME, MTP_PROPERTY_DATE_MODIFIED, MTP_PROPERTY_PARENT_OBJECT, MTP_PROPERTY_PERSISTENT_UID,
												MTP_PROPERTY_NAME, MTP_PROPERTY_DISPLAY_NAME, MTP_PROPERTY_DATE_CREATED,MTP_PROPERTY_ARTIST,MTP_PROPERTY_ALBUM_NAME,
												MTP_PROPERTY_ALBUM_ARTIST, MTP_PROPERTY_TRACK, MTP_PROPERTY_ORIGINAL_RELEASE_DATE, MTP_PROPERTY_GENRE, MTP_PROPERTY_COMPOSER,
												MTP_PROPERTY_AUDIO_WAVE_CODEC, MTP_PROPERTY_BITRATE_TYPE, MTP_PROPERTY_AUDIO_BITRATE, MTP_PROPERTY_NUMBER_OF_CHANNELS,MTP_PROPERTY_SAMPLE_RATE,
												0xFFFF}
	},
	{ MTP_FORMAT_MP3          , (uint16_t[]){   MTP_PROPERTY_STORAGE_ID, MTP_PROPERTY_OBJECT_FORMAT, MTP_PROPERTY_PROTECTION_STATUS, MTP_PROPERTY_OBJECT_SIZE,
												MTP_PROPERTY_OBJECT_FILE_NAME, MTP_PROPERTY_DATE_MODIFIED, MTP_PROPERTY_PARENT_OBJECT, MTP_PROPERTY_PERSISTENT_UID,
												MTP_PROPERTY_NAME, MTP_PROPERTY_DISPLAY_NAME, MTP_PROPERTY_DATE_CREATED,MTP_PROPERTY_ARTIST,MTP_PROPERTY_ALBUM_NAME,
												MTP_PROPERTY_ALBUM_ARTIST, MTP_PROPERTY_TRACK, MTP_PROPERTY_ORIGINAL_RELEASE_DATE, MTP_PROPERTY_GENRE, MTP_PROPERTY_COMPOSER,
												MTP_PROPERTY_AUDIO_WAVE_CODEC, MTP_PROPERTY_BITRATE_TYPE, MTP_PROPERTY_AUDIO_BITRATE, MTP_PROPERTY_NUMBER_OF_CHANNELS,MTP_PROPERTY_SAMPLE_RATE,
												0xFFFF}
	},
	{ MTP_FORMAT_MPEG         , (uint16_t[]){   MTP_PROPERTY_STORAGE_ID, MTP_PROPERTY_OBJECT_FORMAT, MTP_PROPERTY_PROTECTION_STATUS, MTP_PROPERTY_OBJECT_SIZE,
												MTP_PROPERTY_OBJECT_FILE_NAME, MTP_PROPERTY_DATE_MODIFIED, MTP_PROPERTY_PARENT_OBJECT, MTP_PROPERTY_PERSISTENT_UID,
												MTP_PROPERTY_NAME, MTP_PROPERTY_DISPLAY_NAME, MTP_PROPERTY_DATE_CREATED, MTP_PROPERTY_ARTIST, MTP_PROPERTY_ALBUM_NAME,
												MTP_PROPERTY_DURATION, MTP_PROPERTY_DESCRIPTION, MTP_PROPERTY_WIDTH, MTP_PROPERTY_HEIGHT, MTP_PROPERTY_DATE_AUTHORED,
												0xFFFF}
	},
	{ MTP_FORMAT_EXIF_JPEG    , (uint16_t[]){   MTP_PROPERTY_STORAGE_ID, MTP_PROPERTY_OBJECT_FORMAT, MTP_PROPERTY_PROTECTION_STATUS, MTP_PROPERTY_OBJECT_SIZE,
												MTP_PROPERTY_OBJECT_FILE_NAME, MTP_PROPERTY_DATE_MODIFIED, MTP_PROPERTY_PARENT_OBJECT, MTP_PROPERTY_PERSISTENT_UID,
												MTP_PROPERTY_NAME, MTP_PROPERTY_DISPLAY_NAME, MTP_PROPERTY_DATE_CREATED, MTP_PROPERTY_DESCRIPTION, MTP_PROPERTY_WIDTH,
												MTP_PROPERTY_HEIGHT, MTP_PROPERTY_DATE_AUTHORED,
												0xFFFF}
	},
	{ MTP_FORMAT_BMP          , (uint16_t[]){   MTP_PROPERTY_STORAGE_ID, MTP_PROPERTY_OBJECT_FORMAT, MTP_PROPERTY_PROTECTION_STATUS, MTP_PROPERTY_OBJECT_SIZE,
												MTP_PROPERTY_OBJECT_FILE_NAME, MTP_PROPERTY_DATE_MODIFIED, MTP_PROPERTY_PARENT_OBJECT, MTP_PROPERTY_PERSISTENT_UID,
												MTP_PROPERTY_NAME, MTP_PROPERTY_DISPLAY_NAME, MTP_PROPERTY_DATE_CREATED, MTP_PROPERTY_DESCRIPTION, MTP_PROPERTY_WIDTH,
												MTP_PROPERTY_HEIGHT, MTP_PROPERTY_DATE_AUTHORED,
												0xFFFF}
	},
	{ MTP_FORMAT_GIF          , (uint16_t[]){   MTP_PROPERTY_STORAGE_ID, MTP_PROPERTY_OBJECT_FORMAT, MTP_PROPERTY_PROTECTION_STATUS, MTP_PROPERTY_OBJECT_SIZE,
												MTP_PROPERTY_OBJECT_FILE_NAME, MTP_PROPERTY_DATE_MODIFIED, MTP_PROPERTY_PARENT_OBJECT, MTP_PROPERTY_PERSISTENT_UID,
												MTP_PROPERTY_NAME, MTP_PROPERTY_DISPLAY_NAME, MTP_PROPERTY_DATE_CREATED, MTP_PROPERTY_DESCRIPTION, MTP_PROPERTY_WIDTH,
												MTP_PROPERTY_HEIGHT, MTP_PROPERTY_DATE_AUTHORED,
												0xFFFF}
	},
	{ MTP_FORMAT_JFIF         , (uint16_t[]){   MTP_PROPERTY_STORAGE_ID, MTP_PROPERTY_OBJECT_FORMAT, MTP_PROPERTY_PROTECTION_STATUS, MTP_PROPERTY_OBJECT_SIZE,
												MTP_PROPERTY_OBJECT_FILE_NAME, MTP_PROPERTY_DATE_MODIFIED, MTP_PROPERTY_PARENT_OBJECT, MTP_PROPERTY_PERSISTENT_UID,
												MTP_PROPERTY_NAME, MTP_PROPERTY_DISPLAY_NAME, MTP_PROPERTY_DATE_CREATED, MTP_PROPERTY_DESCRIPTION, MTP_PROPERTY_WIDTH,
												MTP_PROPERTY_HEIGHT, MTP_PROPERTY_DATE_AUTHORED,
												0xFFFF}
	},
	{ MTP_FORMAT_WMA          , (uint16_t[]){   MTP_PROPERTY_STORAGE_ID, MTP_PROPERTY_OBJECT_FORMAT, MTP_PROPERTY_PROTECTION_STATUS, MTP_PROPERTY_OBJECT_SIZE,
												MTP_PROPERTY_OBJECT_FILE_NAME, MTP_PROPERTY_DATE_MODIFIED, MTP_PROPERTY_PARENT_OBJECT, MTP_PROPERTY_PERSISTENT_UID,
												MTP_PROPERTY_NAME, MTP_PROPERTY_DISPLAY_NAME, MTP_PROPERTY_DATE_CREATED, MTP_PROPERTY_ARTIST, MTP_PROPERTY_ALBUM_NAME,
												MTP_PROPERTY_ALBUM_ARTIST, MTP_PROPERTY_TRACK, MTP_PROPERTY_ORIGINAL_RELEASE_DATE, MTP_PROPERTY_DURATION, MTP_PROPERTY_DESCRIPTION,
												MTP_PROPERTY_GENRE, MTP_PROPERTY_COMPOSER, MTP_PROPERTY_AUDIO_WAVE_CODEC, MTP_PROPERTY_BITRATE_TYPE, MTP_PROPERTY_AUDIO_BITRATE,
												MTP_PROPERTY_NUMBER_OF_CHANNELS, MTP_PROPERTY_SAMPLE_RATE,
												0xFFFF}
	},
	{ MTP_FORMAT_OGG          , (uint16_t[]){   MTP_PROPERTY_STORAGE_ID, MTP_PROPERTY_OBJECT_FORMAT, MTP_PROPERTY_PROTECTION_STATUS, MTP_PROPERTY_OBJECT_SIZE,
												MTP_PROPERTY_OBJECT_FILE_NAME, MTP_PROPERTY_DATE_MODIFIED, MTP_PROPERTY_PARENT_OBJECT, MTP_PROPERTY_PERSISTENT_UID,
												MTP_PROPERTY_NAME, MTP_PROPERTY_DISPLAY_NAME, MTP_PROPERTY_DATE_CREATED, MTP_PROPERTY_ARTIST, MTP_PROPERTY_ALBUM_NAME,
												MTP_PROPERTY_ALBUM_ARTIST, MTP_PROPERTY_TRACK, MTP_PROPERTY_ORIGINAL_RELEASE_DATE, MTP_PROPERTY_DURATION, MTP_PROPERTY_DESCRIPTION,
												MTP_PROPERTY_GENRE, MTP_PROPERTY_COMPOSER, MTP_PROPERTY_AUDIO_WAVE_CODEC, MTP_PROPERTY_BITRATE_TYPE, MTP_PROPERTY_AUDIO_BITRATE,
												MTP_PROPERTY_NUMBER_OF_CHANNELS, MTP_PROPERTY_SAMPLE_RATE,
												0xFFFF}
	},
	{ MTP_FORMAT_AAC          , (uint16_t[]){   MTP_PROPERTY_STORAGE_ID, MTP_PROPERTY_OBJECT_FORMAT, MTP_PROPERTY_PROTECTION_STATUS, MTP_PROPERTY_OBJECT_SIZE,
												MTP_PROPERTY_OBJECT_FILE_NAME, MTP_PROPERTY_DATE_MODIFIED, MTP_PROPERTY_PARENT_OBJECT, MTP_PROPERTY_PERSISTENT_UID,
												MTP_PROPERTY_NAME, MTP_PROPERTY_DISPLAY_NAME, MTP_PROPERTY_DATE_CREATED, MTP_PROPERTY_ARTIST, MTP_PROPERTY_ALBUM_NAME,
												MTP_PROPERTY_ALBUM_ARTIST, MTP_PROPERTY_TRACK, MTP_PROPERTY_ORIGINAL_RELEASE_DATE, MTP_PROPERTY_DURATION, MTP_PROPERTY_DESCRIPTION,
												MTP_PROPERTY_GENRE, MTP_PROPERTY_COMPOSER, MTP_PROPERTY_AUDIO_WAVE_CODEC, MTP_PROPERTY_BITRATE_TYPE, MTP_PROPERTY_AUDIO_BITRATE,
												MTP_PROPERTY_NUMBER_OF_CHANNELS, MTP_PROPERTY_SAMPLE_RATE,
												0xFFFF}
	},
	{ MTP_FORMAT_ABSTRACT_AV_PLAYLIST, (uint16_t[]){   MTP_PROPERTY_STORAGE_ID, MTP_PROPERTY_OBJECT_FORMAT, MTP_PROPERTY_PROTECTION_STATUS, MTP_PROPERTY_OBJECT_SIZE,
												MTP_PROPERTY_OBJECT_FILE_NAME, MTP_PROPERTY_DATE_MODIFIED, MTP_PROPERTY_PARENT_OBJECT, MTP_PROPERTY_PERSISTENT_UID,
												MTP_PROPERTY_NAME, MTP_PROPERTY_DISPLAY_NAME, MTP_PROPERTY_DATE_CREATED,
												0xFFFF}
	},
	{ MTP_FORMAT_WPL_PLAYLIST,  (uint16_t[]){   MTP_PROPERTY_STORAGE_ID, MTP_PROPERTY_OBJECT_FORMAT, MTP_PROPERTY_PROTECTION_STATUS, MTP_PROPERTY_OBJECT_SIZE,
												MTP_PROPERTY_OBJECT_FILE_NAME, MTP_PROPERTY_DATE_MODIFIED, MTP_PROPERTY_PARENT_OBJECT, MTP_PROPERTY_PERSISTENT_UID,
												MTP_PROPERTY_NAME, MTP_PROPERTY_DISPLAY_NAME, MTP_PROPERTY_DATE_CREATED,
												0xFFFF}
	},
	{ MTP_FORMAT_M3U_PLAYLIST,  (uint16_t[]){   MTP_PROPERTY_STORAGE_ID, MTP_PROPERTY_OBJECT_FORMAT, MTP_PROPERTY_PROTECTION_STATUS, MTP_PROPERTY_OBJECT_SIZE,
												MTP_PROPERTY_OBJECT_FILE_NAME, MTP_PROPERTY_DATE_MODIFIED, MTP_PROPERTY_PARENT_OBJECT, MTP_PROPERTY_PERSISTENT_UID,
												MTP_PROPERTY_NAME, MTP_PROPERTY_DISPLAY_NAME, MTP_PROPERTY_DATE_CREATED,
												0xFFFF}
	},
	{ MTP_FORMAT_PLS_PLAYLIST,  (uint16_t[]){   MTP_PROPERTY_STORAGE_ID, MTP_PROPERTY_OBJECT_FORMAT, MTP_PROPERTY_PROTECTION_STATUS, MTP_PROPERTY_OBJECT_SIZE,
												MTP_PROPERTY_OBJECT_FILE_NAME, MTP_PROPERTY_DATE_MODIFIED, MTP_PROPERTY_PARENT_OBJECT, MTP_PROPERTY_PERSISTENT_UID,
												MTP_PROPERTY_NAME, MTP_PROPERTY_DISPLAY_NAME, MTP_PROPERTY_DATE_CREATED,
												0xFFFF}
	},
	{ MTP_FORMAT_XML_DOCUMENT,  (uint16_t[]){   MTP_PROPERTY_STORAGE_ID, MTP_PROPERTY_OBJECT_FORMAT, MTP_PROPERTY_PROTECTION_STATUS, MTP_PROPERTY_OBJECT_SIZE,
												MTP_PROPERTY_OBJECT_FILE_NAME, MTP_PROPERTY_DATE_MODIFIED, MTP_PROPERTY_PARENT_OBJECT, MTP_PROPERTY_PERSISTENT_UID,
												MTP_PROPERTY_NAME, MTP_PROPERTY_DISPLAY_NAME, MTP_PROPERTY_DATE_CREATED,
												0xFFFF}
	},
	{ MTP_FORMAT_FLAC        ,  (uint16_t[]){   MTP_PROPERTY_STORAGE_ID, MTP_PROPERTY_OBJECT_FORMAT, MTP_PROPERTY_PROTECTION_STATUS, MTP_PROPERTY_OBJECT_SIZE,
												MTP_PROPERTY_OBJECT_FILE_NAME, MTP_PROPERTY_DATE_MODIFIED, MTP_PROPERTY_PARENT_OBJECT, MTP_PROPERTY_PERSISTENT_UID,
												MTP_PROPERTY_NAME, MTP_PROPERTY_DISPLAY_NAME, MTP_PROPERTY_DATE_CREATED,
												0xFFFF}
	},
	{ MTP_FORMAT_AVI          , (uint16_t[]){   MTP_PROPERTY_STORAGE_ID, MTP_PROPERTY_OBJECT_FORMAT, MTP_PROPERTY_PROTECTION_STATUS, MTP_PROPERTY_OBJECT_SIZE,
												MTP_PROPERTY_OBJECT_FILE_NAME, MTP_PROPERTY_DATE_MODIFIED, MTP_PROPERTY_PARENT_OBJECT, MTP_PROPERTY_PERSISTENT_UID,
												MTP_PROPERTY_NAME, MTP_PROPERTY_DISPLAY_NAME, MTP_PROPERTY_DATE_CREATED, MTP_PROPERTY_ARTIST, MTP_PROPERTY_ALBUM_NAME,
												MTP_PROPERTY_DURATION, MTP_PROPERTY_DESCRIPTION, MTP_PROPERTY_WIDTH, MTP_PROPERTY_HEIGHT, MTP_PROPERTY_DATE_AUTHORED,
												0xFFFF}
	},
	{ MTP_FORMAT_ASF          , (uint16_t[]){   MTP_PROPERTY_STORAGE_ID, MTP_PROPERTY_OBJECT_FORMAT, MTP_PROPERTY_PROTECTION_STATUS, MTP_PROPERTY_OBJECT_SIZE,
												MTP_PROPERTY_OBJECT_FILE_NAME, MTP_PROPERTY_DATE_MODIFIED, MTP_PROPERTY_PARENT_OBJECT, MTP_PROPERTY_PERSISTENT_UID,
												MTP_PROPERTY_NAME, MTP_PROPERTY_DISPLAY_NAME, MTP_PROPERTY_DATE_CREATED, MTP_PROPERTY_ARTIST, MTP_PROPERTY_ALBUM_NAME,
												MTP_PROPERTY_DURATION, MTP_PROPERTY_DESCRIPTION, MTP_PROPERTY_WIDTH, MTP_PROPERTY_HEIGHT, MTP_PROPERTY_DATE_AUTHORED,
												0xFFFF}
	},
	{ MTP_FORMAT_MS_WORD_DOCUMENT, (uint16_t[]){   MTP_PROPERTY_STORAGE_ID, MTP_PROPERTY_OBJECT_FORMAT, MTP_PROPERTY_PROTECTION_STATUS, MTP_PROPERTY_OBJECT_SIZE,
												MTP_PROPERTY_OBJECT_FILE_NAME, MTP_PROPERTY_DATE_MODIFIED, MTP_PROPERTY_PARENT_OBJECT, MTP_PROPERTY_PERSISTENT_UID,
												MTP_PROPERTY_NAME, MTP_PROPERTY_DISPLAY_NAME, MTP_PROPERTY_DATE_CREATED,
												0xFFFF}
	},
	{ MTP_FORMAT_MS_EXCEL_SPREADSHEET, (uint16_t[]){   MTP_PROPERTY_STORAGE_ID, MTP_PROPERTY_OBJECT_FORMAT, MTP_PROPERTY_PROTECTION_STATUS, MTP_PROPERTY_OBJECT_SIZE,
												MTP_PROPERTY_OBJECT_FILE_NAME, MTP_PROPERTY_DATE_MODIFIED, MTP_PROPERTY_PARENT_OBJECT, MTP_PROPERTY_PERSISTENT_UID,
												MTP_PROPERTY_NAME, MTP_PROPERTY_DISPLAY_NAME, MTP_PROPERTY_DATE_CREATED,
												0xFFFF}
	},
	{ MTP_FORMAT_MS_POWERPOINT_PRESENTATION, (uint16_t[]){   MTP_PROPERTY_STORAGE_ID, MTP_PROPERTY_OBJECT_FORMAT, MTP_PROPERTY_PROTECTION_STATUS, MTP_PROPERTY_OBJECT_SIZE,
												MTP_PROPERTY_OBJECT_FILE_NAME, MTP_PROPERTY_DATE_MODIFIED, MTP_PROPERTY_PARENT_OBJECT, MTP_PROPERTY_PERSISTENT_UID,
												MTP_PROPERTY_NAME, MTP_PROPERTY_DISPLAY_NAME, MTP_PROPERTY_DATE_CREATED,
												0xFFFF}
	}
#endif
	,

	{ 0xFFFF  , (uint16_t[]){ 0xFFFF } }

};

profile_property properties[]=
{   // prop_code                       data_type         getset    default value          group code           format id
	{MTP_PROPERTY_STORAGE_ID,          MTP_TYPE_UINT32,    0x00,   0x00000000           , 0x000000001 , 0x00 , 0xFFFF },
	{MTP_PROPERTY_OBJECT_FORMAT,       MTP_TYPE_UINT16,    0x00,   MTP_FORMAT_UNDEFINED  , 0x000000000 , 0x00 , MTP_FORMAT_UNDEFINED },
	{MTP_PROPERTY_OBJECT_FORMAT,       MTP_TYPE_UINT16,    0x00,   MTP_FORMAT_ASSOCIATION, 0x000000000 , 0x00 , MTP_FORMAT_ASSOCIATION },
	{MTP_PROPERTY_OBJECT_FORMAT,       MTP_TYPE_UINT16,    0x00,   MTP_FORMAT_TEXT       , 0x000000000 , 0x00 , MTP_FORMAT_TEXT },
	{MTP_PROPERTY_OBJECT_FORMAT,       MTP_TYPE_UINT16,    0x00,   MTP_FORMAT_HTML       , 0x000000000 , 0x00 , MTP_FORMAT_HTML },
	{MTP_PROPERTY_OBJECT_FORMAT,       MTP_TYPE_UINT16,    0x00,   MTP_FORMAT_WAV        , 0x000000000 , 0x00 , MTP_FORMAT_WAV },
	{MTP_PROPERTY_OBJECT_FORMAT,       MTP_TYPE_UINT16,    0x00,   MTP_FORMAT_MP3        , 0x000000000 , 0x00 , MTP_FORMAT_MP3 },
	{MTP_PROPERTY_OBJECT_FORMAT,       MTP_TYPE_UINT16,    0x00,   MTP_FORMAT_MPEG       , 0x000000000 , 0x00 , MTP_FORMAT_MPEG },
	{MTP_PROPERTY_OBJECT_FORMAT,       MTP_TYPE_UINT16,    0x00,   MTP_FORMAT_EXIF_JPEG  , 0x000000000 , 0x00 , MTP_FORMAT_EXIF_JPEG },
	{MTP_PROPERTY_OBJECT_FORMAT,       MTP_TYPE_UINT16,    0x00,   MTP_FORMAT_BMP        , 0x000000000 , 0x00 , MTP_FORMAT_BMP },
	{MTP_PROPERTY_OBJECT_FORMAT,       MTP_TYPE_UINT16,    0x00,   MTP_FORMAT_AIFF       , 0x000000000 , 0x00 , MTP_FORMAT_AIFF },
	{MTP_PROPERTY_OBJECT_FORMAT,       MTP_TYPE_UINT16,    0x00,   MTP_FORMAT_MPEG       , 0x000000000 , 0x00 , MTP_FORMAT_MPEG },
	{MTP_PROPERTY_OBJECT_FORMAT,       MTP_TYPE_UINT16,    0x00,   MTP_FORMAT_WMA        , 0x000000000 , 0x00 , MTP_FORMAT_WMA },
	{MTP_PROPERTY_OBJECT_FORMAT,       MTP_TYPE_UINT16,    0x00,   MTP_FORMAT_OGG        , 0x000000000 , 0x00 , MTP_FORMAT_OGG },
	{MTP_PROPERTY_OBJECT_FORMAT,       MTP_TYPE_UINT16,    0x00,   MTP_FORMAT_AAC        , 0x000000000 , 0x00 , MTP_FORMAT_AAC },
	{MTP_PROPERTY_OBJECT_FORMAT,       MTP_TYPE_UINT16,    0x00,   MTP_FORMAT_MP4_CONTAINER                 , 0x000000000 , 0x00 , MTP_FORMAT_MP4_CONTAINER },
	{MTP_PROPERTY_OBJECT_FORMAT,       MTP_TYPE_UINT16,    0x00,   MTP_FORMAT_3GP_CONTAINER                 , 0x000000000 , 0x00 , MTP_FORMAT_3GP_CONTAINER },
	{MTP_PROPERTY_OBJECT_FORMAT,       MTP_TYPE_UINT16,    0x00,   MTP_FORMAT_ABSTRACT_AV_PLAYLIST          , 0x000000000 , 0x00 , MTP_FORMAT_ABSTRACT_AV_PLAYLIST },
	{MTP_PROPERTY_OBJECT_FORMAT,       MTP_TYPE_UINT16,    0x00,   MTP_FORMAT_WPL_PLAYLIST                  , 0x000000000 , 0x00 , MTP_FORMAT_WPL_PLAYLIST },
	{MTP_PROPERTY_OBJECT_FORMAT,       MTP_TYPE_UINT16,    0x00,   MTP_FORMAT_M3U_PLAYLIST                  , 0x000000000 , 0x00 , MTP_FORMAT_M3U_PLAYLIST },
	{MTP_PROPERTY_OBJECT_FORMAT,       MTP_TYPE_UINT16,    0x00,   MTP_FORMAT_PLS_PLAYLIST                  , 0x000000000 , 0x00 , MTP_FORMAT_PLS_PLAYLIST },
	{MTP_PROPERTY_OBJECT_FORMAT,       MTP_TYPE_UINT16,    0x00,   MTP_FORMAT_XML_DOCUMENT                  , 0x000000000 , 0x00 , MTP_FORMAT_XML_DOCUMENT },
	{MTP_PROPERTY_OBJECT_FORMAT,       MTP_TYPE_UINT16,    0x00,   MTP_FORMAT_FLAC                          , 0x000000000 , 0x00 , MTP_FORMAT_FLAC },
	{MTP_PROPERTY_OBJECT_FORMAT,       MTP_TYPE_UINT16,    0x00,   MTP_FORMAT_AVI                           , 0x000000000 , 0x00 , MTP_FORMAT_AVI },
	{MTP_PROPERTY_OBJECT_FORMAT,       MTP_TYPE_UINT16,    0x00,   MTP_FORMAT_ASF                           , 0x000000000 , 0x00 , MTP_FORMAT_ASF },
	{MTP_PROPERTY_OBJECT_FORMAT,       MTP_TYPE_UINT16,    0x00,   MTP_FORMAT_MS_WORD_DOCUMENT              , 0x000000000 , 0x00 , MTP_FORMAT_MS_WORD_DOCUMENT },
	{MTP_PROPERTY_OBJECT_FORMAT,       MTP_TYPE_UINT16,    0x00,   MTP_FORMAT_MS_EXCEL_SPREADSHEET          , 0x000000000 , 0x00 , MTP_FORMAT_MS_EXCEL_SPREADSHEET },
	{MTP_PROPERTY_OBJECT_FORMAT,       MTP_TYPE_UINT16,    0x00,   MTP_FORMAT_MS_POWERPOINT_PRESENTATION    , 0x000000000 , 0x00 , MTP_FORMAT_MS_POWERPOINT_PRESENTATION },

	{MTP_PROPERTY_OBJECT_SIZE,         MTP_TYPE_UINT64,    0x00,   0x0000000000000000 , 0x000000000 , 0x00 , MTP_FORMAT_ASSOCIATION },
	{MTP_PROPERTY_STORAGE_ID,          MTP_TYPE_UINT32,    0x00,   0x00000000         , 0x000000000 , 0x00 , MTP_FORMAT_ASSOCIATION },
	{MTP_PROPERTY_PROTECTION_STATUS,   MTP_TYPE_UINT16,    0x00,   0x0000             , 0x000000000 , 0x00 , MTP_FORMAT_ASSOCIATION },
	{MTP_PROPERTY_DISPLAY_NAME,        MTP_TYPE_STR,       0x00,   0x0000             , 0x000000000 , 0x00 , MTP_FORMAT_ASSOCIATION },
	{MTP_PROPERTY_OBJECT_FILE_NAME,    MTP_TYPE_STR,       0x01,   0x0000             , 0x000000000 , 0x00 , MTP_FORMAT_ASSOCIATION },
	{MTP_PROPERTY_DATE_CREATED,        MTP_TYPE_STR,       0x00,   0x00               , 0x000000000 , 0x00 , MTP_FORMAT_ASSOCIATION },
	{MTP_PROPERTY_DATE_MODIFIED,       MTP_TYPE_STR,       0x00,   0x00               , 0x000000000 , 0x00 , MTP_FORMAT_ASSOCIATION },
	{MTP_PROPERTY_PARENT_OBJECT,       MTP_TYPE_UINT32,    0x00,   0x00000000         , 0x000000000 , 0x00 , MTP_FORMAT_ASSOCIATION },
	{MTP_PROPERTY_PERSISTENT_UID,      MTP_TYPE_UINT128,   0x00,   0x00               , 0x000000000 , 0x00 , MTP_FORMAT_ASSOCIATION },
	{MTP_PROPERTY_NAME,                MTP_TYPE_STR,       0x00,   0x00               , 0x000000000 , 0x00 , MTP_FORMAT_ASSOCIATION },

	{MTP_PROPERTY_OBJECT_SIZE,         MTP_TYPE_UINT64,    0x00,   0x0000000000000000 , 0x000000000 , 0x00 , MTP_FORMAT_UNDEFINED },
	{MTP_PROPERTY_STORAGE_ID,          MTP_TYPE_UINT32,    0x00,   0x00000000         , 0x000000000 , 0x00 , MTP_FORMAT_UNDEFINED },
	{MTP_PROPERTY_PROTECTION_STATUS,   MTP_TYPE_UINT16,    0x00,   0x0000             , 0x000000000 , 0x00 , MTP_FORMAT_UNDEFINED },
	{MTP_PROPERTY_DISPLAY_NAME,        MTP_TYPE_STR,       0x00,   0x0000             , 0x000000000 , 0x00 , MTP_FORMAT_UNDEFINED },
	{MTP_PROPERTY_OBJECT_FILE_NAME,    MTP_TYPE_STR,       0x01,   0x0000             , 0x000000000 , 0x00 , MTP_FORMAT_UNDEFINED },
	{MTP_PROPERTY_DATE_CREATED,        MTP_TYPE_STR,       0x00,   0x00               , 0x000000000 , 0x00 , MTP_FORMAT_UNDEFINED },
	{MTP_PROPERTY_DATE_MODIFIED,       MTP_TYPE_STR,       0x00,   0x00               , 0x000000000 , 0x00 , MTP_FORMAT_UNDEFINED },
	{MTP_PROPERTY_PARENT_OBJECT,       MTP_TYPE_UINT32,    0x00,   0x00000000         , 0x000000000 , 0x00 , MTP_FORMAT_UNDEFINED },
	{MTP_PROPERTY_PERSISTENT_UID,      MTP_TYPE_UINT128,   0x00,   0x00               , 0x000000000 , 0x00 , MTP_FORMAT_UNDEFINED },
	{MTP_PROPERTY_NAME,                MTP_TYPE_STR,       0x00,   0x00               , 0x000000000 , 0x00 , MTP_FORMAT_UNDEFINED },

	//{MTP_PROPERTY_ASSOCIATION_TYPE,    MTP_TYPE_UINT16,    0x00,   0x0001             , 0x000000000 , 0x00 , 0xFFFF },
	{MTP_PROPERTY_ASSOCIATION_DESC,    MTP_TYPE_UINT32,    0x00,   0x00000000         , 0x000000000 , 0x00 , 0xFFFF },
	{MTP_PROPERTY_PROTECTION_STATUS,   MTP_TYPE_UINT16,    0x00,   0x0000             , 0x000000000 , 0x00 , 0xFFFF },
	{MTP_PROPERTY_HIDDEN,              MTP_TYPE_UINT16,    0x00,   0x0000             , 0x000000000 , 0x00 , 0xFFFF },

	{0xFFFF,                           MTP_TYPE_UINT32,    0x00,   0x00000000         , 0x000000000 , 0x00 }
};

profile_property dev_properties[]=
{   // prop_code                                           data_type         getset    default value          group code
	//{MTP_DEVICE_PROPERTY_SYNCHRONIZATION_PARTNER,          MTP_TYPE_UINT32,    0x00,   0x00000000           , 0x000000000 , 0x00 },
	//{MTP_DEVICE_PROPERTY_IMAGE_SIZE,                       MTP_TYPE_UINT32,    0x00,   0x00000000           , 0x000000000 , 0x00 },
	{MTP_DEVICE_PROPERTY_BATTERY_LEVEL,                    MTP_TYPE_UINT16,    0x00,   0x00000000           , 0x000000000 , 0x00 },
	{MTP_DEVICE_PROPERTY_DEVICE_FRIENDLY_NAME,             MTP_TYPE_STR,       0x00,   0x00000000           , 0x000000000 , 0x00 },

	{0xFFFF,                                               MTP_TYPE_UINT32,    0x00,   0x00000000           , 0x000000000 , 0x00 }
};

int build_properties_dataset(mtp_ctx * ctx,void * buffer, int maxsize,uint32_t property_id,uint32_t format_id)
{
	int ofs,i,j;

	ofs = 0;

	PRINT_DEBUG("build_properties_dataset : 0x%.4X (%s) (Format : 0x%.4X - %s )", property_id, mtp_get_property_string(property_id), format_id, mtp_get_format_string(format_id));

	i = 0;
	while(properties[i].prop_code != 0xFFFF )
	{
		if( ( properties[i].prop_code == property_id ) && ( properties[i].format_id == format_id ) )
		{
			break;
		}
		i++;
	}

	if( properties[i].prop_code == 0xFFFF )
	{
		// Looking for default value
		i = 0;
		while(properties[i].prop_code != 0xFFFF )
		{
			if( ( properties[i].prop_code == property_id ) && ( properties[i].format_id == 0xFFFF ) )
			{
				break;
			}
			i++;
		}
	}

	if( properties[i].prop_code == property_id )
	{
		ofs = poke16(buffer, ofs, maxsize, properties[i].prop_code);            // PropertyCode
		ofs = poke16(buffer, ofs, maxsize, properties[i].data_type);            // DataType
		ofs = poke08(buffer, ofs, maxsize, properties[i].getset);               // Get / Set

		switch(properties[i].data_type)
		{
			case MTP_TYPE_STR:
			case MTP_TYPE_UINT8:
				ofs = poke08(buffer, ofs, maxsize, properties[i].default_value);                         // DefaultValue
			break;
			case MTP_TYPE_UINT16:
				ofs = poke16(buffer, ofs, maxsize, properties[i].default_value);                         // DefaultValue
			break;
			case MTP_TYPE_UINT32:
				ofs = poke32(buffer, ofs, maxsize, properties[i].default_value);                         // DefaultValue
			break;
			case MTP_TYPE_UINT64:
				ofs = poke32(buffer, ofs, maxsize, properties[i].default_value & 0xFFFFFFFF);            // DefaultValue
				ofs = poke32(buffer, ofs, maxsize, properties[i].default_value >> 32);
			break;
			case MTP_TYPE_UINT128:
				for(j=0;j<4;j++)
				{
					ofs = poke32(buffer, ofs, maxsize, properties[i].default_value);
				}
			break;
			default:
				PRINT_ERROR("build_properties_dataset : Unsupported data type : 0x%.4X", properties[i].data_type );
			break;
		}

		ofs = poke32(buffer, ofs, maxsize, properties[i].group_code);           // Group code
		ofs = poke08(buffer, ofs, maxsize, properties[i].form_flag);            // Form flag
	}

	return ofs;
}

int build_device_properties_dataset(mtp_ctx * ctx,void * buffer, int maxsize,uint32_t property_id)
{
	int ofs,i;

	ofs = 0;

	PRINT_DEBUG("build_device_properties_dataset : 0x%.4X (%s)", property_id, mtp_get_property_string(property_id));

	i = 0;
	while(dev_properties[i].prop_code != 0xFFFF && dev_properties[i].prop_code != property_id)
	{
		i++;
	}

	if( dev_properties[i].prop_code == property_id )
	{
		ofs = poke16(buffer, ofs, maxsize, dev_properties[i].prop_code);            // PropertyCode
		ofs = poke16(buffer, ofs, maxsize, dev_properties[i].data_type);            // DataType
		ofs = poke08(buffer, ofs, maxsize, dev_properties[i].getset);               // Get / Set

		switch(dev_properties[i].data_type)
		{
			case MTP_TYPE_STR:
			case MTP_TYPE_UINT8:
				ofs = poke08(buffer, ofs, maxsize, dev_properties[i].default_value);
				ofs = poke08(buffer, ofs, maxsize, dev_properties[i].default_value);
			break;

			case MTP_TYPE_UINT16:
				ofs = poke16(buffer, ofs, maxsize, dev_properties[i].default_value);
				ofs = poke16(buffer, ofs, maxsize, dev_properties[i].default_value);
			break;

			case MTP_TYPE_UINT32:
				ofs = poke32(buffer, ofs, maxsize, dev_properties[i].default_value);
				ofs = poke32(buffer, ofs, maxsize, dev_properties[i].default_value);
			break;

			case MTP_TYPE_UINT64:
				ofs = poke32(buffer, ofs, maxsize, dev_properties[i].default_value & 0xFFFFFFFF);
				ofs = poke32(buffer, ofs, maxsize, dev_properties[i].default_value >> 32);
				ofs = poke32(buffer, ofs, maxsize, dev_properties[i].default_value & 0xFFFFFFFF);
				ofs = poke32(buffer, ofs, maxsize, dev_properties[i].default_value >> 32);
			break;

			default:
				PRINT_ERROR("build_device_properties_dataset : Unsupported data type : 0x%.4X", dev_properties[i].data_type );
				return 0;
			break;
		}

		ofs = poke32(buffer, ofs, maxsize, dev_properties[i].group_code);           // Group code
		ofs = poke08(buffer, ofs, maxsize, dev_properties[i].form_flag);            // Form flag
	}

	return ofs;
}


int build_properties_supported_dataset(mtp_ctx * ctx,void * buffer, int maxsize,uint32_t format_id)
{
	int ofs,i,fmt_index;
	int nb_supported_prop;

	PRINT_DEBUG("build_properties_supported_dataset : (Format : 0x%.4X - %s )", format_id, mtp_get_format_string(format_id));

	fmt_index = 0;
	while(fmt_properties[fmt_index].format_code != 0xFFFF && fmt_properties[fmt_index].format_code != format_id )
	{
		fmt_index++;
	}

	if( fmt_properties[fmt_index].format_code == 0xFFFF )
		return 0;

	nb_supported_prop = 0;

	while( fmt_properties[fmt_index].properties[nb_supported_prop] != 0xFFFF )
		nb_supported_prop++;

	i = 0;

	ofs = poke32(buffer, 0, maxsize, nb_supported_prop);
	while( fmt_properties[fmt_index].properties[i] != 0xFFFF )
	{
		ofs = poke16(buffer, ofs, maxsize, fmt_properties[fmt_index].properties[i]);
		i++;
	}

	return ofs;
}

int setObjectPropValue(mtp_ctx * ctx,MTP_PACKET_HEADER * mtp_packet_hdr, uint32_t handle,uint32_t prop_code)
{
	fs_entry * entry;
	char * path;
	char * path2;
	char * old_filename;
	char tmpstr[256+1];
	unsigned int stringlen;
	uint32_t response_code;
	int ret;

	PRINT_DEBUG("setObjectPropValue : (Handle : 0x%.8X - Prop code : 0x%.4X )", handle, prop_code);

	response_code = 0x00000000;

	if( handle != 0xFFFFFFFF )
	{
		switch( prop_code )
		{
			case MTP_PROPERTY_OBJECT_FILE_NAME:
				entry = get_entry_by_handle(ctx->fs_db, handle);

				if( check_handle_access( ctx, entry, 0x00000000, 1, &response_code) )
					return response_code;

				path = build_full_path(ctx->fs_db, mtp_get_storage_root(ctx, entry->storage_id), entry);
				if(!path)
					return MTP_RESPONSE_GENERAL_ERROR;

				memset(tmpstr,0,sizeof(tmpstr));

				stringlen = peek(mtp_packet_hdr, sizeof(MTP_PACKET_HEADER), 1);

				if( stringlen > sizeof(tmpstr))
					stringlen = sizeof(tmpstr);

				unicode2charstring(tmpstr, (uint16_t *) ((char*)(mtp_packet_hdr) + sizeof(MTP_PACKET_HEADER) + 1), sizeof(tmpstr));
				tmpstr[ sizeof(tmpstr) - 1 ] = 0;

				old_filename = entry->name;

				entry->name = malloc(strlen(tmpstr)+1);
				if( entry->name )
				{
					strcpy(entry->name,tmpstr);
				}
				else
				{
					entry->name = old_filename;
					return MTP_RESPONSE_GENERAL_ERROR;
				}

				path2 = build_full_path(ctx->fs_db, mtp_get_storage_root(ctx, entry->storage_id), entry);
				if(!path2)
				{
					if( old_filename )
					{
						if(entry->name)
							free(entry->name);

						entry->name = old_filename;
					}

					free(path);
					return MTP_RESPONSE_GENERAL_ERROR;
				}

				ret = -1;

				if(!set_storage_giduid(ctx, entry->storage_id))
				{
					ret = rename(path, path2);
				}
				restore_giduid(ctx);

				if(ret)
				{
					PRINT_ERROR("setObjectPropValue : Can't rename %s to %s", path, path2);

					if( old_filename )
					{
						if(entry->name)
							free(entry->name);

						entry->name = old_filename;
					}

					free(path);
					free(path2);
					return MTP_RESPONSE_GENERAL_ERROR;
				}

				free(path);
				free(path2);
				return MTP_RESPONSE_OK;
			break;

			default:
				return MTP_RESPONSE_INVALID_OBJECT_PROP_CODE;
			break;
		}
	}
	else
	{
		return MTP_RESPONSE_INVALID_OBJECT_HANDLE;
	}
}

int build_ObjectPropValue_dataset(mtp_ctx * ctx,void * buffer, int maxsize,uint32_t handle,uint32_t prop_code)
{
	int ofs;
	fs_entry * entry;
	char timestr[32];

	ofs = 0;

	PRINT_DEBUG("build_ObjectPropValue_dataset : Handle 0x%.8X - Property 0x%.4X (%s)", handle, prop_code, mtp_get_property_string(prop_code));

	entry = get_entry_by_handle(ctx->fs_db, handle);
	if( entry )
	{
		switch(prop_code)
		{
			case MTP_PROPERTY_OBJECT_FORMAT:
				if(entry->flags & ENTRY_IS_DIR)
					ofs = poke16(buffer, ofs, maxsize, MTP_FORMAT_ASSOCIATION);                          // ObjectFormat Code
				else
					ofs = poke16(buffer, ofs, maxsize, MTP_FORMAT_UNDEFINED);                            // ObjectFormat Code
			break;

			case MTP_PROPERTY_OBJECT_SIZE:
				ofs = poke32(buffer, ofs, maxsize, entry->size & 0xFFFFFFFF);
				ofs = poke32(buffer, ofs, maxsize, entry->size >> 32);
			break;

			case MTP_PROPERTY_DISPLAY_NAME:
				ofs = poke08(buffer, ofs, maxsize, 0);
			break;

			case MTP_PROPERTY_NAME:
			case MTP_PROPERTY_OBJECT_FILE_NAME:
				ofs = poke_string(buffer, ofs, maxsize, entry->name);                                      // Filename
			break;

			case MTP_PROPERTY_STORAGE_ID:
				ofs = poke32(buffer, ofs, maxsize, entry->storage_id);
			break;

			case MTP_PROPERTY_PARENT_OBJECT:
				ofs = poke32(buffer, ofs, maxsize, entry->parent);
			break;

			case MTP_PROPERTY_HIDDEN:
				ofs = poke16(buffer, ofs, maxsize, 0x0000);
			break;

			case MTP_PROPERTY_SYSTEM_OBJECT:
				ofs = poke16(buffer, ofs, maxsize, 0x0000);
			break;

			case MTP_PROPERTY_PROTECTION_STATUS:
				ofs = poke16(buffer, ofs, maxsize, 0x0000);
			break;

			case MTP_PROPERTY_ASSOCIATION_TYPE:
				if(entry->flags & ENTRY_IS_DIR)
						ofs = poke16(buffer, ofs, maxsize, 0x0001);                          // ObjectFormat Code
				else
						ofs = poke16(buffer, ofs, maxsize, 0x0000);                          // ObjectFormat Code
			break;

			case MTP_PROPERTY_ASSOCIATION_DESC:
				ofs = poke32(buffer, ofs, maxsize, 0x00000000);
			break;

			case MTP_PROPERTY_DATE_CREATED:
			case MTP_PROPERTY_DATE_MODIFIED:
				snprintf(timestr,sizeof(timestr),"%.4d%.2d%.2dT%.2d%.2d%.2d",1900 + 110, 1, 2, 10, 11,12);
				ofs = poke_string(buffer, ofs, maxsize, timestr);
			break;

			case MTP_PROPERTY_PERSISTENT_UID:
				ofs = poke32(buffer, ofs, maxsize, entry->handle);
				ofs = poke32(buffer, ofs, maxsize, entry->parent);
				ofs = poke32(buffer, ofs, maxsize, entry->storage_id);
				ofs = poke32(buffer, ofs, maxsize, 0x00000000);
			break;

			default:
				PRINT_ERROR("build_ObjectPropValue_dataset : Unsupported property : 0x%.4X (%s)", prop_code, mtp_get_property_string(prop_code));
				return 0;
			break;
		}
	}

	return ofs;
}

int build_DevicePropValue_dataset(mtp_ctx * ctx,void * buffer, int maxsize,uint32_t prop_code)
{
	int ofs;

	ofs = 0;

	PRINT_DEBUG("build_DevicePropValue_dataset : Property 0x%.4X (%s)", prop_code, mtp_get_property_string(prop_code));

	switch(prop_code)
	{
		case MTP_DEVICE_PROPERTY_BATTERY_LEVEL:
			ofs = poke16(buffer, ofs, maxsize, 0x8000);
		break;

		case MTP_DEVICE_PROPERTY_DEVICE_FRIENDLY_NAME:
			ofs = poke_string(buffer, ofs, maxsize, ctx->usb_cfg.usb_string_product);
		break;

		default:
			PRINT_ERROR("build_DevicePropValue_dataset : Unsupported property : 0x%.4X (%s)", prop_code, mtp_get_property_string(prop_code));
			return 0;
		break;
	}

	return ofs;
}

int objectproplist_element(mtp_ctx * ctx, void * buffer, int * ofs, int maxsize, uint16_t prop_code, uint32_t handle, void * data,uint32_t prop_code_param)
{
	int i;

	if( (prop_code != prop_code_param) && (prop_code_param != 0xFFFFFFFF) )
	{
		return 0;
	}

	i = 0;
	while(properties[i].prop_code != 0xFFFF && properties[i].prop_code != prop_code)
	{
		i++;
	}

	if( properties[i].prop_code == prop_code )
	{
		*ofs = poke32(buffer, *ofs, maxsize, handle);
		*ofs = poke16(buffer, *ofs, maxsize, properties[i].prop_code);
		*ofs = poke16(buffer, *ofs, maxsize, properties[i].data_type);
		switch(properties[i].data_type)
		{
			case MTP_TYPE_STR:
				if(data)
					*ofs = poke_string(buffer, *ofs, maxsize, (char*)data);
				else
					*ofs = poke08(buffer, *ofs, maxsize, 0);
			break;
			case MTP_TYPE_UINT8:
				*ofs = poke08(buffer, *ofs, maxsize, *((uint8_t*)data));
			break;
			case MTP_TYPE_UINT16:
				*ofs = poke16(buffer, *ofs, maxsize, *((uint16_t*)data));
			break;
			case MTP_TYPE_UINT32:
				*ofs = poke32(buffer, *ofs, maxsize, *((uint32_t*)data));
			break;
			case MTP_TYPE_UINT64:
				*ofs = poke32(buffer, *ofs, maxsize, *((uint64_t*)data) & 0xFFFFFFFF);
				*ofs = poke32(buffer, *ofs, maxsize, *((uint64_t*)data) >> 32);
			break;
			case MTP_TYPE_UINT128:
				for(i=0;i<4;i++)
				{
					*ofs = poke32(buffer, *ofs, maxsize, *((uint32_t*)data)+i);
				}
			break;
			default:
				PRINT_ERROR("objectproplist_element : Unsupported data type : 0x%.4X", properties[i].data_type );
			break;
		}

		return 1;
	}

	return 0;
}

int build_objectproplist_dataset(mtp_ctx * ctx, void * buffer, int maxsize,fs_entry * entry, uint32_t handle,uint32_t format_id, uint32_t prop_code, uint32_t prop_group_code, uint32_t depth)
{
	struct stat64 entrystat;
	time_t t;
	struct tm lt;
	int ofs,ret,numberofelements;
	char * path;
	char timestr[32];
	uint32_t tmp_dword;
	uint32_t tmp_dword_array[4];

	ret = -1;
	path = build_full_path(ctx->fs_db, mtp_get_storage_root(ctx, entry->storage_id), entry);

	if(path)
	{
		ret = stat64(path, &entrystat);
	}

	if(ret)
	{
		if(path)
			free(path);
		return 0;
	}

	/* update the file size infomation */
	entry->size = entrystat.st_size;

	numberofelements = 0;

	ofs = poke32(buffer, 0, maxsize, numberofelements);   // Number of elements

	numberofelements += objectproplist_element(ctx, buffer, &ofs, maxsize, MTP_PROPERTY_STORAGE_ID, handle, &entry->storage_id,prop_code);

	if(entry->flags & ENTRY_IS_DIR)
		tmp_dword = MTP_FORMAT_ASSOCIATION;
	else
		tmp_dword = MTP_FORMAT_UNDEFINED;

	numberofelements += objectproplist_element(ctx, buffer, &ofs, maxsize, MTP_PROPERTY_OBJECT_FORMAT, handle, &tmp_dword,prop_code);

	if(entry->flags & ENTRY_IS_DIR)
		tmp_dword = MTP_ASSOCIATION_TYPE_GENERIC_FOLDER;
	else
		tmp_dword = 0x0000;

	numberofelements += objectproplist_element(ctx, buffer, &ofs, maxsize, MTP_PROPERTY_ASSOCIATION_TYPE, handle, &tmp_dword,prop_code);
	numberofelements += objectproplist_element(ctx, buffer, &ofs, maxsize, MTP_PROPERTY_PARENT_OBJECT, handle, &entry->parent,prop_code);
	numberofelements += objectproplist_element(ctx, buffer, &ofs, maxsize, MTP_PROPERTY_OBJECT_SIZE, handle, &entry->size,prop_code);

	tmp_dword = 0x0000;
	numberofelements += objectproplist_element(ctx, buffer, &ofs, maxsize, MTP_PROPERTY_PROTECTION_STATUS, handle, &tmp_dword,prop_code);

	numberofelements += objectproplist_element(ctx, buffer, &ofs, maxsize, MTP_PROPERTY_OBJECT_FILE_NAME, handle, entry->name,prop_code);
	numberofelements += objectproplist_element(ctx, buffer, &ofs, maxsize, MTP_PROPERTY_NAME, handle, entry->name,prop_code);
	numberofelements += objectproplist_element(ctx, buffer, &ofs, maxsize, MTP_PROPERTY_DISPLAY_NAME, handle, 0,prop_code);

	// Date Created (NR) "YYYYMMDDThhmmss.s"
	t = entrystat.st_mtime;
	localtime_r(&t, &lt);
	snprintf(timestr,sizeof(timestr),"%.4d%.2d%.2dT%.2d%.2d%.2d",1900 + lt.tm_year, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);
	numberofelements += objectproplist_element(ctx, buffer, &ofs, maxsize, MTP_PROPERTY_DATE_CREATED, handle, &timestr,prop_code);

	// Date Modified (NR) "YYYYMMDDThhmmss.s"
	t = entrystat.st_mtime;
	localtime_r(&t, &lt);
	snprintf(timestr,sizeof(timestr),"%.4d%.2d%.2dT%.2d%.2d%.2d",1900 + lt.tm_year, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);
	numberofelements += objectproplist_element(ctx, buffer, &ofs, maxsize, MTP_PROPERTY_DATE_MODIFIED, handle, &timestr,prop_code);

	tmp_dword_array[0] = entry->handle;
	tmp_dword_array[1] = entry->parent;
	tmp_dword_array[2] = entry->storage_id;
	tmp_dword_array[3] = 0x00000000;
	numberofelements += objectproplist_element(ctx, buffer, &ofs, maxsize, MTP_PROPERTY_PERSISTENT_UID, handle, &tmp_dword_array,prop_code);

	poke32(buffer, 0, maxsize, numberofelements);   // Number of elements

	return ofs;
}
