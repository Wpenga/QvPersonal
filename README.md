# Qv2ray

Qv2ray is a cross-platform Qt frontend for V2Ray.

This repo is my personal rewrite of Qv2ray using [Qv2rayBase](https://github.com/moodyhunter/Qv2rayBase), hard forked from `Shadowsocks-NET/Qv2ray`

## OS support status:

- ArchLinux: **Tested and Working**
- Windows 10/11: **Tested and Working**, build system.
- macOS 12 Beta: **Tested and Working**
- macOS 11: **Untested**, build system.
- Ubuntu 20.04: **Untested**, build system.
- Others: **Untested**

## Compatibility Issues

- **This version, and all future versions of QvPersonal are not compatible with the configuration format of `Qv2ray/Qv2ray` and `Shadowsocks-NET/Qv2ray`, DO NOT mix these installations in one host machine.**
- Since the project is a fork of `Shadowsocks-NET/Qv2ray`, the Xray-related bug check code is not removed. 
  - **unlike what they said, the code, actually, will NOT crash if your configuration was correct, which means, it ONLY crashes when the configuration check fails**
- Since QvPersonal uses a plugin interface version 5, no plugins from `Qv2ray/Qv2ray` and `Shadowsocks-NET/Qv2ray` will work with this repo.

# License

[![License: GPL v3](https://www.gnu.org/graphics/gplv3-127x51.png)](https://www.gnu.org/licenses/gpl-3.0)
