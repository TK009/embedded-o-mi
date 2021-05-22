
Code for using Open Messaging Interface (O-MI) with Open Data Format (O-DF) in low memory conditions.

Contains a partial O-MI Node implementation for linux.

Compiling and testing
--------------------

Remember to clone git submodules!

### Computer test version
Desktop version compiled with `clang` for better error messages and fast unit testing.

* `make test` to compile and run desktop version tests and show coverage
* `make core` Desktop command line version is compiled to `./core` but it only works by taking the
   requests in standard input and responds with standard output. For http
   server it needs to be run with (python3) `./embedded-o-mi-testserver.py`

### ESP32 S2

* TODO
* `make esp32-upload` to compile to esp32 WiFi SoC and upload to `PORT`
* `make esp32` to only compile esp32 version


Features
--------

* In-memory, linear array store for O-DF paths (in http get format of the standards)
   * Single value store (without history) with timestamps
* Transfer protocols:
   * WebSocket connections, multiple concurrent allowed
   * callback protocols: WebSocket
   * TODO: Http POST, Http GET data discovery
* O-MI requests
   * Write
   * Read
      * Single items, descriptions, MetaDatas and sub tree
      * Interval subscriptions: TODO
      * Event subscriptions (interval = -1)
   * Cancel
   * Delete: TODO
* Scripting:
   * Write to MetaData, an InfoItem named `onwrite` and write JavaScript to the value
   * Incoming write requests will call the script with globals `event.value`
       and `event.unixTime` containing the written value
   * Use `odf.writeItem(value, path)` to write any results to InfoItem indicated by the O-DF path
   * The engine supports [these features](https://github.com/jerryscript-project/jerryscript/blob/master/jerry-core/profiles/README.md)
     unless set otherwise during compilation in the config file `./platforms/*/jerryscript.config`

### Complexity
(n is number of O-DF nodes in the memory):
* Reading a path is ~O(log(n))
* Reading a subtree is ~O(log(n)+t), where t is the size of the subtree.
* Writing a new path is ~O(n) worst case, due to array move, specially presorted write to empty tree is O(1)
* Writing into existing path is ~O(log(n))
* Subscribing a path is ~O(log(n))
* Subscription cancel/end is ~O(n), but could be accelerated easily

### Speed
TODO: measure

### Memory usage
Increases only when new unseen strings are written to the O-DF tree. Everything else has predefined static memory storage.

TODO
------

Backlog of features that could be implemented are listed in this section.

Undecided features (should be implemented or not?):
* InfoItem write handlers; ability to replace write values before subscription trigger and save

Sorted by about highest to lowest priority:

* Arduino-ESP: Change/Modify websocket library to allow message frame streaming (currently the whole message needs to be passed in one chunk)
* Arduino-ESP: add ws url callback support
* Automatic current connection subscription cancel on connection close
* Fix script write self path causing incorrect subscription notifications when subscribing after script addition
   - first result is the raw value and second is value modified and written by the script
   - options include maintaining write handler order with scripts having higher priority or in separate item/list
   - manual workaround: always write to other item in scripts
* Streaming output (esp async websocket library add chunk stream)
* Script API: read infoitems
* Object(s) level subscriptions that expand when new children are added 
* Flash storage for latest values
* Script API: subscription handler scripts to transform incompatible formats
* Value history (maybe with poll subscriptions only)
* Parser named xmlns attributes
* Creation subscriptions; interval=-2
