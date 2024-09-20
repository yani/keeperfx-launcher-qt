# KeeperFX Launcher

Modern cross platform KeeperFX launcher made using Qt6 C++. Based on [ImpLauncher](https://keeperfx.net/workshop/item/410/implauncher-beta).
This is still a Work in Progress and close to nothing is working. Everything can and will change. Nothing is final. 

Codename: CutieLauncher

#### Building

- Get QT Creator
- Setup a local build kit
- Load the project (by opening CMakeLists.txt)
- Build it

#### Deployment

Deployment is for later. We will probably statically build Qt6 for Windows, and dynamically for Unix.

The current third party libraries are mostly dynamic:
- 7z.so / 7z.dll
- libLIEF.so / libLIEF.dll

We might be able to build these during the build process and include them automatically

#### Discord

Discord thread: https://discord.com/channels/480505152806191114/1285667371272376430
You need to have access to the KeeperFX development channel on the Keeper Klan Discord to access it.
Just ask if you need it
