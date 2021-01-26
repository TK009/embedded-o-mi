

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
* Latest value InfoItem storage
  - Use reference implementation HashMap and SortedSet strategy
  - MetaDatas and value are first class O-DF paths
  - Flags variable
    - 0 is normal read-write InfoItem no special handling
    - Read-only
    - Event subscription for this path
    - Authorization required (future flag?)
    - OnChange script for this path
    - OnWrite script for this path
    - OnCall script for this path
    - OnDelete script for this path
    - OnRead script for this path


TODO - Long term plans
---------------

* Parser
* Implement slash escape to addPath and others
* Check struct packing
* Use skip list for ODF storage

