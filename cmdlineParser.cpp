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


/* 
 * function for checking for parsing and checking the provided
 * command line arguments.
 */
int parse_commandline(int argc, char **argv, cmdLineOpts &options)
{
  // we need at least a password and one specific request
  if (argc < 4)
  {
    usage();
    return 1;
  }

  // check command line arguments
  options.action = NONE;
  options.password = "";
  int c;
  while ((c = getopt(argc,argv, "p:ucfl") ) != -1 )
  {
    switch (c)
    {
        case 'u':
          options.action = UPDATE_FILE_REQUEST;
          break;
        case 'c':
          options.action = CHECK_REQUEST;
          break;
        case 'l':
          options.action = LIST_PATH_REQUEST;
          break;
        case 'f':
          options.action = UPDATE_PATH_REQUEST;
          break;
        case 'p':
          options.password = string(optarg);
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
  if (options.action == NONE)
  {
    err_msg("Please specify at least one option!");
    usage();
    return 1;
  }

  if (options.password.empty()) 
  {
    err_msg("You must provide a password for encrypting the database.");
    usage();
    return 1;
  }

  // grab list of paths if requested
  if (options.action == UPDATE_PATH_REQUEST) 
  {
    // need at least one path
    if (optind == argc) {
      return 1;
    }

    for (int i=optind; i<argc; ++i) 
    {
      options.pathList.push_back(string(argv[i]));
    }
  }

  return 0;
}


