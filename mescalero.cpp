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

#include <algorithm>
#include <iterator>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <memory>
#include <cstdio>
#include <vector>
#include <openssl/sha.h>
#include <fts.h>
#include <sys/stat.h>
#include <time.h>
#include <sqlite3.h>
#include <boost/filesystem.hpp>

#include "mescalero.hpp"

using std::cout;
using std::endl;
using std::string;
using std::ifstream;
using std::vector;
using std::shared_ptr;


/* main entry point */
int main(int argc, char** argv) {

  // open database
  DataBase db("test.db");
  if (!db.success()) {
    cout << "Failed to open database" << endl;
    return 1;
  } else {
    db.query("CREATE TABLE IF NOT EXISTS FileTable (name TEXT, hash TEXT)");
  }

  // determine type of request
  string request;
  if (argv[1]) {
    request = string(argv[1]);
  } else {
    request = string("none");
  }
    
  int requestType = -1;
  if (request == "update") {
    requestType = UPDATE_REQUEST;
  } else if (request == "check") {
    requestType = CHECK_REQUEST;
  } else {
    cout << "Error: Unknown request type " << request << endl;
    return 1;
  }
  
  // generate absolute path
  boost::filesystem::path absPath = boost::filesystem::canonical(argv[2]);

  if (walk_path(absPath.string(), db, requestType) != 0) {
    cout << "Error in walk_path" << endl;
    return 1;
  }

  return 0;
}



/* walk directory hierarchy starting at path */
int walk_path(string path, DataBase& db, int requestType) {

  FTS* fileTree;
  FTSENT* file;
  char* paths[] = {(char*)path.c_str(), NULL};

  fileTree = fts_open(paths, FTS_LOGICAL, NULL);
  if (fileTree == NULL) {
    cout << "Error: fts_open failed" << endl;
    return 1;
  }

  // begin transaction
  db.query("BEGIN IMMEDIATE TRANSACTION");
  if (!db.success()) {
    return 1;
  }
  
  while ((file = fts_read(fileTree))) {
    if (file->fts_info == FTS_F) {
      process_file(file->fts_accpath, file->fts_statp, db, requestType);
    }
  }

  // end transaction - if it fails roll it back
  db.query("COMMIT TRANSACTION");
  if (!db.success()) {
    db.query("ROLLBACK TRANSACTION");
  }

  if (errno != 0) {
     cout << "Error: fts_read failed" << endl;
     return 1;
  }

  if (fts_close(fileTree) < 0) {
     cout << "Error: fts_read failed" << endl;
     return 1;
  }

  return 0;
}
  


/* process each file in the filetree walk */
int process_file(const char *fpath, const struct stat *sb,
                 DataBase& db, int requestType) {

  string fileName(fpath);
  ifstream file(fileName);
  if (!file) {
    return 1;
  }
    
  sha256Hash hash = encode_as_sha256(file);
  //print_hash(fileName, hash);

  std::ostringstream request;
  string hashString;
  hash_to_string(hash, hashString);

  if (requestType == UPDATE_REQUEST) {
    request << "INSERT INTO FileTable (name, hash) VALUES("
            << "'" << fileName << "', '" << hashString << "');";
    db.query(request.str());
  } 
    


    
  //cout << "m_time " << ctime(&sb->st_mtime) << endl;
  return 0; 
}


/* helper function to print out a file's hash */
void print_hash(string fileName, sha256Hash hash) {

  cout << fileName << ":  ";
  for (unsigned char &item : *hash) {
    printf("%02x", item);
  }
    
  cout << endl;
}


void hash_to_string(sha256Hash hash, string& hashString) {

  hashString.clear();

  char c[3];
  for (unsigned char &item : *hash) {
    sprintf(c, "%02x", item);
    hashString += c;
  }
}
 


/* function computing the sha256 hash of the file
 * reference by ifstream file */
sha256Hash encode_as_sha256(ifstream &file) {
  
  // initialize OpenSSL
  SHA256_CTX context;
  unsigned char md[SHA256_DIGEST_LENGTH];
  SHA256_Init(&context);
    
  // process file
  char buf[BUFSIZ];
  while (file) {
    file.read(buf, BUFSIZ);
    SHA256_Update(&context, buf, file.gcount()); 
  }

  SHA256_Final(md, &context);

  sha256Hash hash(new vector<unsigned char>);
  std::copy(md, md+SHA256_DIGEST_LENGTH, std::back_inserter(*hash));
  
  return hash;
}



/*********************************************************************
 * 
 * member defitions for Database class
 * 
 ********************************************************************/
DataBase::DataBase(string databaseName) :
  db_(NULL),
  success_(true) {

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
vector<vector<string>> DataBase::query(string query) {

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
  if (error != "not an error") {
    cout << query << " " << error << endl;
    success_ = false;
  }

  return results;
}

  
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

