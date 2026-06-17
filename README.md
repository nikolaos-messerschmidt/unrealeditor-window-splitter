# unreal-editor-window-splitter

This tool helps with Unreal Editor on KDE / X11 by separating editor sub-windows that normally end up grouped under one main process. Those sub-windows then show up individually in the KDE taskbar instead of being hidden inside one entry.

## What it does

Unreal Editor creates multiple internal windows that are attached to one main window. KDE tends to treat them as a single grouped application.

(Funnily enough, this works natively on Windows.)

This tool:

- detects Unreal Editor windows
- removes transient window relationships
- forces windows to be treated separately by the window manager
- makes those windows appear in the KDE taskbar as separate entries

## Requirements

- Linux (X11 only, not Wayland)
- Xlib development headers
- KDE Plasma
- Unreal Editor running on X11

### Arch Linux
```bash
sudo pacman -S libx11
```

### Debian / Ubuntu
```bash
sudo apt install libx11-dev
```

### Fedora
```bash
sudo dnf install libX11-devel
```

### openSUSE
```bash
sudo zypper install libX11-devel
```

---

## Build

```bash
gcc -o ue_pid_spoofer ue_pid_spoofer.c $(pkg-config --cflags --libs x11) -Wall
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
"/mnt/mainwindows/Users/niko_messerschmidt/Documents/Unreal Projects/Test proj 1/test_proj_1.uproject"
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

### DONT FORGET TO CHANGE THE PATHS TO YOURS. THIS WILL ONLY WORK ON MY SYSTEM

What this does:

- starts the splitter in the background
- launches Unreal Editor normally after that
- passes the project file (`%f`) into Unreal
- keeps both running under one desktop entry

This setup ensures the splitter is always active whenever Unreal starts from the .desktop file.

## How it works

It scans the X11 window tree, finds Unreal Editor windows using WM_CLASS and window titles, then removes the WM_TRANSIENT_FOR relation so the window manager stops grouping them under one parent window.

It also listens for new windows through X11 events and applies the same logic when they appear.

## Notes

- Only works on X11
- Behaviour depends on KDE window management rules
- Unreal Editor updates can change window behaviour
- Experimental tool, expect edge cases

## Status

Tested on UE 5.6.1, Plasma 6.6.5 and 7.0.11-arch1-1 (64-bit)

i cant promise that it works on other versions tbh.. 
