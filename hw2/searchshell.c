/*
 * Copyright ©2019 Hal Perkins.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Washington
 * CSE 333 for use solely during Winter Quarter 2019 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

// Feature test macro for strtok_r (c.f., Linux Programming Interface p. 63)
#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "libhw1/CSE333.h"
#include "memindex.h"
#include "filecrawler.h"

static void Usage(void);

int main(int argc, char **argv) {
  if (argc != 2)
    Usage();

  // Implement searchshell!  We're giving you very few hints
  // on how to do it, so you'll need to figure out an appropriate
  // decomposition into functions as well as implementing the
  // functions.  There are several major tasks you need to build:
  //
  //  - crawl from a directory provided by argv[1] to produce and index
  //  - prompt the user for a query and read the query from stdin, in a loop
  //  - split a query into words (check out strtok_r)
  //  - process a query against the index and print out the results
  //
  // When searchshell detects end-of-file on stdin (cntrl-D from the
  // keyboard), searchshell should free all dynamically allocated
  // memory and any other allocated resources and then exit.
 
  // crawl dir and build memindex
  DocTable doctable;
  MemIndex memindex;
  printf("Indexing '%s'\n", argv[1]);
  Verify333(CrawlFileTree(argv[1], &doctable, &memindex) == 1);

  int length = 0;
  char* userquery[64];

  while (1) {
    int buffsize = 256;
    char* buffer = (char*) malloc(buffsize);
    
    printf("enter query:\n");
    fgets(buffer, buffsize - 1, stdin);
    for (int i =0; i< buffsize - 1; i++) {
      if (buffer[i] == '\n') {
        buffer[i] = '\0';
	break;
      }
    }
    //get elements
    char* saveptr;
    userquery[length] = strtok_r(buffer, " ", &saveptr);
    if (userquery[length] == NULL) {
      printf("bad input");
    }
    length++;
    while (1) { // fence post to use strtok_r
      userquery[length] = strtok_r(NULL, " ", &saveptr);
      if (userquery[length] == NULL) {
	break;
      }
      length++;
    }
   
    LinkedList list = MIProcessQuery(memindex, (char**) userquery, length);
    if (list != NULL) { // non empty result
      SearchResult* search;
      int num = NumElementsInLinkedList(list);
      LLIter iter = LLMakeIterator(list, 0);

      for (int i = 0; i < num; i++) {
        LLIteratorGetPayload(iter, (LLPayload_t*) &search);
        printf( "  %s (%u)\n", DTLookupDocID(doctable, search->docid), search->rank);
	LLIteratorDelete(iter, &free);
      }
      LLIteratorFree(iter);
      FreeLinkedList(list, &free);
    }
    length = 0;
  }
  FreeDocTable(doctable);
  FreeMemIndex(memindex);


  return EXIT_SUCCESS;
}

static void Usage(void) {
  fprintf(stderr, "Usage: ./searchshell <docroot>\n");
  fprintf(stderr,
          "where <docroot> is an absolute or relative " \
          "path to a directory to build an index under.\n");
  exit(EXIT_FAILURE);
}

