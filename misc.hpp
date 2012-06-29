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

/* typedefs */
typedef std::shared_ptr<std::vector<unsigned char>> sha256Hash;

/* function declarations */
void err_msg(std::string message);

sha256Hash hash_as_sha256(std::ifstream &filename);

void hash_to_string(sha256Hash hash,
                    std::string &hashString);

void print_hash(std::string fileName,
                sha256Hash hash);

void check_and_print_result(std::string &hashString,
                            std::vector<std::string> queryResult,
                            std::string fileName,
                            const struct stat *sb); 

void result_formatter(const std::string &name,
                      const std::string &found,
                      const std::string &expected,
                      std::string &result);


/* simple template helper to convert stat types to string */
template <typename T> std::string to_string(T inValue) {
  std::ostringstream converter;
  converter << inValue;
  return converter.str();
}


#endif
