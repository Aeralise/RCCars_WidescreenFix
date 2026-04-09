# RC Cars WidescreenFix

ASI plugin to improve widescreen support for RC Cars (2003)

## ✔ Features

- `Main menu resolution` - no more forced 800x600 in menus, set the same resolution both in ini file and game settings
- `Aspect Ratio` - Fixes in-game camera aspect ratio, the default game stretching 4:3 ratio on any resolution set. Also fixes car preview in menus
- `FOV` - Wider (or narrower) in-game camera field of view, 16:9 aspect ratio crops the camera angle in default game

## ✍ To do:

- `FPS` - Above 70FPS the game (including menu animations) will run in slow motion. Use built-in frame limiter, GPU driver settings or RTSS.
- `HUD Stretching fix` - major of the UI elements are stretching, but some other (in Split-Screen mode) renders in correct aspect and size
- `Split-Screen cameras fix` - cameras in Split-Screen mode utilizes parameters from **Camera.ini** placed in **Settings** folder (see [RC Cars Modding Steam guide](https://steamcommunity.com/sharedfiles/filedetails/?id=3392406916)) 
- `Full game resolution` - both in-menu and in-race resolution set in one place in .ini file
- `Display refresh rate` - by default, game utilizes 60Hz (sometimes even 50Hz) display refresh rate triggering monitor to switch. For now I recommended to use [dgVooDoo](https://github.com/dege-diosg/dgVoodoo2/releases) to set your native resolution and refresh rate.
- `Game speed fix` (in theory) - the game is running a little bit slower than it should (comparing between in-game race time and external tools e.g. LiveSplit) at any fps count (also the higher fps - the slower game)

## Install & Usage

1. Put the plugin files [(see Releases page)](https://github.com/Aeralise/RCCars_WidescreenFix/releases) and [ThirteenAG's Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) to game directory.
2. Open .ini file and set **Width and Height** of your display resolution, choose the aspect (16:9 by default) and FOV (210 is default for 16:9). In game settings, choose the same resolution you set in the .ini.
