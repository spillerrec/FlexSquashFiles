#FxSF CLI interface

TODO: Check interface in other CLI compressors. zip, tar, etc. before finalizing this

## General syntax

Each parameter has a long name `--compress` using two dashes and a short name `-c` using one dash.

Some parameters are standalone flags (e.g. `--autodir`) which never takes an argument, or switches (e.g. `--outpath <filepath>`) which always takes an argument. Arguments are referred to with brackets, real usage should not include them (e.g. `--outpath ~/archive.fsxf`).

##Compressing

Compress `<files...>` and store to `<output-filepath>`. Path to folders will include the whole folder

```bash
FxSF --compress -outpath <output-filepath> <files...>
FxSF -c -o <output-filepath> <files...>
```

Compress `<files...>` and default `<output-filepath>` to `./<dir>` where `<dir>` is parent directory of first file

```bash
FxSF --compress <files...>
FxSF -c <files...>
```

Compress files and add them into a root folder named `<output-filepath>`

```bash
FxSF --compress --autodir <files...>
```

##Extracting

Extract to directory (here)

```bash
FxSF --extract <FxSF files...>
FxSF -x <FxSF files...>
```

Extract to directory, auto-create directory
(if multiple file in root, create directory with name of archive, else just extract)

```bash
FxSF --autodir --extract <FxSF files...>
FxSF -ax <FxSF files...>
```

Extract to custom path/directory

```bash
FxSF --extract --outpath <Directory> <FxSF file>
FxSF -xo <Directory> <FxSF file>
```

Extract to directory

## Selection

For limiting the effect of commands such as `--extract`and `--list`.

```bash
FxSF --select <regex> <...>
FxSF -s <regex> <...>
```



## Other

Show current version, FxSF version, compatibility info, and supported compression algorithms

```bash
FxSF --version
FxSF -v
```

Show the file contents of a FxSF file

```bash
FxSF --list <FxSF file>
```

Verify using CRC

```bash
FxSF --verify <FxSF files...>
```

