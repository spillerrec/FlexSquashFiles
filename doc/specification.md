The format contains 3 main segments:
- HeaderHeader, a very small pre-header
- Header, all meta-data required to access and decompress files
- Data, the compressed files referenced in the header

TODO: Everything here is little endian. Any reason for big-endian?
TODO: Is alignment important?

HeaderHeader
--------

| Field            | Type/Size | Description                           |
|------------------|-----------|---------------------------------------|
| Magic            | char[4]   | Always "FxSF"                         |
| Custom Magic     | char[4]   | User-definable magic code             |
| Header size      | uint32    | Lenght of the header segment in bytes |
| Main header size | uint32    | Lenght of first compressed segment    |

If `Custom magic` contains anything else than `"\0\0\0\0"` then the format is a custom extension.
Applications not recognizing this string are not allowed to modify the file without removing any custom features and should obviously warn the user in the progress. (Expert tools may allow modifications specified by the user.)

If `Header size` is `0`, then the archive is empty and it should not attempt to parse the header segment.


**Rationale**
Including an user-definablee magic code makes it easy to identify a custom file format using this archive format as a base. ZIP based formats need to resort forcing the first file to be an uncompressed recognizable string, which even then is not garantied to be at a fixed position. With FxSF, checking the first 8 bytes is all there is needed for a codec to tell if it can decode a file. This is also wastly less space required than ZIP which needs to specify a file header (including a filename).
It also makes ensures that conforming implementations know that special restrictions might apply.


Header
------
The header is ZSTD compressed with dictionary compression.

| Field       | Compression | Optional | Desccription           |
|-------------|-----|-----|-------------------------------------|
| Main header | Yes | No  | Contain file and folder information |
| Text        | Yes | No  | Contains all text strings           |
| Checksums   | No  | Yes | CRC32 for verification              |
| User-data   | N/A | Yes | Area for extensions                 |

The main header and text segments are indidually ZSTD compressed each with global dictionaries which are stored in the decoder. The ZSTD frames must contain the decompressed sizes. The compressed size of `Main header` (used for decompressing) is defined in the `HeaderHeader`, the compressed size of the `Text` segment is specified in the `Main header`.

`Checksums` contains an array of all files in the order the files are defined in the main header. Each checksum is 4 bytes in CRC32 format. (TODO: specify CRC32)
It is optional, which is defined in the main header.

`User-data` is a space custom extensions can store anything, and extends from the end of the checksums to the `header leght`. If no custom extension is defined, this segment must not be present.

**Rationale**
The separation of the text strings and rest of the header is of two reasons. Filenames/paths are the only thing which have a variable lenght, by storing the text separately we can read the rest of the headers directly into the arrays with a few calls. Furthermore we can load the entire text segment into memory and easily create a text table to avoid creating lots of small allocations.
The second reason is that text has a significantly different probability distribution compared to the rest of the header. By using a dictionary trained on (English) file paths, we should optimistically see an improvement over compressing both the text and binary data interleaved and with a shared dictionary.

Checksums are pretty much random, so I don't expect to see any improvement by compressing it, instead I fear it would negatively affect the compression if stored in the Main header.

Main header
-----------
The segment is ZSTD dictionary compressed (to be defined). Both uncompressed and compressed size must be a part of the ZSTD header (confirm if this is also the case for compressed size).

The decompressed data contains the following:
- Decoding info
- File headers
- Folder headers
- Text lenghts

**Decoding info**

| Field        | Type   | Notes                                 |
|--------------|--------|---------------------------------------|
| FxSF version | uint8  | Currently 0 (unstable)                |
| User version | uint8  | Custom extension version, 0 if not    |
| Flags        | uint16 | Decoding flags, see below             |
| File count   | uint32 | The amount of files                   |
| Folder count | uint32 | The amount of folders                 |
| Text size    | uint32 | The size of the text segment in bytes |

*Decoding flags:*

