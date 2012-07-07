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
#include <iostream>
#include <openssl/sha.h>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <unordered_set>
#include <vector>

#include "cmdlineParser.hpp"
#include "mescalero.hpp"
#include "misc.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::ifstream;
using std::unordered_set;
using std::vector;



// intervall at which transactions are committed */
const int COMMIT_INTERVAL = 20000;


//
// main entry point
//
int 
main(int argc, char** argv) 
{
  CmdLineOpts options;
  if (parseCommandline(argc, argv, options) != 0) {
    return 1;
  }

  // open database
  DataBase db(options.dataBasePath, options.password, true);
  if (!db.success()) {
    cerr << "Failed to open database" << endl;
    return 1;
  }
  
  // nuke password from memory
  options.password.clear();

  // update paths to be check if requested or grab them from
  // the database
  vector<string> paths;
  if (options.action == UPDATE_PATH_REQUEST) {
    updatePaths(db, options.pathList);
    return 0;
  } 
  else {
    if (db.hasTable("ConfigTable")) {
      if (getPathsFromDatabase(db, paths) != 0) {
        errMsg("Database does not contain path for scanning.");
        return 1;
      }
    }
    else {
      errMsg("No paths for file checking defined.");
      return 1;
    }
  }

  if (options.action == LIST_PATH_REQUEST) {
    cout << "Currently active paths: \n\n";

    for (string& val : paths) {
      cout << val << endl;
    }

    cout << endl;
  }
  else if (options.action == UPDATE_FILE_REQUEST) {
    if (updateFileProperties(db, paths) != 0) {
      return 1;
    }
  }
  else if (options.action == CHECK_REQUEST) {
    // check properties of all known files
    checkDatabaseAgainstFs(db);
    
    // check if any files present are not in database
    for (string &path : paths) {
      if (walkPath(path, db, options.action) != 0) {
        cout << "Error in walkPath" << endl;
        return 1;
      }
    }
  }

  return 0;
}



//
// this function goes through all entries in the database
// and compares them with the current content of the database
//
int 
checkDatabaseAgainstFs(DataBase& db)
{
  QueryResult results(db.query("SELECT * from FileTable"));
  for (vector<string>& result : results) {
    
    if (result.size() != 8) {
      errMsg("incomplete query from database. ");
    }

    if (checkFile(result) != 0) {
      cout << "Warning: " << result[0] << " has disappeared. \n" << endl;
    }
  }

  return 0;
}



//
// query the database for the filesystem path covered by
// the content.
//
int 
getPathsFromDatabase(DataBase& db, vector<string> &paths)
{

  QueryResult result(db.query("SELECT path FROM ConfigTable;"));
  
  for (vector<string>& val : result) {
    if (val.size() != 1) {
      errMsg("Trouble retrieving file path");
      return 1;
    }

    paths.push_back(val[0]);
  }

  return 0;
}

    

//
// walk directory hierarchy starting at path 
//
int 
walkPath(string path, DataBase& db, actionToggle requestType) 
{
  FTS* fileTree;
  char* paths[] = {(char*)path.c_str(), NULL};

  int options = FTS_LOGICAL;
  if (requestType == CHECK_REQUEST) {
    options |= FTS_NOSTAT;
  }

  fileTree = fts_open(paths, options, NULL);
  if (fileTree == NULL) {
    errMsg("fts_open failed");
    return 1;
  }

  // walk tree depending on request
  if (requestType == UPDATE_FILE_REQUEST) {
    if (walkPathToUpdate(fileTree, db) != 0) {
      return 1;
    }
  }
  else if (requestType == CHECK_REQUEST) {
   if (walkPathToCheck(fileTree, db) != 0) {
      return 1;
    }
  }

  if (errno != 0) {
    errMsg("fts_read failed");
    return 1;
  }

  if (fts_close(fileTree) < 0) {
    errMsg("fts_read failed");
    return 1;
  }

  return 0;
}
  


