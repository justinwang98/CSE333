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

#include <iostream>
#include <algorithm>

#include "./QueryProcessor.h"

extern "C" {
  #include "./libhw1/CSE333.h"
}

namespace hw3 {

QueryProcessor::QueryProcessor(list<string> indexlist, bool validate) {
  // Stash away a copy of the index list.
  indexlist_ = indexlist;
  arraylen_ = indexlist_.size();
  Verify333(arraylen_ > 0);

  // Create the arrays of DocTableReader*'s. and IndexTableReader*'s.
  dtr_array_ = new DocTableReader *[arraylen_];
  itr_array_ = new IndexTableReader *[arraylen_];

  // Populate the arrays with heap-allocated DocTableReader and
  // IndexTableReader object instances.
  list<string>::iterator idx_iterator = indexlist_.begin();
  for (HWSize_t i = 0; i < arraylen_; i++) {
    FileIndexReader fir(*idx_iterator, validate);
    dtr_array_[i] = new DocTableReader(fir.GetDocTableReader());
    itr_array_[i] = new IndexTableReader(fir.GetIndexTableReader());
    idx_iterator++;
  }
}

QueryProcessor::~QueryProcessor() {
  // Delete the heap-allocated DocTableReader and IndexTableReader
  // object instances.
  Verify333(dtr_array_ != nullptr);
  Verify333(itr_array_ != nullptr);
  for (HWSize_t i = 0; i < arraylen_; i++) {
    delete dtr_array_[i];
    delete itr_array_[i];
  }

  // Delete the arrays of DocTableReader*'s and IndexTableReader*'s.
  delete[] dtr_array_;
  delete[] itr_array_;
  dtr_array_ = nullptr;
  itr_array_ = nullptr;
}

vector<QueryProcessor::QueryResult>
QueryProcessor::ProcessQuery(const vector<string> &query) {
  Verify333(query.size() > 0);
  vector<QueryProcessor::QueryResult> finalresult;

  // MISSING:

  for (HWSize_t i = 0; i < arraylen_; i++) {
	  // initialize all the readers
	  IndexTableReader *indR = itr_array_[i];
	  DocTableReader *docR = dtr_array_[i];
	  DocIDTableReader *didR = indR->LookupWord(query[0]);

	  // check if docidtable is null/empty
	  if (didR != nullptr) {
		  list<docid_element_header> list1 = didR->GetDocIDList();

		  // loop thru the rest of the query
		  for (HWSize_t j = 1; j < query.size(); j++) {
			  didR = indR->LookupWord(query[j]);

			  // check if docidtable is null/empty
			  if (didR != nullptr) {
				  // initialize secondary list and iterators
				  int seen = 0;
				  list<docid_element_header> list2 = didR->GetDocIDList();

				  list<docid_element_header>::iterator iter1;
				  list<docid_element_header>::iterator iter2;
				  
				  // loop thru both lists
				  for (iter1 = list1.begin(); iter1 != list1.end();) {
					  for (iter2 = list2.begin(); iter2 != list2.end();) {
						  // deal with same docid
						  if (iter1->docid == iter2->docid) {
							  seen++;
							  iter1->num_positions += iter2->num_positions;
							  break;
						  }
						  iter2++;
					  }
					  if (seen != 0) {
						  iter1++;
						  seen = 0;
					  }
					  else {
						  iter1 = list1.erase(iter1);
					  }
				  }

			  }
			  else {
				  list1.clear();
				  break;
			  }
		  }
		  
		  // check if original list is empty
		  if (!list1.empty()) {
			  list<docid_element_header>::iterator iter;
			  string name;

			  // create the query to be pushed onto the finalresult list
			  for (iter = list1.begin(); iter != list1.end();) {
				  docR->LookupDocID(iter->docid, &name);
				  QueryResult query{ name, iter->num_positions};
				  finalresult.push_back(query);
				  iter++;
			  }
		  }
		  // cleanup
		  delete didR;
	  }
  }

  // Sort the final results.
  std::sort(finalresult.begin(), finalresult.end());
  return finalresult;
}

}  // namespace hw3
