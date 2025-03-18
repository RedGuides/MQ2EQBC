# EQBCS - EverQuest Box Chat Server

EverQuest Box Chat Server allows you to issue commands to any or all of your characters from your main window.
It consists of two parts, the server and the plugin.

## Compiling on Linux
Install git if not allready: ```sudo apt install git -y```

Create git directory: ```sudo mkdir /repo``` (be sure to verify permisions)

Compiler components:
```bash
sudo apt-get install autoconf -y
sudo apt-get install libtool -y
sudo apt install cmake -y
```
Clone repos:
```bash
git clone https://github.com/macroquest/macroquest.git
cd macroquest
git submodule update --init
git clone https://github.com/RedGuides/MQ2EQBC.git plugins/MQ2EQBC
cd plugins/MQ2EQBC
git submodule update --init
cd server
```
Compile the server:
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
