# my toy operating system

## Hacking
Run `nix develop` (flakes and nix-command features must be enabled prior to use).
Then, create build directory with `mkdir build`, cd into it, initialise CMake with `cmake ..`, and finally build the project using `make`.

You can flash the firmware onto the board by running `sudo picotool load -fux main.bin` (-f forces RPI Zero into BOOTSEL mode for flashing, -u flashes only the sectors that have changed, and -x automatically reboots the board into the flashed program afterwards).

## compile-commands.json
The file is initially a broken symlink, but once CMake is ran, it should point to `build/compile_commands.json`.
