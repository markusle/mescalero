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

#ifndef CMDLINEPARSER_HPP
#define CMDLINEPARSER_HPP


// macros for type of request 
enum actionToggle {NONE, 
                   UPDATE_FILE_REQUEST, 
                   CHECK_REQUEST,
                   UPDATE_PATH_REQUEST,
                   LIST_PATH_REQUEST};



// structure describing commandline options 
struct CmdLineOpts {
  std::string password;
  actionToggle action;
  std::vector<std::string> pathList;
};


// function declarations
int parseCommandline(int argc, char** argv, CmdLineOpts& cmdLineOpts);



#endif
