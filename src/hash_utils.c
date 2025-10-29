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
 * @file   hash_utils.c
 * @brief  Entries names Hash functions.
 * @author Kanstantsin Shabalouski
 */

#include "buildconf.h"
#include <inttypes.h>
#include <stdio.h>

#include "hash_utils.h"
#include "logs_out.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define INITIAL_NODE_CAPACITY 4
#define NODE_GROWTH_FACTOR 2

int init_hash_node(hash_node *node)
{
	node->entries = malloc(INITIAL_NODE_CAPACITY * sizeof(fs_entry*));

	if (!node->entries)
		return 0;

	node->capacity = INITIAL_NODE_CAPACITY;
	node->size = 0;
	return 1;
}

int expand_hash_node(hash_node *node)
{
	uint32_t new_capacity;

	// Integer overflow allocation size check.
	if( node->capacity >= (0x80000000 / sizeof(fs_entry*) ) )
		return 0;

	new_capacity = node->capacity * NODE_GROWTH_FACTOR;

	fs_entry **new_entries = realloc(node->entries, new_capacity * sizeof(fs_entry*));

	if (!new_entries)
		return 0;

	node->entries = new_entries;
	node->capacity = new_capacity;

	return 1;
}

uint32_t hash_function_name(const char *name)
{
	uint32_t hash = 5381;
	int c;

	while ((c = *name++))
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash % HASH_TABLE_SIZE;
}

uint32_t hash_function_handle(uint32_t handle)
{
	uint32_t hash = handle;

	return hash % HASH_TABLE_SIZE;
}

int allocate_pool_block(fs_handles_db *db)
{
	fs_entry_pool_block *new_block = malloc(sizeof(fs_entry_pool_block));

	if (!new_block)
	{
		return 0;
	}

	memset(new_block, 0, sizeof(fs_entry_pool_block));
	new_block->next = db->pool_head;
	db->pool_head = new_block;
	db->pool_free_count += POOL_BLOCK_SIZE;

	return 1;
}

void insert_entry_generic(hash_node *node, fs_entry* entry)
{
	if (node->entries == NULL || node->capacity == 0)
	{
		if (!init_hash_node(node))
		{
			PRINT_ERROR("Failed to initialize hash node");
			return;
		}
	}

	if (node->size >= node->capacity)
	{
		if (!expand_hash_node(node))
		{
			PRINT_ERROR("Failed to expand hash node");
			return;
		}
	}

	node->entries[node->size++] = entry;
}

void insert_entry(fs_handles_db *db, fs_entry *entry)
{
	uint32_t index_name = hash_function_name(entry->name) % HASH_TABLE_SIZE;
	uint32_t index_handle = hash_function_handle(entry->handle) % HASH_TABLE_SIZE;

	hash_node *node_name = &db->hash_table_by_name[index_name];
	hash_node *node_handle = &db->hash_table_by_handle[index_handle];

	insert_entry_generic(node_handle, entry);
	insert_entry_generic(node_name, entry);
}

fs_entry *find_entry(fs_handles_db *db, const char *name, uint32_t parent, uint32_t storage_id)
{
	uint32_t index = hash_function_name(name) % HASH_TABLE_SIZE;
	hash_node *node = &db->hash_table_by_name[index];

	for (uint32_t i = 0; i < node->size; i++)
	{
		if (strcmp(node->entries[i]->name, name) == 0 &&
			node->entries[i]->parent == parent &&
			node->entries[i]->storage_id == storage_id &&
			 ((node->entries[i]->flags & ENTRY_IS_DELETED) == 0))
		{
			return node->entries[i];
		}
	}

	return NULL;
}