# OBS Overtime

OBS Overtime adds production timers to OBS Studio projector windows. The overlay is a separate window, so it stays visible to the crew without appearing in scenes, streams, or recordings.

## Features

- Streaming and recording durations, plus elapsed and remaining time for a selected media source, displayed as `HH:MM:SS`.
- Independent visibility, position, and edge offset for each timer.
- Font, text color, background color, opacity, and padding controls.
- Media end warning and recurring stream/recording warnings with configurable color, opacity, interval, and duration.
- Media source selection dock, including an option to show media timers only while the selected source is active and playing.
- Settings saved between OBS sessions.

## Download and install

Download the [Windows x64 release](https://github.com/menacius/OBS-Overtime/releases/latest) or visit the [OBS Overtime product page](https://software.omniatv.com/overtime/). OBS Studio 31 or newer is required.

1. Close OBS Studio and extract the ZIP.
2. Copy the ZIP's `obs-plugins` and `data` folders into the OBS Studio installation directory.
3. Restart OBS. Open a projector window, then use **Tools → OBS Overtime Settings** to configure the overlay.
4. For media timers, choose sources in the **OBS Overtime Media Selection** dock.

The Windows release contains the plugin DLL and English locale data. Linux/X11 builds are possible from source but are not currently distributed as a prebuilt package.

## Build from source

You need CMake 3.20+, a C++17 compiler, Qt 6 (Core, Gui, Widgets), and OBS Studio/libobs and frontend API development files. On Windows, use `scripts/build-windows.ps1`; it can locate OBS and Qt dependencies or accept explicit `-ObsSdkDir` and `-Qt6Path` values. Build a configured CMake tree with `cmake --build <build-dir> --config RelWithDebInfo`.

## Source layout

| Path | Responsibility |
| --- | --- |
| `src/plugin-main.cpp` | Module load/unload, menu action, dock registration |
| `src/plugin-config.*` | Persistent settings |
| `src/overlay-controller.*` | Timer updates and overlay lifecycle |
| `src/overlay-window.*` | Projector overlay rendering |
| `src/projector-tracker.*` | Projector discovery and geometry |
| `src/media-manager.*` | Media source selection and timing |
| `src/settings-dialog.*` | Settings UI |
| `src/media-dock.*` | Media source dock |
| `src/time-utils.*` | Duration formatting |
