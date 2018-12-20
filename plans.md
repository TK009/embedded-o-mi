

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


TODO - Long term plans
---------------

* Implement slash escape to addPath and others
* Check struct packing
* Use skip list for ODF storage

