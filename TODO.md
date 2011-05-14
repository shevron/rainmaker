Rainmaker 1.0 TODO
============================================================

HTTP Feature Support
--------------------
- Form data submission
  - POST as multiplart
  	- file upload support
  - POST as JSON (?)
  - GET parameters (?)
- Keepalive (?)

Testing and Analysis
--------------------
- Improve logging (log only errors, remove Soup messages)
- Scriptability - SpiderMonkey integration
- "Expected Response" per request
  - Check status code
  - Check header value / existance
  - Check body string match / pattern match / XPath query
- Per request statistics (?)

Concurrency
-----------
- Multi-machine concurrency (master / slaves) 

Internal
--------
- Simplify rmRequestParamType - only need scalars, arrays, objects and file
- Get rid of baseRequest - there should be a client setup section, and requests
- Reuse SoupMessage on request repeats
