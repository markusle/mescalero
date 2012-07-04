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
#include <unordered_set>
#include <vector>

#include "database.hpp"
#include "mescalero.hpp"
#include "misc.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::ifstream;
using std::unordered_set;
using std::vector;

/* hardcoded path to database for now */
const std::string DATABASE_PATH = "test.db";

/* intervall at which transactions are committed */
const int COMMIT_INTERVAL = 20000;


/*
 * main entry point
 */
int main(int argc, char** argv) {

  // parse command line
  if (argc <= 2)
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
    err_msg("Please specify at least on of -u or -c");
    usage();
    return 1;
  }

  // open database
  DataBase db(DATABASE_PATH, argv[2], true);
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
    if (argc >= 4)
    {
      boost::filesystem::path absPath =
        boost::filesystem::canonical(argv[3]);
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

    // if we don't have a path at this point the user should have
    // specified one
    if (path.empty())
    {
      err_msg("Please specify a path to a file tree for updating\n"
              "       the database.");
      return 1;
    }

    if (walk_path(path, db, action) != 0) {
      err_msg("Error occured in walk_path");
      return 1;
    }
  }
  else if (action == CHECK_REQUEST)
  {
    // if we don't have a path at this point the user should have
    // specified one
    if (path.empty())
    {
      err_msg("Please specify a path to a file tree for updating\n"
              "the database");
      return 1;
    }

    // check properties of all known files
    check_database_against_fs(db);
    
    // check if any files present are not in database
    if (walk_path(path, db, action) != 0) {
      cout << "Error in walk_path" << endl;
      return 1;
    }
  }

  return 0;
}


/*
 * this function goes through all entries in the database
 * and compares them with the current content of the database
 *
 */
int check_database_against_fs(DataBase& db)
{
  queryResult results(db.query("SELECT * from FileTable"));
  for (vector<string>& result : results) {
    
    if (result.size() != 8) {
      err_msg("incomplete query from database. ");
    }

    if (check_file(result) != 0)
    {
      cout << "Warning: " << result[0] << " has disappeared. \n" << endl;
    }
  }

  return 0;
}



/*
 * query the database for the filesystem path covered by
 * the content.
 */
string get_path_from_database(DataBase& db)
{

  string path;
  queryResult result = db.query("SELECT path FROM ConfigTable;");
  if (result.size() != 1)
  {
    err_msg("Trouble retrieving file path");
    return path;
  }

  if (result[0].size() != 1)
  {
    err_msg("Trouble retrieving file path");
    return path;
  }

  return result[0][0];
}

    

/* walk directory hierarchy starting at path */
int walk_path(string path, DataBase& db, actionToggle requestType) {

  FTS* fileTree;
  char* paths[] = {(char*)path.c_str(), NULL};

  int options = FTS_LOGICAL;
  if (requestType == CHECK_REQUEST) {
    options |= FTS_NOSTAT;
  }

  fileTree = fts_open(paths, options, NULL);
  if (fileTree == NULL) {
    err_msg("fts_open failed");
    return 1;
  }

  // walk tree depending on request
  if (requestType == UPDATE_REQUEST)
  {
    if (walk_path_to_update(fileTree, db) != 0)
    {
      return 1;
    }
  }
  else if (requestType == CHECK_REQUEST)
  {
   if (walk_path_to_check(fileTree, db) != 0)
    {
      return 1;
    }
  }

  if (errno != 0)
  {
    err_msg("fts_read failed");
    return 1;
  }

  if (fts_close(fileTree) < 0)
  {
    err_msg("fts_read failed");
    return 1;
  }

  return 0;
}
  

/*
 * walk file tree given by FTS and update the database
 */
int walk_path_to_update(FTS* fileTree, DataBase& db)
{
  // begin transaction
  db.query("BEGIN IMMEDIATE TRANSACTION");
  if (!db.success()) {
    return 1;
  }

  long counter = 0;
  FTSENT* file;
  while ((file = fts_read(fileTree))) {

    // dump transactions every COMMIT_INTERVAL
    ++counter;
    if (counter % COMMIT_INTERVAL == 0) {
      
      db.query("COMMIT TRANSACTION");
      if (!db.success()) {
        db.query("ROLLBACK TRANSACTION");
      }
      
      db.query("BEGIN IMMEDIATE TRANSACTION");
      if (!db.success()) {
        return 1;
      }
    }
        
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

  return 0;
}



/*
 * walk file tree given by FTS and check if any
 * entries are not in database
 *
 * NOTE: It is very inefficient to walk through
 * the database entry by entry. Instead we dump
 * the whole content (filenames only) into an
 * unordered_set and look through it.
 */
int walk_path_to_check(FTS* fileTree, DataBase& db)
{
  FTSENT* file;

  queryResult result(db.query("SELECT * FROM FileTable;"));
  unordered_set<string> allFiles;
  for(vector<string>& item : result) {
    if (item.size() != 8) {
      continue;
    }

    allFiles.emplace(item[0]);
  }

  while ((file = fts_read(fileTree)))
  {
    if (file->fts_info == FTS_F)
    {
      if (allFiles.find(file->fts_accpath) == allFiles.end())
      {
        cerr << "Warning: " << file->fts_accpath << " not in database\n"
             << endl;
      }
    }
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


/*
 * compare reference file properties against the current values on
 * the file system
 */
int check_file(std::vector<std::string> referenceValues)
{
  string fileName = referenceValues[0];
  
  // check contents 
  ifstream file(fileName);
  if (!file) {
    return 1;
  }
  
  sha256Hash hash = hash_as_sha256(file);
  string hashString;
  hash_to_string(hash, hashString);

  // stat the file
  struct stat sb;
  if (stat(fileName.c_str(), &sb) != 0) {
    return 1;
  }
  
  // all checks out now look at the result and print error if needed
  check_and_print_result(hashString, referenceValues, fileName, &sb);
 
  return 0; 
}

