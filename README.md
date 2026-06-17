# unreal-editor-window-splitter

This tool helps with Unreal Engine on KDE / X11 by separating editor sub-windows that normally end up grouped under one main process. Those sub-windows then show up individually in the KDE taskbar instead of being hidden inside one entry.

## What it does

Unreal Engine often creates multiple internal windows like panels, cameras, or editors that are attached to one main window. KDE tends to treat them as a single grouped application.

This tool:

- detects Unreal Editor windows
- removes transient window relationships
- forces windows to be treated separately by the window manager
- makes those windows appear in the KDE taskbar as separate entries

## Requirements

- Linux (X11 only, not Wayland)
- Xlib development headers
- KDE Plasma
- Unreal Engine running on X11

Install on Arch Linux:

```bash
sudo pacman -S libx11
```

## Usage

### Run manually

```bash
./unrealeditor-window-splitter /path/to/UnrealEditor /path/to/project.uproject
```

Example:

```bash
./unrealeditor-window-splitter \
/mnt/niko/Linux-Unreal/Unreal_My_Beloved/Linux_Unreal_Engine_5.6.1/Engine/Binaries/Linux/UnrealEditor \
"/mnt/mainwindows/Users/niko_messerschmidt/Documents/Unreal Projects/An Unreal Rake Remaster/AnUnrealRakeRemaster.uproject"
```

## Desktop entry usage

You can integrate the splitter directly into your `.desktop` file so it starts together with Unreal Engine.

Example:

```ini
[Desktop Entry]
Name=Unreal Editor
Comment=Run Unreal Editor
Exec=sh -c '/mnt/niko/Linux-Unreal/Unreal_My_Beloved/Linux_Unreal_Engine_5.6.1/unrealeditor-window-splitter & exec /mnt/niko/Linux-Unreal/Unreal_My_Beloved/Linux_Unreal_Engine_5.6.1/Engine/Binaries/Linux/UnrealEditor "$1"' sh %f
Icon=/mnt/niko/Linux-Unreal/Unreal_My_Beloved/Linux_Unreal_Engine_5.6.1/unreal-engine.png
Terminal=false
Type=Application
Categories=Application;
```

What this does:

- starts the splitter in the background
- launches Unreal Editor normally after that
- passes the project file (`%f`) into Unreal
- keeps both running under one desktop entry

This setup ensures the splitter is always active whenever Unreal starts from the launcher.

## How it works

It scans the X11 window tree, finds Unreal Editor windows using WM_CLASS and window titles, then removes the WM_TRANSIENT_FOR relation so the window manager stops grouping them under one parent window.

It also listens for new windows through X11 events and applies the same logic when they appear.

## Notes

- Only works on X11
- Behaviour depends on KDE window management rules
- Unreal Engine updates can change window behaviour
- Experimental tool, expect edge cases

## Status

Works for typical Unreal Engine setups on KDE Plasma.
