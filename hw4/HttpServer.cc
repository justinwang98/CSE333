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

#include <boost/algorithm/string.hpp>
#include <iostream>
#include <memory>
#include <vector>
#include <sstream>

#include "./FileReader.h"
#include "./HttpConnection.h"
#include "./HttpRequest.h"
#include "./HttpUtils.h"
#include "./HttpServer.h"
#include "./libhw3/QueryProcessor.h"

using std::cerr;
using std::cout;
using std::endl;

namespace hw4 {

// This is the function that threads are dispatched into
// in order to process new client connections.
void HttpServer_ThrFn(ThreadPool::Task *t);

// Given a request, produce a response.
HttpResponse ProcessRequest(const HttpRequest &req,
                            const std::string &basedir,
                            const std::list<std::string> *indices);

// Process a file request.
HttpResponse ProcessFileRequest(const std::string &uri,
                                const std::string &basedir);

// Process a query request.
HttpResponse ProcessQueryRequest(const std::string &uri,
                                 const std::list<std::string> *indices);

bool HttpServer::Run(void) {
  // Create the server listening socket.
  int listen_fd;
  cout << "  creating and binding the listening socket..." << endl;
  if (!ss_.BindAndListen(AF_INET6, &listen_fd)) {
    cerr << endl << "Couldn't bind to the listening socket." << endl;
    return false;
  }

  // Spin, accepting connections and dispatching them.  Use a
  // threadpool to dispatch connections into their own thread.
  cout << "  accepting connections..." << endl << endl;
  ThreadPool tp(kNumThreads);
  while (1) {
    HttpServerTask *hst = new HttpServerTask(HttpServer_ThrFn);
    hst->basedir = staticfile_dirpath_;
    hst->indices = &indices_;
    if (!ss_.Accept(&hst->client_fd,
                    &hst->caddr,
                    &hst->cport,
                    &hst->cdns,
                    &hst->saddr,
                    &hst->sdns)) {
      // The accept failed for some reason, so quit out of the server.
      // (Will happen when kill command is used to shut down the server.)
      break;
    }
    // The accept succeeded; dispatch it.
    tp.Dispatch(hst);
  }
  return true;
}

void HttpServer_ThrFn(ThreadPool::Task *t) {
  // Cast back our HttpServerTask structure with all of our new
  // client's information in it.
  std::unique_ptr<HttpServerTask> hst(static_cast<HttpServerTask *>(t));
  cout << "  client " << hst->cdns << ":" << hst->cport << " "
       << "(IP address " << hst->caddr << ")" << " connected." << endl;

  bool done = false;
  while (!done) {
    // Use the HttpConnection class to read in the next request from
    // this client, process it by invoking ProcessRequest(), and then
    // use the HttpConnection class to write the response.  If the
    // client sent a "Connection: close\r\n" header, then shut down
    // the connection.

    // MISSING:
	  HttpRequest request;
	  HttpConnection connect(hst->client_fd);
	  if (!connect.GetNextRequest(&request)) {
		  done = true;
		  connect.~HttpConnection();
	  }
	  HttpResponse response = ProcessRequest(request, hst->basedir, hst->indices);
	  
	  if (!connect.WriteResponse(response) || request.headers["connection"] == "close") {
		  done = true;
		  connect.~HttpConnection();
	  }
  }
  
}

HttpResponse ProcessRequest(const HttpRequest &req,
                            const std::string &basedir,
                            const std::list<std::string> *indices) {
  // Is the user asking for a static file?
  if (req.URI.substr(0, 8) == "/static/") {
    return ProcessFileRequest(req.URI, basedir);
  }

  // The user must be asking for a query.
  return ProcessQueryRequest(req.URI, indices);
}


HttpResponse ProcessFileRequest(const std::string &uri,
                                const std::string &basedir) {
  // The response we'll build up.
  HttpResponse ret;

  // Steps to follow:
  //  - use the URLParser class to figure out what filename
  //    the user is asking for.
  //
  //  - use the FileReader class to read the file into memory
  //
  //  - copy the file content into the ret.body
  //
  //  - depending on the file name suffix, set the response
  //    Content-type header as appropriate, e.g.,:
  //      --> for ".html" or ".htm", set to "text/html"
  //      --> for ".jpeg" or ".jpg", set to "image/jpeg"
  //      --> for ".png", set to "image/png"
  //      etc.
  //
  // be sure to set the response code, protocol, and message
  // in the HttpResponse as well.
  std::string fname = "";

  // MISSING:
  //create urlparser and parse
  URLParser up;
  up.Parse(uri);

  // acquire the path
  fname = up.get_path().substr(8);

  // read the path into mem
  FileReader fr(basedir, fname);
  std::string fContents;
  if (fr.ReadFile(&fContents) == true) {
	  //copy file contents
	  ret.body = fContents;

	  //set response type accordingly

	  boost::trim(fname);
	  std::vector<std::string> fileNameParts;
	  boost::iter_split(fileNameParts, fname, boost::algorithm::first_finder("."));
	  std::string headerType = fileNameParts[1];

	  if (headerType.compare("html") == 0 || headerType.compare("htm") == 0) {
		  ret.headers["Content-type"] = "text/html";
	  }
	  else if (headerType.compare("css") == 0) {
		  ret.headers["Content-type"] = "text/css";
	  }
	  else if (headerType.compare("xml") == 0) {
		  ret.headers["Content-type"] = "text/xml";
	  }
	  else if (headerType.compare("javascript") == 0) {
		  ret.headers["Content-type"] = "application/javascript";
	  }
	  else if (headerType.compare("jpeg") == 0 || headerType.compare("jpg") == 0) {
		  ret.headers["Content-type"] = "image/jpeg";
	  }
	  else if (headerType.compare("png") == 0) {
		  ret.headers["Content-type"] = "image/png";
	  }
	  else if (headerType.compare("gif") == 0) {
		  ret.headers["Content-type"] = "image/gif";
	  }

	  ret.protocol = "HTTP/1.1";
	  ret.response_code = 200;
	  ret.message = "OK";
	  return ret;
  }



  // If you couldn't find the file, return an HTTP 404 error.
  ret.protocol = "HTTP/1.1";
  ret.response_code = 404;
  ret.message = "Not Found";
  ret.body = "<html><body>Couldn't find file \"";
  ret.body +=  EscapeHTML(fname);
  ret.body += "\"</body></html>";
  return ret;
}

HttpResponse ProcessQueryRequest(const std::string &uri,
                                 const std::list<std::string> *indices) {
  // The response we're building up.
  HttpResponse ret;

  // Your job here is to figure out how to present the user with
  // the same query interface as our solution_binaries/http333d server.
  // A couple of notes:
  //
  //  - no matter what, you need to present the 333gle logo and the
  //    search box/button
  //
  //  - if the user had previously typed in a search query, you also
  //    need to display the search results.
  //
  //  - you'll want to use the URLParser to parse the uri and extract
  //    search terms from a typed-in search query.  convert them
  //    to lower case.
  //
  //  - you'll want to create and use a hw3::QueryProcessor to process
  //    the query against the search indices
  //
  //  - in your generated search results, see if you can figure out
  //    how to hyperlink results to the file contents, like we did
  //    in our solution_binaries/http333d.

  // MISSING:
  
  ret.body = "<html><head><title>333gle</title></head>\n";
  ret.body += "<body>\n";
  ret.body += "<center style = \"font-size:500%;\">\n";
  ret.body += "<span style = \"position:relative;bottom:-0.33em;color:orange;\">3</span><span style = \"color:red;\">3</span><span style = \"color:gold;\">3</span><span style = \"color:blue;\">g</span><span style = \"color:green;\">l</span><span style = \"color:red;\">e</span>\n";
  ret.body += "</center>\n";
  ret.body += "<p>\n";
  ret.body += "</p><div style = \"height:20px;\"></div>\n";
  ret.body += "<center>\n";
  ret.body += "<form action = \"/query\" method = \"get\">\n";
  ret.body += "<input type = \"text\" size = \"30\" name = \"terms\">\n";
  ret.body += "<input type = \"submit\" value = \"Search\">\n";
  ret.body += "</form>\n";
  ret.body += "</center><p>\n";
  ret.body += "\n\n";
  ret.body += "</p>";

  // create and use parser to get back search terms
  URLParser up;
  up.Parse(uri);
  std::string terms = up.get_args()["terms"];
  if (terms.compare("") != 0) { // there was a previous search with terms

	  // to lower and trimmed
	  boost::trim(terms);
	  boost::to_lower(terms);
	  //construct QueryProcessor
	  hw3::QueryProcessor qp(*indices, false);

	  std::vector<std::string> queryTerms;
	  boost::iter_split(queryTerms, terms, boost::algorithm::first_finder(" "));

	  // process queries
	  std::vector<hw3::QueryProcessor::QueryResult> results = qp.ProcessQuery(queryTerms);

	  if (results.empty() == true) { // no results
		  ret.body += "<p><br>";
		  ret.body += "No results found for <b>";
		  ret.body += terms;;
		  ret.body += "</b>";
		  ret.body += "</p><p>\n</p>";
	  }
	  else { // parse results
		  if(results.size() == 1){
			  ret.body += "<p><br>";
			  ret.body += "1 result found for <b>";
			  ret.body += terms;
			  ret.body += "</b>";
			  ret.body += "</p><p>\n</p>";
		  }
		  else { //greater than 1
			  ret.body += "<p><br>";
			  ret.body += std::to_string(results.size());
			  ret.body += " results found for <b>";
			  ret.body += terms;
			  ret.body += "</b>";
			  ret.body += "</p><p>\n</p>";
		  }
		  // now add in the ul
		  ret.body += "<ul>\n";

		  for (unsigned int i = 0; i < results.size(); i++) {
			  ret.body += "<li>\n <a href=\"";
			  if (results[i].document_name.find("http://") == string::npos) { // add  static
				  ret.body += "/static/";
			  }
			  ret.body += results[i].document_name + "\">\n";
			  ret.body += results[i].document_name + "</a>\n";
			  ret.body += "[";
			  ret.body += std::to_string(results[i].rank);
			  ret.body += "]\n<br>\n</li>\n";
		  }
		  ret.body += "</ul>\n";
	  }
	  
  } 
  
  ret.body += "</body></html>";
  ret.protocol = "HTTP/1.1";
  ret.response_code = 200;
  ret.message = "OK";
  return ret;
}

}  // namespace hw4