| Bit  | Description        |
|------|--------------------|
|  0   | Checksums disabled |
| 1-15 | Reserved, always 0 |

**File header**
Next follows `File count` file headers. The order is used to coorelate it with the text strings.

| Field           | Type   | Description |
|-----------------|--------|-------------|
| Offset | uint64 | The offset in the data section to where the file data is stored |
| File size       | uint64 | The uncompressed file size |
| Compressed size | uint64 | The actual size for storing it in the archive |
| Folder          | uint32 | ID of the folder this file is placed in |
| compression     | uint8  | Compression method used |
| flags           | uint8  | Misc stuff |
| reserved        | uint16 | Reserved for future versions |
| user-data       | uint64 | Room for custom extensions |

The `Offset` is the amount of bytes from the start of the data section to where the file data is stored, using `Compressed size` amount of bytes. Files may be placed anywhere in the data section, in any order. Areas in the data section not covered by any files must be zero. (**Rationale:** Force a consistent handling of undefined areas, and many file systems support zero block optimization.)

The file header is slightly processed in order to compress better. The `File size` have been substracted with the `Compressed size`. The `Offset` have been substracted with the ending of the previous file, i.e. `offset - (prev.offset + prev.compressed_size)`.
The application must support proper wrap-around in case `Offset` or `Filesize` becomes negative, though an implementation should always switch to `None` compression in case the selected method increases the filesize.

Compression defines the compression method based on the following ids:

| ID | Method  | Notes |
|----|---------|------|
| 0  | None    | File is stored uncompressed |
| 1  | ZSTD    | File is compressed with Zstandard |
| 2  | Deflate | |
| 3  | LZMA    | |

The range `128-255` can be used by custom extensions to define their own compression methods.

TODO: Support dictonary compression?

| Bit | Description |
|-----|-------------|
|  0  | Compressed with next if set |
| 1-7 | Reserved |

To compress several small files efficiently, multiple files may be stored in a single compressed stream. In this case, the first file in the stream has the flag set and the `Compressed size` is the size of the entire compressed stream. The later files with the flag set have a `Compressed size` of zero with the same `Offset` as the filest file, and the last file included in that compressed stream will have the flag unset.
TODO: Should we even support this? It complicates decoding quite a bit and the whole point of this format is to allow quick access to abbritrary files, using big compressed chunks defeats this. Perhaps we can get decent results by creating dictionaries?


`reserved` must be zero.

`user-data` must be zero unless this is a custom extension.

**Folder header**

| Field  | Type   | Description             |
|--------|--------|-------------------------|
| Parent | uint32 | ID of the parent folder |

The root folder is identified by its parent folder being itself. To improve compression the most refered folder should have the ID `0`. Order specifies the folder ID.

TODO: Check how much this matter in reality. Might go for an order which is more efficient to process.

**Text lenght**

| Field  | Type   | Description                           |
|--------|--------|---------------------------------------|
| Lenght | uint16 | Amount of bytes including ending "\0" |

TODO: Forced text encoding to UTF-8 even for custom extensions?

Text
----

The compressed size of the text segment is stored in the `HeaderHeader`. The uncompressed size is the sum of the text lenght, and the position of each text string is right after the previous one in the order defined in the text lenght.

Checksums
---------

If `Checksums disabled` is false, this segment contains `Files count` CRC32 checksums (uint32) without any compression applied.

The option of disabling checksums is mainly there for custom extensions or use-cases which have some other way of verifying the integrety of the archive. Consider for example a package system which saves a checksum for each package in its repository.

User-data
---------
This segment is the size of `Header size` substracted with the sizes of all other segments, and may only exists in custom extensions. The contents of the segment is entirely up to the custom extension.

**Rationale:**
In case the 64 bits in the `File header` isn't enough, this segment can be used to provide extended meta-data to each file, shared dictionaries, or other info useful before starting to decode the files. Custom extensions could use an ordinary file with a specific name or position instead though.