# Third-party notices

This project is licensed under the GNU General Public License v3.0 (`LICENSE`). The components
below are compatible with it.

## satrecomp 0.2.0

The toolkit in `satrecomp/` is a copy of [satrecomp](https://github.com/tmcleroy/satrecomp) 0.2.0,
Copyright (C) 2026 Tommy McLeroy, GPL-3.0-only. It is copied by its own
`tools/sync-toolkit.sh` and is not edited here. The local toolkit installation used as the source
of this copy contained no `THIRD_PARTY_NOTICES.md`; this file records the notices that apply to
the components actually built and shipped by this project.

## recomp-common

`recomp-common/` (the in-game menu, settings rules and launcher) is a copy of
[recomp-common](https://github.com/tmcleroy/recomp-common), GPL-3.0, made by
`tools/sync-common.sh`. Its launcher descends from
[wiiparty-recomp](https://github.com/arelkair/wiiparty-recomp) by arelkair
(GPL-3.0-or-later); the files carrying that notice keep it.

## Downloaded at build time (not stored in this repository)

| Component | License | Use |
| --- | --- | --- |
| [SDL 3](https://libsdl.org) 3.4.16 | zlib; Copyright (C) 1997-2026 Sam Lantinga | Window, input, audio and GPU in the port and launcher |
| [Dear ImGui](https://github.com/ocornut/imgui) 1.92.9b | MIT; Copyright (c) 2014-2026 Omar Cornut | Launcher interface, compiled into the launcher |
| [Inter](https://rsms.me/inter/) 4.1 | SIL Open Font License 1.1; Copyright (c) 2016 The Inter Project Authors | Launcher font, compiled into the launcher |
| [zlib](https://zlib.net) | zlib; Copyright (C) 1995-2024 Jean-loup Gailly and Mark Adler | PNG screenshots |

A binary distribution must carry the license texts for SDL 3, Dear ImGui and Inter (the font is
inside the launcher), together with the GPL-3.0 and the offer of source that goes with it.

## Reference/development only

Ymir (GPL-3.0) is the emulator satrecomp uses as a development reference when its unpublished
reference patch is available. Neither Ymir nor that patch is part of this project's builds or
distribution. No reference verification was performed for this port at the time of writing.

## What is not included

No disc image, no part of one, no extracted game files, no BIOS image, no save data, no input
recording, no block database and no generated translated C++. The player supplies their own disc.
