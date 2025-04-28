# KeeperFX Launcher

Modern cross platform KeeperFX launcher made using Qt6 C++. Based on [ImpLauncher](https://keeperfx.net/workshop/item/410/implauncher-beta).

Codename: **CutieLauncher**

## Download

You can download it from the KeeperFX workshop: https://keeperfx.net/workshop/item/739/cutielauncher-alpha

## Building

- Get QT Creator
- Setup a local build kit (Qt6+)
- Load the project (by opening CMakeLists.txt)
- Build it

## Deployment

There is currently a script that uses docker to create a build environment and then uses it to build and package the launcher. 

This script is currently Linux only and should probably only be used for building the actual release versions.

```
./compile-win64s.sh
```

## Linux

A Linux version of the launcher will be released when KeeperFX itself works natively on Linux. All of the launcher's functionality already works on Linux but 
before we know how everything will be shipped, there's not much reason to already be releasing these builds. 

## Discord

Discord thread: https://discord.com/channels/480505152806191114/1285667371272376430  
You need to have access to the KeeperFX development channel on the Keeper Klan Discord to access it.
Just ask if you need it

## Translations

The launcher uses translations and should have at least the languages that are also available in the game.
Feel free to submit PRs for translations or contact Yani on the Keeper Klan Discord to work on the translations.
The best way of translating is using a tool such as [POEdit](https://poedit.net/).
Because it allows you to update translation files (PO) with the original translation template (POT).

You can find the used POT file here: [i18n/translations.pot](/i18n/translations.pot)

| Language | Code | Status | Maintainer |
|---|---|---|---|
| English | EN | ✅ | [Yani](https://github.com/yani) |
| Dutch | NL | ✅ | [Yani](https://github.com/yani) |
| Italian | IT | | |
| French | FR | | AncientWay |
| Spanish | ES | | |
| German | DE | | Aqua, Dofi |
| Polish | PL | | |
| Swedish | SV | | |
| Japanese | JA | | [AdamPlenty](https://github.com/AdamPlenty) |
| Russian | RU | | |
| Korean | KO | | Desert |
| Chinese (Simplified) | ZH-HANS | | [JieLeTian](https://github.com/jieletian) |
| Chinese (Traditional) | ZH-HANT | | |
| Czech | CS | | |
| Latin | LA | | |
| Ukrainian | UK | | Mr.Negative |

## License

This project is licensed under the GNU General Public License v2.0. Feel free to use, modify, and distribute it according to the terms of this license.
