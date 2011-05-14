rainmaker - an HTTP load testing tool                 
==============================================================================
rainmaker is an HTTP load generator - a tool designed to generate large 
amounts of HTTP requests for the main purpose of testing or benchmarking an HTTP
server or a Web application under load. 

rainmaker uses XML-based scenario files to describe a set of requests to send
to the server, as well as tests that need to be run on responses returned for
each request. Additionally, you can specify the concurrency level (how many 
times the scenario should be executed *in parallel*), and the number of times
you want the entire scenario repeated.   

Installation
------------
rainmaker requires glib 2.8, libxml2 2.7 and up and libsoup 2.4 and up. 
Most Linux users will be able to obtain those from their distribution 
repositories. For Mac OS X it is recommended to use MacPorts to install these 
libraries. I have not tested installing on other operating systems, but if you
have tried and can report on your experience, I would be happy to know.  

You will also need the usual GNU build tools and C compiler (gcc) to build 
rainmaker.

Once you have all the required tools and dependencies, simply run the usual:
    
    ./configure
    make
    make install

and you're done. 

The file INSTALL contains more details installation instructions for those 
interested.

Usage
-----
Currently, rainmaker supports the following functions:

 - Execute a set of HTTP requests specified in an XML based scenario file 
   - Use GET, POST, PUT or any arbitrary HTTP request method
   - Specify HTTP request body as raw data (could be base64 encoded) or as
     form fields to be url-encoded
   - Specify a base URL for the entire scenario
   - Set custom HTTP headers for each request or for the entire scenario
 - Run the same scenario on a number of clients (threads) in parallel 
 - Optional per-client HTTP Cookie persistence 
 - Stop test execution in case of HTTP errors (4xx or 5xx codes)
 - Stop test execution in case of TCP or other connection errors
 - Stop test execution in case of HTTP redirects (3xx) - off by default
 - Print out an execution summary specifying the total time it took to run the
   entire scenario on all clients, and the total number of requests / responses
   split by HTTP response code. 

Run `rainmaker --help` for usage information.

Currently the format of the scenario XML file is not documented. Documentation
as well as examples will be available soon - in the mean time, you can check
out the XSD file for some idea on the structure of the XML. 

If you are interested to know what's in plan for rainmaker, see the TODO.md 
file for some pending tasks and high-level features.

Acknowledgments & Contact
--------------------------
rainmaker uses, and owes most of its functionality, to the following 3rd party 
open-source libraries: 

 - [libsoup](http://live.gnome.org/LibSoup) by Dan Winship and others at the 
   GNOME project. Used under the terms of the GNU Lesser General Public License.
      
 - [glib](http://developer.gnome.org/glib/), a subproject of the GNOME project. 
   Used under the terms of the GNU Lesser General Public License.
       
 - [libxml](http://xmlsoft.org/) by Daniel Veillard. Used under the terms of 
   the [MIT license](http://www.opensource.org/licenses/mit-license.html).  

rainmaker was written and is kind-of-maintained by Shahar Evron. You can try
to contact me at <shahar@arr.gr> but please note I am not interested in cheap
medicine or the enlargement of various parts of my body.

Licensing & Disclaimer
----------------------
rainmaker is free / open-source software, and is distributed under the terms of
the New BSD license. This means you can do quite a lot of things with it, 
including using it for commercial purposes. See COPYING for additional 
information.

rainmaker was created as an experimental, educational project. There is 
absolutely no guarantee that it actually works, or that it will not destroy
your server, crash your hard drive or eat your children. Use it at your own 
risk. 
