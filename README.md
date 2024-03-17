<div align="center">
  <h3 align="center">Mojang2JSON</h3>

  <p align="center">
    Converts <a href="https://piston-meta.mojang.com/mc/game/version_manifest_v2.json">Mojang mappings</a> to a clean JSON format
  </p>
</div>

## What is this

This is a work in progress project I had made a while ago. For now it only parses Mojang mappings into a simpler format, but I plan to add later a system which will resolve the mappings by injecting into Minecraft and then convert it to the required JSON format.

## Build

Simply head into the `cmake/` folder, and run the build script (`.sh` for Linux and `.bat` for Windows). The output will be stored inside `cmake/bin/Mojang2JSON`.

## Usage

Simply execute Mojang2JSON while passing a text Mojang mappings which you can get from their API.