# CSS-CS2Fixes

This plugin adds back some essential patches from CS2Fixes that are useful for surf servers.

- Minimizes rampbugs
- trigger_push fix
    - Reverts trigger_push behaviour to that seen in CS:GO
- Water fix
    - Fixes being stuck to the floor underwater, allowing players to swim up
- Navmesh lookup lag fix
    - Some maps with built navmeshes would cause tremendous lag

This plugin isn't perfect and rampbugs will continue to occur until Valve decides to finally fix them.

Fixes taken from:
- [CS2Fixes](https://github.com/Source2ZE/CS2Fixes)
- [Cs2Fixes-Rampbugfix](https://github.com/Interesting-exe/CS2Fixes-RampbugFix)

## Installation

- Install [Metamod](https://cs2.poggu.me/metamod/installation/)
- Download the [latest release package](https://github.com/Source2ZE/CS2Fixes/releases) for your OS
- Extract the package contents into `game/csgo` on your server

## Compilation

A guide on how to compile CS2-SurfFixes for Linux using VSCode and WSL.

#### Prerequisites
- [WSL](https://learn.microsoft.com/en-us/windows/wsl/install)
- [VSCode WSL Extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-wsl)
- [Git](https://git-scm.com/downloads)
- [Python3](https://www.python.org/downloads/)

**ALL PATHS ARE ASSUMED FOR EVERYTHING BEING PLACED IN THE /ROOT DIRECTORY**

#### File Structure
```bash
mkdir alliedmodders
cd alliedmodders
```

#### Ubuntu 20.04 setup
```bash
sudo apt update
sudo apt install python3-pip
sudo apt-get install clang

# make sure we can use pip installed packages anywhere
echo "export PATH=\"$HOME/.local/bin:$PATH\"" >> ~/.bashrc
. ~/.bashrc
```

#### DownloadAMBuild
```bash
git clone https://github.com/alliedmodders/ambuild
pip install ./ambuild

# In case of an error try
# cd ambuild && python setup.py install && cd ..
```

#### Instructions
```bash
mkdir hl2sdk-root
git clone --branch cs2 https://github.com/alliedmodders/hl2sdk/ hl2sdk-root/hl2sdk-cs2
cd hl2sdk-root/hl2sdk-cs2
git checkout 74ea9343746475b148f5f53679516dc9cd2b336b

git clone https://github.com/alliedmodders/metamod-source --recursive
cd metamod-source

echo "export HL2SDKCS2=/root/alliedmodders/hl2sdk-root/hl2sdk-cs2" >> ~/.bashrc
echo "export MMSOURCE112=/root/alliedmodders/metamod-source/" >> ~/.bashrc
. ~/.bashrc
```

#### Plugin Compilation
```bash
git clone https://github.com/Fallen-Networks/CS2-SurfFixes && cd CS2-SurfFixes
git submodule update --init --recursive

mkdir build && cd build
python3 ../configure.py --enable-optimize --sdks cs2
ambuild
```

Copy the contents of package/ to your server's csgo/ directory.
