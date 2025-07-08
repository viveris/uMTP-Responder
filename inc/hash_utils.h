#ifndef HASH_UTILS_H
#define HASH_UTILS_H

#include "fs_handles_db.h"

int init_hash_node(hash_node *node);
int expand_hash_node(hash_node *node);
uint32_t hash_function_name(const char *name);
uint32_t hash_function_handle(uint32_t handle);
int allocate_pool_block(fs_handles_db *db);
void insert_entry_generic(hash_node *node, fs_entry* entry);
void insert_entry(fs_handles_db *db, fs_entry *entry);
fs_entry *find_entry(fs_handles_db *db, const char *name, uint32_t parent, uint32_t storage_id);

#endif // HASH_UTILS_H