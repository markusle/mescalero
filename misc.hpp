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

#ifndef MISC_HPP
#define MISC_HPP

#include <memory>
#include <sstream>
#include <string>


// typedefs 
using Sha256Hash = std::unique_ptr<std::vector<unsigned char>>;


// function declarations 
void errMsg(std::string message);

Sha256Hash hashAsSha256(std::ifstream& filename);

void hashToString(const Sha256Hash& sha256Hash,
                  std::string& hashString);

void printHash(std::string& fileName,
               Sha256Hash& sha256Hash);

void checkAndPrintResult(const std::string& hashString,
                         const std::vector<std::string>& queryResult,
                         const std::string& fileName,
                         const struct stat* sb); 

void resultFormatter(const std::string& name,
                     const std::string& found,
                     const std::string& expected,
                     std::string& result);


void usage();


// simple template helper to convert stat types to string 
template <typename T> std::string to_string(T inValue) {
  std::ostringstream converter;
  converter << inValue;
  return converter.str();
}


#endif
