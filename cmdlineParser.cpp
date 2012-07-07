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

#include <string>
#include <unistd.h>
#include <iostream>
#include <vector>

#include "cmdlineParser.hpp"
#include "misc.hpp"

using std::cout;
using std::endl;
using std::string;
using std::vector;


//
// function for checking for parsing and checking the provided
// command line arguments.
//
int 
parseCommandline(int argc, char** argv, CmdLineOpts& cmdLineOpts)
{
  // we need at least a password and one specific request
  if (argc < 4) {
    usage();
    return 1;
  }

  // check command line arguments
  cmdLineOpts.action = NONE;
  cmdLineOpts.password = "";
  int c;
  while ((c = getopt(argc,argv, "p:ucfl") ) != -1 ) {
    switch (c) {
        case 'u':
          cmdLineOpts.action = UPDATE_FILE_REQUEST;
          break;
        case 'c':
          cmdLineOpts.action = CHECK_REQUEST;
          break;
        case 'l':
          cmdLineOpts.action = LIST_PATH_REQUEST;
          break;
        case 'f':
          cmdLineOpts.action = UPDATE_PATH_REQUEST;
          break;
        case 'p':
          cmdLineOpts.password = string(optarg);
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
  if (cmdLineOpts.action == NONE) {
    errMsg("Please specify at least one option!");
    usage();
    return 1;
  }

  if (cmdLineOpts.password.empty()) {
    errMsg("You must provide a password for encrypting the database.");
    usage();
    return 1;
  }

  // grab list of paths if requested
  if (cmdLineOpts.action == UPDATE_PATH_REQUEST) {
    // need at least one path
    if (optind == argc) {
      return 1;
    }

    for (int i=optind; i<argc; ++i) {
      cmdLineOpts.pathList.push_back(string(argv[i]));
    }
  }

  return 0;
}


