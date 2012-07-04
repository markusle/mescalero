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

#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <sqlite3.h>


typedef std::vector<std::vector<std::string>> queryResult;


/* lightweight wrapper around sqlite3 database */
class DataBase {

public:

  DataBase(std::string databaseName, bool verbose);
  ~DataBase(); 
  
  queryResult query(std::string query);
  
  bool success() { return success_; }
  bool has_table(std::string tableName); 
  void close() { sqlite3_close(db_); }

private:

  sqlite3* db_;
  bool success_;
  bool verbose_;     // if set print error message for queries

  int _open_database(std::string name);
};

#endif
