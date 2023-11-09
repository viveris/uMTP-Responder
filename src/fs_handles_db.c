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
 * @file   fs_handles_db.c
 * @brief  Local file system helpers and handles database management.
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include "buildconf.h"

#include <inttypes.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>

#include "mtp.h"
#include "mtp_helpers.h"

#include "fs_handles_db.h"
#include "inotify.h"
#include "logs_out.h"

int fs_remove_tree( char *folder )
{
	struct dirent *d;
	DIR * dir;
	struct stat64 fileStat;
	char * tmpstr;
	int del_fail;

	del_fail = 0;

	dir = opendir (folder);
	if( dir )
	{
		do
		{
			d = readdir (dir);
			if( d )
			{
				tmpstr = malloc (strlen(folder) + strlen(d->d_name) + 4 );
				if( tmpstr )
				{
					strcpy(tmpstr,folder);
					strcat(tmpstr,"/");
					strcat(tmpstr,d->d_name);

					memset(&fileStat,0,sizeof(struct stat64));
					if( !lstat64 (tmpstr, &fileStat) )
					{
						if ( S_ISDIR ( fileStat.st_mode ) )
						{
							if( strcmp(d->d_name,"..") && strcmp(d->d_name,".") )
							{
								if( fs_remove_tree(tmpstr) )
									del_fail = 1;
							}
						}
						else
						{
							if( remove(tmpstr) )
								del_fail = 1;
						}
					}

					free(tmpstr);
				}
				else
				{
					del_fail = 1;
				}
			}
		}while(d);

		closedir(dir);

		if( remove(folder) )
			del_fail = 1;
	}
	else
	{
		del_fail = 1;
	}

	return del_fail;
}

int fs_entry_stat(char *path, filefoundinfo* fileinfo)
{
	struct stat64 fileStat;
	int i;

	memset(&fileStat,0,sizeof(struct stat64));
	if( !stat64 (path, &fileStat) )
	{
		if ( S_ISDIR ( fileStat.st_mode ) )
			fileinfo->isdirectory = 1;
		else
			fileinfo->isdirectory = 0;

		fileinfo->size = fileStat.st_size;

		i = strlen(path);
		while( i )
		{
			if( path[i] == '/' )
			{
				i++;
				break;
			}
			i--;
		}

		strncpy(fileinfo->filename,&path[i],FS_HANDLE_MAX_FILENAME_SIZE);
		fileinfo->filename[FS_HANDLE_MAX_FILENAME_SIZE] = '\0';

		return 1;
	}

	return 0;
}

DIR * fs_find_first_file(char *folder, filefoundinfo* fileinfo)
{
	struct dirent *d;
	DIR * dir;
	char * tmpstr;

	dir = opendir (folder);
	if( dir )
	{
		d = readdir (dir);
		if( d )
		{
			tmpstr = malloc (strlen(folder) + strlen(d->d_name) + 4 );
			if( tmpstr )
			{
				strcpy(tmpstr,folder);
				strcat(tmpstr,"/");
				strcat(tmpstr,d->d_name);

				if( fs_entry_stat(tmpstr, fileinfo) )
				{
					free(tmpstr);
					return (void*)dir;
				}

				free(tmpstr);
			}
		}

		closedir(dir);
		dir = NULL;
	}

	return dir;
}

int fs_find_next_file(DIR* dir, char *folder, filefoundinfo* fileinfo)
{
	int ret;
	struct dirent *d;
	char * tmpstr;

	if(!dir)
		return 0;

	d = readdir (dir);

	ret = 0;

	if( d )
	{
		tmpstr = malloc (strlen(folder) + strlen(d->d_name) + 4 );
		if( tmpstr )
		{
			strcpy(tmpstr,folder);
			strcat(tmpstr,"/");
			strcat(tmpstr,d->d_name);

			if( fs_entry_stat( tmpstr, fileinfo ) )
			{
				ret = 1;
				free(tmpstr);
				return ret;
			}

			free(tmpstr);
		}
	}

	return ret;
}

