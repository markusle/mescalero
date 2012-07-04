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

#include <assert.h>
#include <boost/filesystem.hpp>
#include <cstdio>
#include <fstream>
#include <fts.h>
#include <iostream>
#include <openssl/sha.h>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <vector>

#include "database.hpp"
#include "mescalero.hpp"
#include "misc.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::ifstream;
using std::vector;

/* hardcoded path to database for now */
const std::string DATABASE_PATH = "test.db";


/* main entry point */
int main(int argc, char** argv) {

  // parse command line
  if (argc <= 1)
  {
    usage();
    return 1;
  }

  // check command line arguments
  actionToggle action = NONE;
  int c;
  while ((c = getopt(argc,argv, "uc") ) != -1 )
  {
    switch (c)
    {
        case 'u':
          action = UPDATE_REQUEST;
          break;
        case 'c':
          action = CHECK_REQUEST;
          break;
        case '?':
          usage();
          break;
        case ':':
          usage();
          break;
        default:
          usage();
          break;
    }
  }

  // make sure user specified one of u or c
  if (action == NONE)
  {
    cerr << "Error: Please specify at least on of -u or -c" << endl;
    usage();
    return 1;
  }

  // open database
  DataBase db(DATABASE_PATH, true);
  if (!db.success())
  {
    cerr << "Failed to open database" << endl;
    return 1;
  }

  // grab path to use for updating and checking
  // if one exists
  std::string path;
  if (db.has_table("ConfigTable"))
  {
    path = get_path_from_database(db);
    if (path.empty()) {
      return 1;
    }
  }
 
  if (action == UPDATE_REQUEST)
  {
    // see if user supplied name on command line
    // if not grab name from database
    if (argc >= 3)
    {
      boost::filesystem::path absPath =
        boost::filesystem::canonical(argv[optind]);
      path = absPath.string();
      db.query("DROP TABLE IF EXISTS ConfigTable;");
      db.query("CREATE TABLE ConfigTable (path TEXT);");
      db.query("INSERT INTO ConfigTable (path) VALUES (\"" + path + "\");");
    }

    // erase previous table and start a new one
    // FIXME: In principle we could just update the entries here.
    //        The question is if this is faster than just creating
    //        it from scratch.
    db.query("DROP TABLE IF EXISTS FileTable;");
    db.query("CREATE TABLE FileTable "
             "(name TEXT, hash TEXT, uid TEXT, gid TEXT, "
             "mode TEXT, size TEXT, mtime TEXT, ctime TEXT)");
  }

  // if we don't have a path at this point the user should have
  // specified one
  if (path.empty())
  {
    cerr << "Error: Please specify a path to a file tree for updating\n"
         << "       the database." << endl;
    return 1;
  }

  // walk path
  if (walk_path(path, db, action) != 0) {
    cout << "Error in walk_path" << endl;
    return 1;
  }

  return 0;
}



/*
 * query the database for the filesystem path covered by
 * the content.
 */
string get_path_from_database(DataBase& db) {

  string path;
  queryResult result = db.query("SELECT path FROM ConfigTable;");
  if (result.size() != 1)
  {
    cerr << "Error: Trouble retrieving file path" << endl;
    return path;
  }

  if (result[0].size() != 1)
  {
    cerr << "Error: Trouble retrieving file path" << endl;
    return path;
  }

  return result[0][0];
}

    

/* walk directory hierarchy starting at path */
int walk_path(string path, DataBase& db, actionToggle requestType) {

  FTS* fileTree;
  FTSENT* file;
  char* paths[] = {(char*)path.c_str(), NULL};

  fileTree = fts_open(paths, FTS_LOGICAL, NULL);
  if (fileTree == NULL) {
    cout << "Error: fts_open failed" << endl;
    return 1;
  }


  // updates can be transactioned
  if (requestType == UPDATE_REQUEST) {

    // begin transaction
    db.query("BEGIN IMMEDIATE TRANSACTION");
    if (!db.success()) {
      return 1;
    }
  
    while ((file = fts_read(fileTree))) {
      if (file->fts_info == FTS_F) {
        update_file(file->fts_accpath, file->fts_statp, db);
      }
    }


    // end transaction - if it fails roll it back
    db.query("COMMIT TRANSACTION");
    if (!db.success())
    {
      db.query("ROLLBACK TRANSACTION");
    }
  }
  else if (requestType == CHECK_REQUEST)
  {
    while ((file = fts_read(fileTree)))
    {
      if (file->fts_info == FTS_F)
      {
        if (check_file(file->fts_accpath, file->fts_statp, db))
        {
          cerr << "Error: " << file->fts_accpath << " not in database";
        }
      }
    }
  }

  if (errno != 0)
  {
     cout << "Error: fts_read failed" << endl;
     return 1;
  }

  if (fts_close(fileTree) < 0)
  {
     cout << "Error: fts_read failed" << endl;
     return 1;
  }

  return 0;
}
  


/* process each file in the filetree walk and update its information
 * in the database */
int update_file(const char *fpath, const struct stat *sb, DataBase &db) {

  string fileName(fpath);
  ifstream file(fileName);
  if (!file) {
    return 1;
  }
     
  sha256Hash hash = hash_as_sha256(file);
  string hashString;
  hash_to_string(hash, hashString);

  std::ostringstream request;
  request << "INSERT INTO FileTable (name, hash, uid, gid, mode, "
             "size, mtime, ctime) VALUES("
          << "\"" << fileName << "\", \"" << hashString << "\", \""
          << sb->st_uid << "\", \"" << sb->st_gid << "\", \""
          << sb->st_mode << "\", \"" << sb->st_size << "\", \""
          << sb->st_mtime << "\", \"" << sb->st_ctime << "\");";
  db.query(request.str());
    
  return 0; 
}



/* process each file in the filetree walk and check its information
 * against the data in the database */
int check_file(const char *fpath,
               const struct stat *sb,
               DataBase &db) {

  string fileName(fpath);
  std::ostringstream request;
  request << "SELECT * FROM FileTable where name = \""
          << fileName << "\"";
  vector<vector<string>> result = db.query(request.str());

  /* if we get none or more than a single result we're in trouble */
  if (result.empty()) {
    err_msg(fileName + " not in database.");
    return 1;
  } else if (result.size() != 1) {
    err_msg("multiple results returned for file " + fileName);
    return 1;
  }

  /* make sure we have valid query result */
  vector<string> testResult = result[0];
  if (testResult.size() != 8) {
    err_msg("incomplete query from database. ");
  }

  /* check contents */
  ifstream file(fileName);
  if (!file) {
    return 1;
  }
  sha256Hash hash = hash_as_sha256(file);
  string hashString;
  hash_to_string(hash, hashString);

  // all checks out now look at the result and print error if needed
  check_and_print_result(hashString, testResult, fileName, sb);
 
  return 0; 
}

