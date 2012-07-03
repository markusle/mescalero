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


/* macros for type of request */
enum actionToggle {NONE, UPDATE_REQUEST, CHECK_REQUEST};


/* lightweight wrapper around sqlite3 database */
class DataBase {

public:

  DataBase(std::string databaseName);
  ~DataBase(); 
  
  std::vector<std::vector<std::string>> query(std::string query);
  
  bool success() { return success_; }
  void close() { sqlite3_close(db_); }

private:

  sqlite3* db_;
  bool success_;

  int _open_database(std::string name);
};


/* function declarations */

int update_file(const char *fpath,
                const struct stat *sb,
                DataBase &db);

int check_file(const char *fpath,
               const struct stat *sb,
               DataBase &db);

int walk_path(std::string path,
              DataBase &db,
              actionToggle requestType);



#endif
