# GaussianBeam

Fork of the classic GaussianBeam app developed by Jérôme Lodewyck and available on [SourceForge](https://sourceforge.net/projects/gaussianbeam/). This fork is mostly for my own purposes, with minor tweaks for now. The code is now built with LTS Qt 5.15.12 and I intend to drop support for version 4 as well as port to version 6.5.0. This requires replacing Qt XmlPatterns unfortunately.

I also removed French translation since I don't speak it and don't intend to maintain it. It also created some build errors for me, but you can contribute the original files if you want to bring it back :)

## Building from source

### Windows

1. Download and run the [Qt Online Installer](https://www.qt.io/download-qt-installer-oss)
2. Select a custom install and toggle the **Archive** category
3. Install Qt Creator and Qt 5.15.12 Sources and 64bit MinGW
4. Open the `GaussianBeam.pro` project file using QtCreator

Alternatively you may try the old instructions in [INSTALL](./INSTALL)