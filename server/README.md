# EQBCS - EverQuest Box Chat Server

EverQuest Box Chat Server allows you to issue commands to any or all of your characters from your main window.
It consists of two parts, the server and the plugin.

## Compiling on Linux
```bash
cd contrib/safeclib
./build-aux/autogen.sh
./configure
make
sudo make install

cd ../..
mkdir build
cd build
cmake ..
make
```


## Original Author

Originally written by Omnictrl