int fs_find_close(DIR* dir)
{
	if( dir )
		closedir( dir );

	return 0;
}

fs_handles_db * init_fs_db(void * mtp_ctx)
{
	fs_handles_db * db;

	PRINT_DEBUG("init_fs_db called");

	db = (fs_handles_db *)malloc(sizeof(fs_handles_db));
	if( db )
	{
		memset(db,0,sizeof(fs_handles_db));
		db->next_handle = 0x00000001;
		db->mtp_ctx = mtp_ctx;
	}

	return db;
}

void deinit_fs_db(fs_handles_db * fsh)
{
	fs_entry * next_entry;

	PRINT_DEBUG("deinit_fs_db called");

	if( fsh )
	{
		while( fsh->entry_list )
		{
			next_entry = fsh->entry_list->next;

			if( fsh->entry_list->watch_descriptor != -1 )
			{
				// Disable the inotify watch point
				inotify_handler_rmwatch( fsh->mtp_ctx, fsh->entry_list->watch_descriptor );
				fsh->entry_list->watch_descriptor = -1;
			}

			if( fsh->entry_list->name )
				free( fsh->entry_list->name );

			free( fsh->entry_list );

			fsh->entry_list = next_entry;
		}

		free(fsh);
	}

	return;
}

fs_entry * search_entry(fs_handles_db * db, filefoundinfo *fileinfo, uint32_t parent, uint32_t storage_id)
{
	fs_entry * entry_list;

	if( !db )
		return NULL;

	entry_list = db->entry_list;

	while( entry_list )
	{
		if( !( entry_list->flags & ENTRY_IS_DELETED ) && ( entry_list->parent == parent ) && ( entry_list->storage_id == storage_id ) && entry_list->name )
		{
			if( !strcmp(entry_list->name,fileinfo->filename) )
			{
				return entry_list;
			}
		}

		entry_list = entry_list->next;
	}

	return NULL;
}

fs_entry * alloc_entry(fs_handles_db * db, filefoundinfo *fileinfo, uint32_t parent, uint32_t storage_id)
{
	fs_entry * entry;

	entry = malloc(sizeof(fs_entry));
	if( entry )
	{
		memset(entry,0,sizeof(fs_entry));

		entry->handle = db->next_handle;
		db->next_handle++;
		entry->parent = parent;
		entry->storage_id = storage_id;

		entry->name = malloc(strlen(fileinfo->filename)+1);
		if( entry->name )
		{
			strcpy(entry->name,fileinfo->filename);
		}

		entry->size = fileinfo->size;

		entry->watch_descriptor = -1;

		if( fileinfo->isdirectory )
			entry->flags = ENTRY_IS_DIR;
		else
			entry->flags = 0x00000000;

		entry->next = db->entry_list;

		db->entry_list = entry;
	}

	return entry;
}

fs_entry * alloc_root_entry(fs_handles_db * db, uint32_t storage_id)
{
	fs_entry * entry;

	if( !db )
		return NULL;

	entry = malloc(sizeof(fs_entry));
	if( entry )
	{
		memset(entry,0,sizeof(fs_entry));

		entry->handle = 0x00000000;
		entry->parent = 0x00000000;
		entry->storage_id = storage_id;

		entry->name = malloc(strlen("/")+1);
		if( entry->name )
		{
			strcpy(entry->name,"/");
		}

		entry->size = 1;

		entry->watch_descriptor = -1;

		entry->flags = ENTRY_IS_DIR;

		entry->next = db->entry_list;

		db->entry_list = entry;
	}

	return entry;
}


fs_entry * add_entry(fs_handles_db * db, filefoundinfo *fileinfo, uint32_t parent, uint32_t storage_id)
{
	fs_entry * entry;

	entry = search_entry(db, fileinfo, parent, storage_id);
	if( entry )
	{
		// entry already there...
		PRINT_DEBUG("add_entry : File already present (%s)",fileinfo->filename);
	}
	else
	{
		// add the entry
		PRINT_DEBUG("add_entry : File not present - add entry (%s)",fileinfo->filename);

		entry = alloc_entry( db, fileinfo, parent, storage_id);
	}

	return entry;
}

