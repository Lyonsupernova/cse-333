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

#include <stdint.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <map>
#include <string>
#include <vector>

#include "./HttpRequest.h"
#include "./HttpUtils.h"
#include "./HttpConnection.h"

using std::map;
using std::string;
using std::vector;

namespace hw4 {
  static const char *kHeaderEnd = "\r\n\r\n";
  static const int kHeaderEndLen = 4;
  static const int buffer_size = 1024;

  bool HttpConnection::GetNextRequest(HttpRequest *request) {
    // Use "WrappedRead" to read data into the buffer_
    // instance variable.  Keep reading data until either the
    // connection drops or you see a "\r\n\r\n" that demarcates
    // the end of the request header. Be sure to try and read in
    // a large amount of bytes each time you call WrappedRead.
    //
    // Once you've seen the request header, use ParseRequest()
    // to parse the header into the *request argument.
    //
    // Very tricky part:  clients can send back-to-back requests
    // on the same socket.  So, you need to preserve everything
    // after the "\r\n\r\n" in buffer_ for the next time the
    // caller invokes GetNextRequest()!

    // STEP 1:
    // find the current position of the header end sign
    // "\r\n\r\n"
    size_t header_pos = buffer_.find(kHeaderEnd);
    // we need to reach to the end of the request header
    // if we find the header end, then break
    while (header_pos == string::npos) {
      // read the request by 1024
      unsigned char buffer[buffer_size];
      int res = WrappedRead(fd_, buffer, buffer_size);
      // print out the error message if res = -1
      if (res == -1) {
        return false;
      } else if (res == 0) {
        // connection drops
        break;
      } else {
        // buffer forward by read bytes
        buffer_ += string(reinterpret_cast<char*> (buffer), res);
        header_pos = buffer_.find(kHeaderEnd);
      }
    }
    if (header_pos == string::npos) {
      request = nullptr;
      return false;
    }
    *request = ParseRequest(buffer_.substr(0, header_pos + kHeaderEndLen));
    // if the request is not valid print out the error message

    // preserve the information of next request into the buffer
    buffer_ = buffer_.substr(header_pos + kHeaderEndLen);
    return true;  // You may want to change this.
  }

  bool HttpConnection::WriteResponse(const HttpResponse &response) {
    string str = response.GenerateResponseString();
    int res = WrappedWrite(fd_,
    (unsigned char *) str.c_str(),
    str.length());
    if (res != static_cast<int>(str.length()))
      return false;
    return true;
  }

  HttpRequest HttpConnection::ParseRequest(const string &request) {
    HttpRequest req("/");  // by default, get "/".

    // Split the request into lines.  Extract the URI from the first line
    // and store it in req.URI.  For each additional line beyond the
    // first, extract out the header name and value and store them in
    // req.headers_ (i.e., HttpRequest::AddHeader).  You should look
    // at HttpRequest.h for details about the HTTP header format that
    // you need to parse.
    //
    // You'll probably want to look up boost functions for (a) splitting
    // a string into lines on a "\r\n" delimiter, (b) trimming
    // whitespace from the end of a string, and (c) converting a string
    // to lowercase.
    //
    // Note that you may assume the request you are parsing is correctly
    // formatted. If for some reason you encounter a header that is
    // malformed, you may skip that line.

    // STEP 2:
    // split the request string into lines based on the end indicatro "\r\n"
    vector<string> lines;
    boost::split(lines, request,
                 boost::is_any_of("\r\n"),
                 boost::token_compress_on);
    // trim the white space of every line
    for (size_t i = 0; i < lines.size(); i++) {
      boost::trim(lines[i]);
    }
    // for the first line, extract the uri and store in req.uri
    vector<string> first_line;
    boost::split(first_line, lines[0],
                 boost::is_any_of(" "),
                 boost::token_compress_on);
    // format: GET [URI] [http_protocol]\r\n
    if (first_line.size() == 3 &&
      first_line[0] == "GET" &&
    first_line[1][0] == '/' &&
    first_line[2].find("HTTP/") != string::npos) {
      req.set_uri(first_line[1]);
    } else {
      return req;
    }

    // beyond the first line, extract out the header name and value and store in
    // req.headers_
    for (size_t i = 1; i < lines.size() - 1; i++) {
      // find the ": " which separtes the header name and header val
      size_t header_pos = lines[i].find(": ");
      // if not found, set as INVALID tag
      if (header_pos == string::npos) {
        return req;
      }
      string header_name = lines[i].substr(0, header_pos);
      // to lower case
      boost::to_lower(header_name);
      string header_val = lines[i].substr(header_pos + 2);
      req.AddHeader(header_name, header_val);
    }
    return req;
  }
}  // namespace hw4
