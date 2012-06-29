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

#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <sstream>
#include <cstdio>
#include <sys/stat.h>
#include <vector>
#include <openssl/sha.h>

#include "misc.hpp"

using std::cerr;
using std::cout;
using std::endl;
using std::vector;
using std::string;
using std::ifstream;
using std::shared_ptr;


/* error reporting function */
void err_msg(std::string message) {
  cerr << "Error: " << message << endl;
}


/* helper function to print out a file's hash */
void print_hash(string fileName,
                sha256Hash hash) {

  cout << fileName << ":  ";
  for (unsigned char &item : *hash) {
    printf("%02x", item);
  }
    
  cout << endl;
}


/* convert a sha256Hash type into a string represenation */
void hash_to_string(sha256Hash hash, string& hashString) {

  hashString.clear();

  char c[3];
  for (unsigned char &item : *hash) {
    sprintf(c, "%02x", item);
    hashString += c;
  }
}


/* check current file status against what we have stored
 * in the database and complain if there's mismatch */
void check_and_print_result(string &hashString,
                            vector<std::string> queryResult,
                            string fileName,
                            const struct stat *sb) {

  bool status = false;
  string m;
  if (queryResult[1] != hashString) {
    status = true;
    m += "hash mismatch :: \n";
    m = m + "(F) " + hashString + "\n";
    m = m + "(E) " + queryResult[1] + "\n";
  }

  if (queryResult[2] != to_string(sb->st_uid) ) {
    status = true;
    result_formatter("uid", to_string(sb->st_uid),
                     queryResult[2], m);
  }

  if (queryResult[3] != to_string(sb->st_gid) ) {
    status = true;
    result_formatter("gid", to_string(sb->st_gid),
                     queryResult[3], m);
  }

  if (queryResult[4] != to_string(sb->st_mode) ) {
    status = true;
    result_formatter("mode", to_string(sb->st_mode),
                     queryResult[4], m);
  }

  if (queryResult[5] != to_string(sb->st_size) ) {
    status = true;
    result_formatter("size", to_string(sb->st_size),
                     queryResult[5], m);
  }

  if (queryResult[6] != to_string(sb->st_mtime) ) {
    status = true;
    result_formatter("mtime", to_string(sb->st_mtime),
                     queryResult[6], m);
  }

  if (queryResult[7] != to_string(sb->st_ctime) ) {
    status = true;
    result_formatter("ctime", to_string(sb->st_ctime),
                     queryResult[7], m);
    }

  if (status) {
    cout << "+++++++++ Mismatch ++++++++++++++++++++\n";
    cout << "filename : " << fileName << "\n";
    cout << m;
    cout << "++++++++++++++++++++++++++++++++++++++\n" << endl;
  }
}


/* simple pretty printer for outputting expected versus
 * found results */
void result_formatter(const string &name,
                      const string &found,
                      const string &expected,
                      string &result) {

  result = result + name + " mismatch :: ";
  result = result + "(F) " + found + " | ";
  result = result + "(E) " + expected + "\n";
}



/* function computing the sha256 hash of the file
 * reference by ifstream file */
sha256Hash hash_as_sha256(ifstream &file) {
  
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


