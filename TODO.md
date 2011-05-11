Rainmaker 1.0 TODO
============================================================

HTTP Feature Support
--------------------
- Cookie persistance (per scenario)
- Custom headers
- POST / PUT raw data submission
- Form data submission
  - POST urlencoded vs. multiplart
  - GET
- Keepalive (?)

Testing and Analysis
--------------------
- Logging and debugging features
- Scriptability - SpiderMonkey integration
- Fail on HTTP redirect
- "Expected Response" per request
  - Check status code
  - Check header value / existance
  - Check body string match / pattern match / XPath query
- Per request statistics (?)

Concurrency
-----------
- Single machine concurrency (threads)
- Multi-machine concurrency (master / slaves) 

