# copilot - A linux server monitoring and managing tool

Hello out there, this project aims an easy-to-use web-based monitoring and administration tool for linux systems.

IT IS NOT READY YET, DONT USE IT ON PRODUCTION AND FEEL FREE TO TEST IT AND GIVE ME FEEDBACK.

### Just, Why ?
Yes, there are many tools out there to manager your linux-systems, but for me it was difficult to setup, all uses
some kind of web-server and magic behind the scenes. So i started this project mainly to manage my home servers and
my servers on the internet from one single Interface. And hey, maybe somebody can use it also :)
There is an angularjs webinterface on my git-repo. This interface is able to select an external node without change of ui/ux,
just switch, manage and switch back

# building / installing

## Update dependencies
Of course you need to get the libs: git submodule update --remote

## CMAKE
Copilotd is build with cmake.

mkdir /tmp/copilotd
cd /tmp/copilotd
cmake <source directory>
make
make install

### CMAKE Options
Away from the normal build, you can decide what to build and install.
Therefore some cmake-options provided:
- DEVELOP=ON ( Default is OFF ) Enable develop mode. THIS IS POTENTIAL DANGEROUSE. DONT USE IT ON PRODUCTION !!!!
  ( Why? Because it disable some securiy features, like random-secret-generation )
- PLUGIN_LTLS=ON ( Default is ON ) Enable libressl-support to connect to other copilotd instances
- PLUGIN_WEBSOCKET=ON ( Default is OFF ) Enable websocket-support for web-client ( use it only on your local machine ! )

#### Example usage

mkdir /tmp/copilotd
cd /tmp/copilotd
cmake <source directory> -DPLUGIN_SSL=OFF
make
make install


# usage
of course:
copilotd --help



## pacman
you need
make
gcc
jansson
libwebsockets
libsodium

## runtime
sudo
nft









