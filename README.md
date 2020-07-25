# AzurLaneDecTools
Tools to handle LuaJIT files in Azur Lane 5.0. This is forked from the original repositories ([here](https://github.com/jspzyhl/AzurLane5.0-uabDec) and [here](https://github.com/jspzyhl/AzurLane5.0-bcDec)), which only builds on Windows.

## Build

* Only supports 32-bit x86. The original code uses super unsafe pointer operations that makes a few assumptions it has no business making, like assuming `size_t` is an alias for `unsigned int` (which it isn't on x86_64).
* The compiled binaries will run on x86_64, as usual for 32b-bit x86, but if compiled natively to 64-bit x86, it will segfault upon execution. I'm way too lazy to fix this behavior so it'll have to do for now.
* If you're on Linux or another POSIX system you should be able to navigate to the base directory and run `make` out of the box.
* GCC you use must support `-m32` and `-march=i386` which usually means you need some sort of multilib GCC. On Arch Linux this is the `gcc-multilib` package but I'm not sure what it is on other distros.

## Usage

This spits out binaries `uabdec` and `bcdec` which do two different things.

You have to obtain `scripts32` from the game files on your own, which can usually be taken from an android filesystem. It's located at `Android/data/com.YoStarEN.AzurLane/files/AssetBundles/scripts32`. Copy it over locally, however you wish.

From a terminal, you can decrypt the `scripts32` archive using `uabdec`:
```
$ uabdec ./scripts32 scripts32-dec
```

This creates a decrypted unity asset bundle which can be extracted with most unity bundle extractors. In particular, `unityextract` is a PyPI module, and you can install it with `python3 -m pip unityextract`. You can then dump the Lua files with:

```
$ mkdir -p extract/
$ unityextract --text --outdir extract/ scripts32-dec
```

This creates a bunch of encrypted luajit files in `extract/`, which we then can batch decrypt with `bcdec`, the other binary file from this repo.

```
$ mkdir -p lualj/
$ find extract/ -name '*.lua.bin' | parallel bcdec {} lualj/
```

This produces several compiled luajit files in `lualj/`. You don't need to use `parallel` here but it makes the process faster. `find -exec` is single threaded.

Now, we use [`luajit-decompiler`](https://gitlab.com/znixian/luajit-decompiler) to actually decompile the LuaJIT files. These LuaJIT files have a slightly mangled magic signature, but this decompiler ignores that so It Works (TM). `luajit-decompiler` has a batch option to operate on a directory but since it doesn't multithread, I'm going to use parallel again here.

```
$ mkdir -p lua-sauce/
$ find lualj/ -name '*.lua.lj' | parallel luajit-decompiler --file={} --output=lua-sauce/{/.} --catch_asserts
```

This should produce several lua files in `lua-sauce`. Behold:

```
$ head luasauce/ship_data_statistics.lua 
pg = pg or {}
pg.ship_data_statistics = {
    [100001] = {
        raid_distance = 0,
        oxy_max = 0,
        name = "Universal Bulin",
        type = 1,
        oxy_cost = 0,
        skin_id = 100000,
        english_name = "UNIV Universal Bulin",
```
