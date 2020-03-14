Higan-Core
==========

OpenEmu Core plugin for Higan

This fork of the Higan core enables Super Game Boy support

* BIOS directory must contain valid `Super Game Boy (World).sfc` and `sgb.rom` files, sourced from the Japanese "Super Game Boy 2"

![alt text](https://www.mariowiki.com/images/8/8b/Donkey_Kong_Super_Game_Boy_Screen_9.png "Donkey Kong '94'")


### Cloning & Compiling

```
git clone --recursive https://github.com/TRIFORCE89/OpenEmu.git
cd OpenEmu
xcodebuild -workspace OpenEmu.xcworkspace -scheme "Build & Install Higan (Performance)" -configuration Release
```
