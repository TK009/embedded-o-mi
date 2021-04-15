#ifndef SETTINGS_H
#define SETTINGS_H

#ifndef ODFTREE_BLOCKS
// *32 number of latest values stored
#define ODFTREE_BLOCKS 2
#endif

#ifndef ODFTREE_SIZE
// Contains Object elements also, so can be more than 32*ODFTREE_BLOCKS
#define ODFTREE_SIZE 32 * ODFTREE_BLOCKS + 4
#endif


#endif
