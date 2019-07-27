

Quality strategy
-------------


* Memory usage must keep static without fragmenting
  - use memory pools
  - Use low priority, "background" tasks to optimize data structures 


Architecture parts
----------------

* Main loop: Task system
  - high priority tasks
    - incoming requests etc
  - low priority background tasks
    - procedural algorithms
    - fast execution
    - can have a deadline
* Low memory parser
  - Handle everything by streaming ready pieces to processing functions
  - Parse in chunks, give control to task system after chunk
  - OmiParser -> XmlParser
  - OdfParser -> OmiParser -> XmlParser
  - parse constant strings (tag names, attribute names, etc.) as hashes


TODO - Long term plans
---------------

* Parser
* Implement slash escape to addPath and others
* Check struct packing
* Use skip list for ODF storage