int scan_and_add_folder(fs_handles_db * db, char * base, uint32_t parent, uint32_t storage_id)
{
	fs_entry * entry;
	char * path;
	DIR* dir;
	int ret;
	filefoundinfo fileinfo;
	struct stat64 entrystat;

	PRINT_DEBUG("scan_and_add_folder : %s, Parent : 0x%.8X, Storage ID : 0x%.8X",base,parent,storage_id);

	dir = fs_find_first_file(base, &fileinfo);
	if( dir )
	{
		do
		{
			PRINT_DEBUG("---------------------");
			PRINT_DEBUG("File : %s",fileinfo.filename);
			PRINT_DEBUG("Size : 0x%"SIZEHEX,fileinfo.size);
			PRINT_DEBUG("IsDir: %d",fileinfo.isdirectory);
			PRINT_DEBUG("---------------------");

			if( strcmp(fileinfo.filename,"..") && strcmp(fileinfo.filename,".") && \
				(((mtp_ctx *)db->mtp_ctx)->usb_cfg.show_hidden_files || fileinfo.filename[0] != '.') )
			{
				add_entry(db, &fileinfo, parent, storage_id);
			}

		}while(fs_find_next_file(dir, base, &fileinfo));
		fs_find_close(dir);
	}

	// Scan the DB to find and remove deleted files...
	init_search_handle(db, parent, storage_id);
	do
	{
		entry = get_next_child_handle(db);
		if(entry)
		{
			path = build_full_path(db, mtp_get_storage_root(db->mtp_ctx, entry->storage_id), entry);

			if(path)
			{
				ret = stat64(path, &entrystat);
				if(ret)
				{
					PRINT_DEBUG("scan_and_add_folder : discard entry %s - stat error", path);
					entry->flags |= ENTRY_IS_DELETED;
					if( entry->watch_descriptor != -1 )
					{
						inotify_handler_rmwatch( db->mtp_ctx, entry->watch_descriptor );
						entry->watch_descriptor = -1;
					}
				}
				else
				{
					entry->size = entrystat.st_size;
				}

				free(path);
			}
		}
	}while(entry);

	if(dir)
		return 0;
	else
		return -1;
}

fs_entry * init_search_handle(fs_handles_db * db, uint32_t parent, uint32_t storage_id)
{
	db->search_entry = db->entry_list;
	db->handle_search = parent;
	db->storage_search = storage_id;

	return db->search_entry;
}

fs_entry * get_next_child_handle(fs_handles_db * db)
{
	fs_entry * entry_list;

	entry_list = db->search_entry;

	while( entry_list )
	{
		if( !( entry_list->flags & ENTRY_IS_DELETED ) && ( entry_list->parent == db->handle_search ) && ( entry_list->storage_id == db->storage_search ) && ( entry_list->handle != entry_list->parent ) )
		{
			db->search_entry = entry_list->next;

			return entry_list;
		}

		entry_list = entry_list->next;
	}

	db->search_entry = 0x00000000;

	return NULL;
}

fs_entry * get_entry_by_handle(fs_handles_db * db, uint32_t handle)
{
	fs_entry * entry_list;

	entry_list = db->entry_list;

	while( entry_list )
	{
		if( !( entry_list->flags & ENTRY_IS_DELETED ) && ( entry_list->handle == handle ) )
		{
			if( mtp_get_storage_root(db->mtp_ctx, entry_list->storage_id) )
			{
				return entry_list;
			}
		}

		entry_list = entry_list->next;
	}

	return 0;
}

