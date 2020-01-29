NEO-RAW:
====

This is an Another World VM implementation.  Based on Gregory Montoir's original work, the codebase has been cleaned up with legibility and readability in mind.

Architecture:
=============

http://fabiensanglard.net/anotherWorld_code_review/index.php
http://fabiensanglard.net/another_world_polygons/index.html
http://fabiensanglard.net/another_world_polygons_PC_DOS/index.html

Fabien Sanglard


About:
------

raw is a re-implementation of the engine used in the game Another World. This 
game, released under the name Out Of This World in non-European countries, was 
written by Eric Chahi at the beginning of the '90s. More information can be 
found here : http://www.mobygames.com/game/sheet/p,2/gameId,564/.

Supported Versions:
-------------------

English PC DOS version is supported ("Out of this World").

Compiling:
----------
```
cmake .
make
```
Running:
--------

You will need the original files, here is the required list :
- BANK*
- MEMLIST.BIN
	
To start the game, you can either :
- put the game's datafiles in the same directory as the executable
- use the --datapath command line option to specify the datafiles directory

Here are the various in game hotkeys :
    Arrow Keys      allow you to move Lester
    Enter/Space     allow you run/shoot with your gun
    C               allow to enter a code to jump at a specific level
    P               pause the game
    Alt X           exit the game
    Ctrl S          save game state
    Ctrl L          load game state
    Ctrl + and -    change game state slot
    Ctrl F          toggle fast mode
    Alt Enter       toggle windowed/fullscreen mode
    Alt + and -     change scaler factor

Credits:
--------

Eric Chahi, obviously, for making this great game.

Contact:
--------

Gregory Montoir, cyx@users.sourceforge.net
Fabien Sanglard, fabiensanglard.net@gmail.com
