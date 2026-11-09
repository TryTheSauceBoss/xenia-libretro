# Xenia Edge (Xendroid+) libretro core

A [libretro](https://www.libretro.com/) core of [Xenia Edge](https://github.com/has207/xenia-edge), the Xbox 360 emulator, for RetroArch and other libretro frontends, with fixes and speed-ups ported from [Xendroid+](https://github.com/phforner0/Xendroid-Plus), Xenia's Android port.

It is a core of its own, installed beside the plain Xenia Edge core rather than in its place: it shows as "Xenia Edge (Xendroid+)" in RetroArch, which keeps its options and overrides apart, its file is `xenia_edge_xendroid_libretro`, and it keeps everything in `system/Xenia-Edge-Xendroid/`. To carry saves and profiles over from Xenia Edge, copy `system/Xenia-Edge/content/` (and `xbla_folders.txt` or `xbl_profilename.txt`, if you made them) into `system/Xenia-Edge-Xendroid/`.

The emulator is upstream's: this repository follows xenia-edge and adds the libretro frontend in `libretro/`. It holds the core only; upstream's standalone UI, packaging and CI are not part of it (the paths are listed in `.upstream-excluded`). The version the core reports is upstream's, with the upstream commit it is merged to (`upstream.version`). The core is currently on xenia-edge commit 8dc77b0 of 7 October 2026.

## Downloads

This core is not on libretro's buildbot (the plain Xenia Edge core is, in RetroArch's Online Updater > Core Downloader); install it from a build's artifact with Load Core > Install or Restore a Core. Every push to the `libretro` branch is built in [Actions](https://github.com/TryTheSauceBoss/xenia-libretro/actions/workflows/libretro.yml), macOS (Apple Silicon) included, and every push to `xendroid-port` for Android; the builds are the artifacts of a run, and downloading them needs a GitHub login.

- **Windows:** `xenia_edge_xendroid_libretro.dll`, which needs the Visual C++ 2015-2022 runtime. It carries `dxil.dll`, Direct3D 12's shader validator, inside and writes it to `system/Xenia-Edge-Xendroid/D3D12/` on start.
- **Linux:** `xenia_edge_xendroid_libretro.so`, glibc 2.35 or newer; besides the C/C++ runtime it uses only lz4 and fontconfig; no X11 or Wayland libraries. ARM64 uses the a64 CPU backend.
- **macOS:** `xenia_edge_xendroid_libretro.dylib`, Apple Silicon only. The x64 CPU backend needs the address space below 4 GB, which a core loaded into RetroArch cannot free, so there is no Intel build.
- **Android:** `xenia_edge_xendroid_libretro_android.so`.

## Video

The core option Graphics API picks Xenia's GPU backend: Vulkan, or on Windows Direct3D 12. The core asks RetroArch for that kind of context, and RetroArch switches its video driver to match when it is allowed to (Settings > Video > Output). Direct3D 12 runs on the system's Direct3D 12 runtime: the Agility SDK runtime that standalone Xenia uses can only be loaded by an executable that exports it, which RetroArch does not.

## Content

Xbox 360 games as a disc image (`.iso`), an executable (`.xex`), or a `.zar` / `.xcp` package. No BIOS or firmware is needed.

Xbox Live Arcade and Games on Demand games are packages without a file extension, which RetroArch's file browser does not show. Put them in `system/Xenia-Edge-Xendroid/xbla/`, in any layout (the console's own `<Title ID>/000D0000/<package>` is fine): each time RetroArch loads the core, it writes `<game name> [<Title ID>].m3u` for each game in that folder, and those are opened like any other game. Games kept elsewhere, such as on an SD card, are found too: list their folders in `system/Xenia-Edge-Xendroid/xbla_folders.txt`, one a line (the core writes that file with only comments when it is missing). Their `.m3u` files still go in `system/Xenia-Edge-Xendroid/xbla/`, so only the small shortcuts take internal storage. The core rewrites or deletes its own `.m3u` files as the packages change (not while a listed folder, such as a removed card, cannot be opened), and leaves other files there alone.

Original Xbox games run through XeFu, the Xbox 360's own emulator of the original Xbox, which upstream Xenia Edge can run. It needs XeFu's files from a console (`xbox.xex` and the `xefu*.xex` builds of the system partition's `Compatibility` folder) in `<system>/Xenia-Edge-Xendroid/xefu/`; the game is opened as a disc image (`.iso`), a full dump or one rebuilt to the game partition alone. Only the games the console itself could run have a chance.

Everything the core keeps is in `system/Xenia-Edge-Xendroid/`: saves, title updates and DLC under `content/`, laid out as in standalone Xenia's content folder; the shader and other caches under `cache/`; and Xenia's log, `xenia.log` (none with Log Level None, the default). Errors and warnings from it also appear in RetroArch's log. The core option Apply Title Updates turns updates off.

Games that need a signed-in profile get one: the core signs in the first profile under `content/`, or creates one when there is none, named after the first line of `system/Xenia-Edge-Xendroid/xbl_profilename.txt` ("Player" if the file is missing, which the core then writes there).

### DLC and add-ons

To install DLC, title updates or other add-on packages, put the files in `system/Xenia-Edge-Xendroid/install/`, the folder the core makes when it starts. They can be in its root or in any subfolders, so a download can be copied in with its own folder structure.

At the next start of the core, with any game, the core installs every package it finds there into `content/`, as standalone Xenia Edge's Install Content does, and then deletes it from `install/` along with the folders left empty. This happens before the game starts, so DLC for the game being started works right away. A file that is not a package, or fails to install, stays in `install/`, and RetroArch shows its name (the reason is in the log).

## Building

With upstream's build dependencies (see upstream's documentation), from a checkout with its submodules:

```
git clone --recursive https://github.com/TryTheSauceBoss/xenia-libretro.git
cd xenia-libretro
./xenia-build.py slang
./xenia-build.py fetchdata
./xenia-build.py build --config=Release --target=xenia-libretro
```

The CI workflow `.github/workflows/libretro.yml` builds every platform, including the Android cross build.

## License

BSD, as Xenia. See [LICENSE](LICENSE).
