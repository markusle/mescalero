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


// function declarations 

int getPathsFromDatabase(DataBase& db,
                         std::vector<std::string>& paths);

int updateFile(const char* fpath,
               const struct stat* sb,
               DataBase& db);

int checkFile(std::vector<std::string> referenceValues);

int walkPath(std::string path,
             DataBase& db,
             Action requestType);

int walkPathToUpdate(FTS* fileTree,
                     DataBase& db);

int walkPathToCheck(FTS* fileTree,
                    DataBase& db);

int checkDatabaseAgainstFs(DataBase& db);

int updatePaths(DataBase& db, 
                const std::vector<std::string>& paths);

int appendRemovePaths(DataBase& db,
                      const std::vector<std::string>& paths);

int updateFileProperties(DataBase& db,
                         const std::vector<std::string>& paths);


#endif
