
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
* Scripting InfoItem write handlers; ability to replace write values before subscription trigger and save ("responsible agent")

Sorted by about highest to lowest priority:

* Fix crash bug when subscribing to items that also have scripts (trace; requestHandler.c:43, requestHandler.c:423,502,571 OMIParser.c:336)
* Arduino-ESP: Change/Modify websocket library to allow output message frame streaming 
    - Some tools expect the whole request/response be in one ws message
    - Currently the whole message needs to be passed as one chunk (library limitation)
    - Fix allows more concurrent clients and/or memory as buffers can be reduced
* Arduino-ESP: add ws url callback support
    - to allow device to device subscriptions added from a configurator app
* Arduino-ESP: fix parser reuse for open connections and refactor async code handling with locks
    - bugs where parsers or input buffers are not released correctly
* Change parser string buffer to dynamic when full, instead of cutting the text
    - The buffer is used for long string values
    - Mainly problem with long scripts or descriptions; other strings are usually short
    - current limit is `1024` chars (`ParserSinglePassLength` define)
* Automatic current connection subscription cancel on connection close
    - otherwise memory is wasted
* Fix script write self path causing incorrect subscription notifications when subscribing after script addition
    - first result is the raw value and second is value modified and written by the script
    - options include maintaining write handler order with scripts having higher priority or in separate item/list
    - manual workaround: always write to other item in scripts
* Script API: read infoitems
    - makes some logic easier to implement in scripts
    - workaround: put a onwrite script in the interesting items to copy the walue in a global var
* Interval subscription request
* Delete request
* Object(s) level subscriptions that expand when new children are added 
    - currently subscribes only currently existing leaf nodes
* Flash storage for latest values
    - flash wears fast -> spread over all available space
    - flash needs to be written in chunks -> better to collect full chunk before writing
    - maybe by using proprietary attribute to trigger flash write
* Value history (maybe with poll subscriptions only)
    - Circular buffers?
* Callback response retry upon failure
* Script API: subscription handler scripts to change incoming paths before write or do something completely different with the data
* Parser named xmlns attributes
    - does not understand them, easiest fix would be to remove any prefixes as there is no overlap with O-MI and O-DF
* Creation subscriptions; interval=-2
    - 

