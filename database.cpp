/*
 *
 * (C) 2012 Markus Dittrich
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


#include <iostream>
#include <sqlite3.h>
#include <string>
#include <vector>

#include "database.hpp"

using std::cout;
using std::endl;
using std::string;
using std::vector;


/**********************************************************************
 * 
 * public interface
 * 
 *********************************************************************/
DataBase::DataBase(string databaseName, bool verbose) :
  db_(NULL),
  success_(true),
  verbose_(verbose) {

  if (_open_database(databaseName) != 0) {
    success_ = false;
  }
}

DataBase::~DataBase() {
  close();
  sqlite3_shutdown();
}


/*
 * main workhorse - interacts with database
 */
queryResult DataBase::query(string query) {

  // re-initialize success
  success_ = true;
    
  sqlite3_stmt* statement;
  vector<vector<string>> results;

  if(sqlite3_prepare_v2(db_, query.c_str(), -1, &statement, 0)
     == SQLITE_OK) {
      
    int cols = sqlite3_column_count(statement);
    int result = 0;
      
    while(true) {
      result = sqlite3_step(statement);

      if(result == SQLITE_ROW) {
        vector<string> values;
        for(int col = 0; col < cols; col++) {
          const char* content = reinterpret_cast<const char*>(
            sqlite3_column_text(statement, col));
          if (content) {
            values.push_back(content);
          }
        }
        results.push_back(values);
      } else {
        break;
      }
    }

    sqlite3_finalize(statement);
  }

  string error = sqlite3_errmsg(db_);
  if (verbose_ && error != "not an error") {
    cout << query << " " << error << endl;
    success_ = false;
  }

  return results;
}


/*
 * check if table exists in database; returns true on success
 * and false otherwise
 */ 
bool DataBase::has_table(string tableName) {

  string checkQuery("SELECT 1 FROM ");
  checkQuery += tableName;
  checkQuery += ";";

  queryResult result = query(checkQuery);

  return !result.empty();
}
  


/**********************************************************************
 * 
 * private interface
 * 
 *********************************************************************/

/*
 * open database; return 0 on success and 1 on failure
 */
int DataBase::_open_database(string name) {
  sqlite3_initialize();
  int rc = sqlite3_open_v2(name.c_str(), &db_, SQLITE_OPEN_READWRITE |
                           SQLITE_OPEN_CREATE, NULL);
  if (rc != SQLITE_OK) {
    sqlite3_close(db_);
    return 1;
  }

  return 0;
}

