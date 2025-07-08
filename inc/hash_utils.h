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
 * @file   hash_utils.h
 * @brief  Entries names hash functions.
 * @author Kanstantsin Shabalouski
 */

#ifndef _INC_HASH_UTILS_H_
#define _INC_HASH_UTILS_H_

#include "fs_handles_db.h"

int init_hash_node(hash_node *node);
int expand_hash_node(hash_node *node);
uint32_t hash_function_name(const char *name);
uint32_t hash_function_handle(uint32_t handle);
int allocate_pool_block(fs_handles_db *db);
void insert_entry_generic(hash_node *node, fs_entry* entry);
void insert_entry(fs_handles_db *db, fs_entry *entry);
fs_entry *find_entry(fs_handles_db *db, const char *name, uint32_t parent, uint32_t storage_id);

#endif // _INC_HASH_UTILS_H_