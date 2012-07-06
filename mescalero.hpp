/*
 *
 * (c) 2012 Markus Dittrich
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License Version 3 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License Version 3 for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef MESCALERO_HPP
#define MESCALERO_HPP

#include <fts.h>

#include "database.hpp"
#include "cmdlineParser.hpp"


const std::string VERSION = "0.1";


/* function declarations */

int get_paths_from_database(DataBase& db,
                            std::vector<std::string>& paths);

int update_file(const char* fpath,
                const struct stat* sb,
                DataBase& db);

int check_file(std::vector<std::string> referenceValues);

int walk_path(std::string path,
              DataBase& db,
              actionToggle requestType);

int walk_path_to_update(FTS* fileTree,
                        DataBase& db);

int walk_path_to_check(FTS* fileTree,
                       DataBase& db);

int check_database_against_fs(DataBase& db);

int update_paths(DataBase& db, 
                 const std::vector<std::string>& paths);

int update_file_properties(DataBase& db,
                           const std::vector<std::string>& paths);


#endif
