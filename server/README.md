# EQBCS - EverQuest Box Chat Server

EverQuest Box Chat Server allows you to issue commands to any or all of your characters from your main window.
It consists of two parts, the server and the plugin.

## Compiling on Linux
Required components:
```bash
sudo apt-get install autoconf -y
sudo apt-get install libtool -y
sudo apt install cmake -y
```
Compile:
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
