rainmaker - a simple HTTP load generator                 
=========================================
rainmaker is an HTTP load generator - a tool designed to generate a large 
amount of repeating HTTP requests for the main purpose of testing or 
benchmarking an HTTP server or a web-based application.

Installation
------------
rainmaker requires glib 2.8 and up and libsoup 2.4 and up. Most Linux users
will be able to obtain those from their distribution repositories. For Mac OS X
it is recommended to use MacPorts to install these libraries. I have not tested
installing on other operating systems (you know who you are right?). 

You will also need the usual autotools and C compiler (gcc) to build rainmaker.

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

 - Send multiple requests to a specified URL
 - Send multiple requests concurrently from a number of clients running in
   separate threads
 - Send GET requests, and POST or PUT requests with custom content
 - Set multiple custom headers to the request
 - Optionally stop test execution in case of HTTP 4xx or 5xx error
 - With newer (2.24+) versions of libsoup, per-client cookie persistence 

Run `rainmaker --help` for usage information. 

Acknowledgments & Contact
--------------------------
rainmaker uses libsoup and glib - both are subprojects of the open-source GNOME
project. See http://www.gnome.org/ for more info. 

rainmaker was written and is kind-of-maintained by Shahar Evron. You can try
to contact me at <shahar@arr.gr> but please note I am not interested in cheap
medicine or the enlargement of various parts of my body.

Licensing & Disclaimer
----------------------
rainmaker is free / open-source software, and is distributed under the terms of
the New BSD license. This means you can do quite a lot of things with it. See 
COPYING for additional information.

rainmaker was created as an experimental, educational project. There is 
absolutely no guarantee that it actually works, or that it will not destroy
your server, crash your hard drive or eat your children. Use it at your own 
risk. 

