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

#include "./QueryProcessor.h"

#include <iostream>
#include <algorithm>

extern "C" {
  #include "./libhw1/CSE333.h"
}

using std::list;
using std::sort;
using std::string;
using std::vector;

namespace hw3 {

QueryProcessor::QueryProcessor(const list<string> &indexlist, bool validate) {
  // Stash away a copy of the index list.
  indexlist_ = indexlist;
  arraylen_ = indexlist_.size();
  Verify333(arraylen_ > 0);

  // Create the arrays of DocTableReader*'s. and IndexTableReader*'s.
  dtr_array_ = new DocTableReader *[arraylen_];
  itr_array_ = new IndexTableReader *[arraylen_];

  // Populate the arrays with heap-allocated DocTableReader and
  // IndexTableReader object instances.
  list<string>::const_iterator idx_iterator = indexlist_.begin();
  for (int i = 0; i < arraylen_; i++) {
    FileIndexReader fir(*idx_iterator, validate);
    dtr_array_[i] = fir.NewDocTableReader();
    itr_array_[i] = fir.NewIndexTableReader();
    idx_iterator++;
  }
}

QueryProcessor::~QueryProcessor() {
  // Delete the heap-allocated DocTableReader and IndexTableReader
  // object instances.
  Verify333(dtr_array_ != nullptr);
  Verify333(itr_array_ != nullptr);
  for (int i = 0; i < arraylen_; i++) {
    delete dtr_array_[i];
    delete itr_array_[i];
  }

  // Delete the arrays of DocTableReader*'s and IndexTableReader*'s.
  delete[] dtr_array_;
  delete[] itr_array_;
  dtr_array_ = nullptr;
  itr_array_ = nullptr;
}

// This structure is used to store a index-file-specific query result.
typedef struct {
  DocID_t docid;  // The document ID within the index file.
  int rank;       // The rank of the result so far.
} IdxQueryResult;

vector<QueryProcessor::QueryResult>
QueryProcessor::ProcessQuery(const vector<string> &query) {
  Verify333(query.size() > 0);

  // STEP 1.
  // (the only step in this file)
  vector<QueryProcessor::QueryResult> finalresult;
  for (uint32_t i = 0; i < query.size(); i++) {
    vector<QueryProcessor::QueryResult> querybegin = finalresult;
    // get the word from the query
    string word = query[i];
    for (int j = 0; j < arraylen_; j++) {
      // current index from the arrays of IndexTableReader
      IndexTableReader *indexTable = itr_array_[j];
      // docID from the doctable id search for current word
      DocIDTableReader *docIDTable = indexTable->LookupWord(word);
      // the word corresponding docidtable,
      // if null, then continue to next iteration.
      if (docIDTable == NULL) {
        continue;
      }
      // get a list of docID
      list<DocIDElementHeader> docIDlist = docIDTable->GetDocIDList();
      // current index from the arrays of DocTableReader
      DocTableReader *docTable = dtr_array_[j];

      // searches the word in docid list and check
      // if the the docid is in the list, then increase the rank
      // otherwise, add it into the docid list
      // it also determines whether the word is the first word
      list<DocIDElementHeader>:: iterator itr;
      for (auto itr = docIDlist.begin(); itr != docIDlist.end(); itr++) {
        string filename;
        // look for the filename with docID
        DocIDElementHeader header = *itr;
        docTable->LookupDocID(header.docID, &filename);
        // if the word is the first, then add the docid into the list
        if (i == 0) {
          QueryProcessor::QueryResult qr;
          qr.documentName = filename;
          qr.rank = header.numPositions;
          finalresult.push_back(qr);
          continue;
        }
        for (uint32_t index = 0; index < finalresult.size(); index++) {
          // increase the number of rank if the docid exists and
          // the word is not the first
          if (finalresult[index].documentName.compare(filename) == 0 &&
              i != 0) {
            finalresult[index].rank += header.numPositions;
          }
        }
      }
      delete docIDTable;
    }
    // after we updating the finalresult, we need to check if the rank has
    // been changed and if the two query results positions are in the same
    // document and their ranks are equivalent, we need to erase from
    // our final result
    for (uint32_t i = 0; i < querybegin.size(); i++) {
      for (uint32_t j = 0; j < finalresult.size(); j++) {
        if ((finalresult[j].
        documentName.compare(querybegin[i].documentName) == 0)
        & (finalresult[j].rank == querybegin[i].rank)) {
            finalresult.erase(finalresult.begin() + j);
        }
      }
    }
  }

  // Sort the final results.
  sort(finalresult.begin(), finalresult.end());
  return finalresult;
}

}  // namespace hw3