//
// walk file tree given by FTS and update the database
//
int 
walkPathToUpdate(FTS* fileTree, DataBase& db)
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
      updateFile(file->fts_accpath, file->fts_statp, db);
    }
  }

  // end transaction - if it fails roll it back
  db.query("COMMIT TRANSACTION");
  if (!db.success()) {
    db.query("ROLLBACK TRANSACTION");
  }

  return 0;
}



//
// walk file tree given by FTS and check if any
// entries are not in database
//
// NOTE: It is very inefficient to walk through
// the database entry by entry. Instead we dump
// the whole content (filenames only) into an
// unordered_set and look through it.
//
int 
walkPathToCheck(FTS* fileTree, DataBase& db)
{
  FTSENT* file;

  QueryResult result(db.query("SELECT * FROM FileTable;"));
  unordered_set<string> allFiles;
  for(vector<string>& item : result) {
    if (item.size() != 8) {
      continue;
    }

    allFiles.emplace(item[0]);
  }

  while ((file = fts_read(fileTree))) {
    if (file->fts_info == FTS_F) {
      // skip files we can't open since they won'r be in the database
      string fileName(file->fts_accpath);
      ifstream openFile(fileName);
      if (!openFile) {
        continue;
      }

      if (allFiles.find(fileName) == allFiles.end()) {
        cerr << "Warning: " << fileName << " not in database\n"
             << endl;
      }
    }
  }

  return 0;
}



//
// process each file in the filetree walk and update its information
// in the database 
//
int 
updateFile(const char *fpath, const struct stat *sb, DataBase &db) 
{
  string fileName(fpath);
  ifstream file(fileName);
  if (!file) {
    return 1;
  }
     
  Sha256Hash hash = hashAsSha256(file);
  string hashString;
  hashToString(hash, hashString);

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



//
// compare reference file properties against the current values on
// the file system
//
int 
checkFile(std::vector<std::string> referenceValues)
{
  string fileName = referenceValues[0];
  
  // check contents 
  ifstream file(fileName);
  if (!file) {
    return 1;
  }
  
  Sha256Hash hash = hashAsSha256(file);
  string hashString;
  hashToString(hash, hashString);

  // stat the file
  struct stat sb;
  if (stat(fileName.c_str(), &sb) != 0) {
    return 1;
  }
  
  // all checks out now look at the result and print error if needed
  checkAndPrintResult(hashString, referenceValues, fileName, &sb);
 
  return 0; 
}



//
// update the scanning paths with the supplied values 
//
int 
updatePaths(DataBase& db, const vector<string>& paths) 
{
  db.query("DROP TABLE IF EXISTS ConfigTable;");
  db.query("CREATE TABLE ConfigTable (path TEXT);");
  db.query("BEGIN IMMEDIATE TRANSACTION");
  
  for (const string& path : paths) {
    boost::filesystem::path absPath = boost::filesystem::canonical(path);
    db.query("INSERT INTO ConfigTable (path) VALUES (\"" 
        + absPath.string() + "\");");
  }

  db.query("COMMIT TRANSACTION");
  if (!db.success()) {
    db.query("ROLLBACK TRANSACTION");
  }

  return 0;
}



//
// update properties of all files under path
// 
// FIXME: In principle we could just update the entries here.
// The question is if this is faster than just creating
// them from scratch.
//
int 
updateFileProperties(DataBase& db, const vector<string>& paths)
{
  db.query("DROP TABLE IF EXISTS FileTable;");
  db.query("CREATE TABLE FileTable "
            "(name TEXT, hash TEXT, uid TEXT, gid TEXT, "
            "mode TEXT, size TEXT, mtime TEXT, ctime TEXT)");

  for (const string& path : paths) {
    if (walkPath(path, db, UPDATE_FILE_REQUEST) != 0) {
      errMsg("Error occured in walkPath");
      return 1;
    }
  }

  return 0;
}

