
Code for using Open Messaging Interface (O-MI) with Open Data Format (O-DF) in low memory conditions.

Contains a partial O-MI Node implementation for linux.

Compiling and testing
--------------------

Desktop version compiled with `clang` for better error messages and fast unit testing.

* `make esp32-upload` to compile to esp32 WiFi SoC and upload to `PORT`
* `make test` to compile and run desktop version tests and show coverage
* `make core` Desktop version is compiled to `./core` but it only works by taking the
   requests in standard input and responds with standard output. For http
   server it needs to be run with (python3) `./embedded-o-mi-testserver.py`
* `make esp32` to only compile esp32 version


Features
--------

* In-memory, linear array store for O-DF paths (in http get format of the standards)
   * Single value store (no history) with timestamps
* O-MI requests
   * Write
   * Read
      * Single items, descriptions, MetaDatas and sub tree
      * Interval subscriptions: TODO
      * Event subscriptions (interval = -1)
   * Cancel
   * Delete: TODO

### Complexity
(n is number of O-DF nodes in the memory):
* Reading a path is ~O(log(n))
* Reading a subtree is ~O(log(n)+t), where t is the size of the subtree.
* Writing a new path is ~O(n) worst case due to array move
* Writing into existing path is ~O(log(n))
* Subscribing a path is ~O(log(n))
* Subscription cancel/end is ~O(n), but could be accelerated easily

### Speed
With small O-DF trees it is fast.

### Memory usage
Increases only when new unseen strings are written to the O-DF tree. Everything else has predefined static memory storage.

TODO
------

Sorted by about highest to lowest priority:

* Flash storage
* Value history (maybe with poll subscriptions only)
* Parser named xmlns attributes