fs_entry * get_entry_by_handle_and_storageid(fs_handles_db * db, uint32_t handle, uint32_t storage_id)
{
	fs_entry * entry_list;

	entry_list = db->entry_list;

	while( entry_list )
	{
		if( !( entry_list->flags & ENTRY_IS_DELETED ) && ( entry_list->handle == handle ) && ( entry_list->storage_id == storage_id ) )
		{
			return entry_list;
		}
		entry_list = entry_list->next;
	}

	return NULL;
}

char * build_full_path(fs_handles_db * db,char * root_path,fs_entry * entry)
{
	int totallen,namelen;
	fs_entry * curentry;
	char * full_path;
	int full_path_offset;

	full_path = NULL;

	if( !entry )
		return full_path;

	curentry = entry;
	totallen = 0;
	do
	{
		if(curentry->name)
			totallen += strlen(curentry->name);

		totallen++; // '/'

		if( curentry->parent && ( curentry->parent != 0xFFFFFFFF ) )
		{
			curentry = get_entry_by_handle(db, curentry->parent);
		}
		else
		{
			curentry = NULL;
		}

	}while( curentry );

	if(root_path)
	{
		totallen += strlen(root_path);
	}

	full_path = malloc(totallen+1);
	if( full_path )
	{
		memset(full_path,0,totallen+1);
		full_path_offset = totallen;
		curentry = entry;

		do
		{
			if(curentry->name)
				namelen = strlen(curentry->name);
			else
				namelen = 0;

			full_path_offset -= namelen;

			if( namelen )
				memcpy(&full_path[full_path_offset],curentry->name,namelen);

			full_path_offset--;

			full_path[full_path_offset] = '/';

			if(curentry->parent && curentry->parent!=0xFFFFFFFF)
			{
				curentry = get_entry_by_handle(db, curentry->parent);
			}
			else
			{
				curentry = NULL;
			}
		}while(curentry);

		if(root_path)
		{
			memcpy(&full_path[0],root_path,strlen(root_path));
		}

		if(entry->name)
			PRINT_DEBUG("build_full_path : %s -> %s",entry->name, full_path);
	}

	return full_path;
}

int entry_open(fs_handles_db * db, fs_entry * entry)
{
	int file;
	char * full_path;

	file = -1;

	full_path = build_full_path(db,mtp_get_storage_root(db->mtp_ctx, entry->storage_id), entry);
	if( full_path )
	{
		file = -1;

		if(!set_storage_giduid(db->mtp_ctx, entry->storage_id))
		{
			file = open(full_path,O_RDONLY | O_LARGEFILE);
		}

		restore_giduid(db->mtp_ctx);

		if( file == -1 )
			PRINT_DEBUG("entry_open : Can't open %s !",full_path);

		free(full_path);
	}

	return file;
}

int entry_read(fs_handles_db * db, int file, unsigned char * buffer_out, mtp_offset offset, mtp_size size)
{
	int totalread;

	if( file != -1 )
	{
		lseek64(file, offset, SEEK_SET);

		totalread = read( file, buffer_out, size );

		return totalread;
	}

	return 0;
}

void entry_close(int file)
{
	if( file != -1 )
		close(file);
}

fs_entry * get_entry_by_wd( fs_handles_db * db, int watch_descriptor, fs_entry * entry_list )
{
	if(!entry_list && db)
		entry_list = db->entry_list;

	while( entry_list )
	{
		if( !( entry_list->flags & ENTRY_IS_DELETED ) && ( entry_list->watch_descriptor == watch_descriptor ) )
		{
			return entry_list;
		}

		entry_list = entry_list->next;
	}

	return NULL;
}

fs_entry * get_entry_by_storageid( fs_handles_db * db, uint32_t storage_id, fs_entry * entry_list )
{
	if(!entry_list && db)
		entry_list = db->entry_list;

	while( entry_list )
	{
		if( !( entry_list->flags & ENTRY_IS_DELETED ) && ( entry_list->storage_id == storage_id ) )
		{
			return entry_list;
		}

		entry_list = entry_list->next;
	}

	return NULL;
}
