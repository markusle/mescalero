
#ifndef MESCALERO_HPP
#define MESCALERO_HPP

//#include <algorithm>
//#include <iterator>
//#include <iostream>
//#include <fstream>
//#include <sstream>
//#include <string>
//#include <memory>
//#include <cstdio>
//#include <vector>
//#include <openssl/sha.h>
//#include <fts.h>
//#include <sys/stat.h>
//#include <time.h>
//#include <sqlite3.h>
//#include <boost/filesystem.hpp>

//using std::cout;
//using std::endl;
//using std::string;
//using std::ifstream;
//using std::vector;
//using std::shared_ptr;

typedef std::shared_ptr<std::vector<unsigned char>> sha256Hash;

/* macros for type of request */
const int UPDATE_REQUEST = 0;
const int CHECK_REQUEST = 1;



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
sha256Hash encode_as_sha256(std::ifstream &filename);
void print_hash(std::string fileName, sha256Hash hash);
int process_file(const char *fpath, const struct stat *sb,
                 DataBase& db, int requestType);
int walk_path(std::string path, DataBase& db, int requestType);
void hash_to_string(sha256Hash hash, std::string& hashString);


#endif
