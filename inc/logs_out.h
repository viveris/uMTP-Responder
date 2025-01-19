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
 * @file   logs_out.h
 * @brief  Log output functions
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#ifndef _INC_DEBUG_OUT_H_
#define _INC_DEBUG_OUT_H_

void timestamp(char * timestr, int maxsize);

#define SIZEHEX PRIx64

#ifdef USE_SYSLOG

#include <syslog.h>

#else

#ifdef DEBUG
#include <stdio.h>
#endif

#endif


#ifdef USE_SYSLOG // Syslog usage

#define PRINT_MSG(fmt, args...)     syslog(LOG_NOTICE, "[uMTPrd - Info] " fmt "\n", \
										## args)
#define PRINT_ERROR(fmt, args...)   syslog(LOG_ERR, "[uMTPrd - Error] " fmt "\n", \
										## args)
#define PRINT_WARN(fmt, args...)    syslog(LOG_WARNING, "[uMTPrd - Warning] " fmt "\n", \
										## args)
#ifdef DEBUG

#define PRINT_DEBUG(fmt, args...)   syslog(LOG_DEBUG, "[uMTPrd - Debug] " fmt "\n",  \
										## args)
#else

#define PRINT_DEBUG(fmt, args...)

#endif

#else // Stdout usage

#define PRINT_MSG(fmt, args...)     do {                                \
										char timestr[32];               \
										timestamp((char*)&timestr, sizeof(timestr)); \
										fprintf(stdout,                 \
											"[uMTPrd - %s - Info] " fmt "\n",(char*)&timestr, \
											## args);                   \
										fflush(stdout);                 \
									} while (0)

#define PRINT_ERROR(fmt, args...)   do {                                \
										char timestr[32];               \
										timestamp((char*)&timestr, sizeof(timestr)); \
										fprintf(stderr,                 \
											"[uMTPrd - %s - Error] " fmt "\n",(char*)&timestr, \
											## args);                   \
										fflush(stderr);                 \
									} while (0)

#define PRINT_WARN(fmt, args...)    do {                                \
										char timestr[32];               \
										timestamp((char*)&timestr, sizeof(timestr)); \
										fprintf(stdout,                 \
											"[uMTPrd - %s - Warning] " fmt "\n",(char*)&timestr, \
											## args);                   \
										fflush(stdout);                 \
									} while (0)

#ifdef DEBUG
#define PRINT_DEBUG(fmt, args...)   do {                                \
										char timestr[32];               \
										timestamp((char*)&timestr, sizeof(timestr)); \
										fprintf(stdout,                 \
											"[uMTPrd - %s - Debug] " fmt "\n",(char*)&timestr, \
											## args);                   \
										fflush(stdout);                 \
									} while (0)
#else

#define PRINT_DEBUG(fmt, args...)

#endif

#endif

#ifdef DEBUG

#define PRINT_DEBUG_BUF(x, y) printbuf( x, y );
void printbuf(void * buf,int size);

#else

#define PRINT_DEBUG_BUF(x, y)

#endif

#endif

