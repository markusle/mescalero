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
#include <cstdio>
#include <sys/stat.h>
#include <vector>
#include <openssl/sha.h>

#include "misc.hpp"
#include "mescalero.hpp"

using std::cerr;
using std::cout;
using std::endl;
using std::vector;
using std::string;
using std::ifstream;
using std::shared_ptr;



//
// error reporting function 
//
void 
errMsg(std::string message) 
{
  cerr << "Error: " << message << endl;
}



//
// helper function to print out a file's hash 
//
void 
printHash(const string& fileName, const Sha256Hash& sha256Hash) 
{
  cout << fileName << ":  ";
  for (const unsigned char& item : *sha256Hash) {
    printf("%02x", item);
  }
    
  cout << endl;
}



//
// convert a Sha256Hash type into a string represenation 
//
void 
hashToString(const Sha256Hash& sha256Hash, string& hashString) 
{
  hashString.clear();

  char c[3];
  for (const unsigned char& item : *sha256Hash) {
    sprintf(c, "%02x", item);
    hashString += c;
  }
}



//
// check current file status against what we have stored
// in the database and complain if there's mismatch 
//
void 
checkAndPrintResult(const string& hashString, 
                    const vector<std::string>& result,
                    const string& fileName, const struct stat *sb) 
{
  bool status = false;
  string m;
  if (result[1] != hashString) {
    status = true;
    m += "hash mismatch :: \n";
    m = m + "(F) " + hashString + "\n";
    m = m + "(E) " + result[1] + "\n";
  }

  if (result[2] != to_string(sb->st_uid) ) {
    status = true;
    resultFormatter("uid", to_string(sb->st_uid),
                     result[2], m);
  }

  if (result[3] != to_string(sb->st_gid) ) {
    status = true;
    resultFormatter("gid", to_string(sb->st_gid),
                     result[3], m);
  }

  if (result[4] != to_string(sb->st_mode) ) {
    status = true;
    resultFormatter("mode", to_string(sb->st_mode),
                     result[4], m);
  }

  if (result[5] != to_string(sb->st_size) ) {
    status = true;
    resultFormatter("size", to_string(sb->st_size),
                     result[5], m);
  }

  if (result[6] != to_string(sb->st_mtime) ) {
    status = true;
    resultFormatter("mtime", to_string(sb->st_mtime),
                     result[6], m);
  }

  if (result[7] != to_string(sb->st_ctime) ) {
    status = true;
    resultFormatter("ctime", to_string(sb->st_ctime),
                     result[7], m);
    }

  if (status) {
    cout << "+++++++++ Mismatch ++++++++++++++++++++\n";
    cout << "filename : " << fileName << "\n";
    cout << m;
    cout << "++++++++++++++++++++++++++++++++++++++\n" << endl;
  }
}



//
// simple pretty printer for outputting expected versus
// found results 
//
void 
resultFormatter(const string &name, const string &found,
                const string &expected, string &result) 
{
  result = result + name + " mismatch :: ";
  result = result + "(F) " + found + " | ";
  result = result + "(C) " + expected + "\n";
}



//
// function computing the sha256 hash of the file
// reference by ifstream file 
//
Sha256Hash 
hashAsSha256(ifstream &file) 
{
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

  Sha256Hash hash(new vector<unsigned char>);
  std::copy(md, md+SHA256_DIGEST_LENGTH, std::back_inserter(*hash));
  
  return hash;
}



// 
// sconcho usage information 
//
void 
usage() 
{
  cout << "mescalero v " << VERSION << " (C) 2012 Markus Dittrich\n\n"
       << "usage: mescalero -p password [options]\n\n"
       << "Available options (at least one is required):\n\n"
       << "\t-c\n"
       << "\t   check file properties\n\n"
       << "\t-u\n"
       << "\t   update file properties\n\n"
       << "\t-l\n"
       << "\t   list the current set of file paths\n\n"
       << "\t-f <list of file paths>\n"
       << "\t   update file paths for file checking\n\n"
       << "\n"
       << endl;
}
