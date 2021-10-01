/*
 * Copyright ©2020 Hal Perkins.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Washington
 * CSE 333 for use solely during Fall Quarter 2020 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <list>

#include "./ServerSocket.h"
#include "./HttpServer.h"

using std::cerr;
using std::cout;
using std::endl;
using std::list;
using std::string;

// Print out program usage, and exit() with EXIT_FAILURE.
static void Usage(char *progname);

// Parses the command-line arguments, invokes Usage() on failure.
// "port" is a return parameter to the port number to listen on,
// "path" is a return parameter to the directory containing
// our static files, and "indices" is a return parameter to a
// list of index filenames.  Ensures that the path is a readable
// directory, and the index filenames are readable, and if not,
// invokes Usage() to exit.
static void GetPortAndPath(int argc,
                    char **argv,
                    uint16_t *port,
                    string *path,
                    list<string> *indices);

int main(int argc, char **argv) {
  // Print out welcome message.
  cout << "Welcome to http333d, the UW cse333 web server!" << endl;
  cout << "  Copyright 2012 Steven Gribble" << endl;
  cout << "  http://www.cs.washington.edu/homes/gribble" << endl;
  cout << endl;
  cout << "initializing:" << endl;
  cout << "  parsing port number and static files directory..." << endl;

  // Ignore the SIGPIPE signal, otherwise we'll crash out if a client
  // disconnects unexpectedly.
  signal(SIGPIPE, SIG_IGN);

  // Get the port number and list of index files.
  uint16_t portnum;
  string staticdir;
  list<string> indices;
  GetPortAndPath(argc, argv, &portnum, &staticdir, &indices);
  cout << "    port: " << portnum << endl;
  cout << "    path: " << staticdir << endl;

  // Run the server.
  hw4::HttpServer hs(portnum, staticdir, indices);
  if (!hs.Run()) {
    cerr << "  server failed to run!?" << endl;
  }

  cout << "server completed!  Exiting." << endl;
  return EXIT_SUCCESS;
}


static void Usage(char *progname) {
  cerr << "Usage: " << progname << " port staticfiles_directory indices+";
  cerr << endl;
  exit(EXIT_FAILURE);
}

static void GetPortAndPath(int argc,
                    char **argv,
                    uint16_t *port,
                    string *path,
                    list<string> *indices) {
  // Be sure to check a few things:
  //  (a) that you have a sane number of command line arguments
  //  (b) that the port number is reasonable
  //  (c) that "path" (i.e., argv[2]) is a readable directory
  //  (d) that you have at least one index, and that all indices
  //      are readable files.

  // STEP 1:
  // check a sane number of command line arguments
  if (argc < 4) {
    Usage(argv[0]);
  }
  // check that the port number is reasonable
  int port_num = atoi(argv[1]);
  if (port_num < 0 || port_num > 65535) {
    cerr << "Port number is not valid error." << endl;
    Usage(argv[0]);
  }
  // set the pointer to point to the port number
  *port = static_cast<uint16_t> (port_num);
  struct stat path_stat;
  // if the current path is not readable directory,
  // print out the error message
  if (stat(argv[2], &path_stat) == -1 || !S_ISDIR(path_stat.st_mode)) {
    cerr << "Path is not valid error." << endl;
    Usage(argv[0]);
  }
  *path = string(argv[2]);
  for (int i = 3; i < argc; i++) {
    // firstly check out the information of filename
    string filename(argv[i]);
    if (filename.length() <= 4 ||
        filename.substr(filename.length() - 4) != ".idx") {
      cerr << "Not index file read error." << endl;
      Usage(argv[i]);
    }
    // if the current index is not readable directory,
    // print out the error message
    struct stat stat_index;
    if (stat(argv[2], &stat_index) == -1 || !S_ISDIR(stat_index.st_mode)) {
      cerr << "index is not valid error." << endl;
      Usage(argv[0]);
    }
    indices->push_back(string(argv[i]));
  }
}

