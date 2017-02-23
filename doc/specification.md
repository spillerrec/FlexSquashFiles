The format contains 3 main segments:
- HeaderHeader, a very small pre-header
- Header, all meta-data required to access and decompress files
- Data, the compressed files referenced in the header

Everything here is little endian. Any reason for big-endian?

HeaderHeader
--------

| Field        | Type/Size | Description               |
|--------------|-----------|---------------------------|
| Magic        | char[4]   | Always "FxSF"             |
| Custom Magic | char[4]   | User-definable magic code |
| Header size  | uint32    | Lenght of the header segment in bytes |

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

The main header and text segments are indidually ZSTD compressed each with global dictionaries which are stored in the decoder. The ZSTD frames must contain the decompressed sizes. To find the next segment, ZSTD frames by default contain the compressed size??

`Checksums` contains an array of all files in the order the files are defined in the main header. Each checksum is 4 bytes in CRC32 format. (TODO: specify CRC32)
It is optional, which is defined in the main header.

`User-data` is a space custom extensions can store anything, and extends from the end of the checksums to the `header leght`. If no custom extension is defined, this segment must not be present.

**Rationale**
The separation of the text strings and rest of the header is of two reasons. Filenames/paths are the only thing which have a variable lenght, by storing the text separately we can read the rest of the headers directly into the arrays with a few calls. Furthermore we can load the entire text segment into memory and easily create a text table to avoid creating lots of small allocations.
The second reason is that text has a significantly different properbility distribution compared to the rest of the header. By using a dictionary trained on (English) file paths, we should optimistically see an improvement over compressing both the text and binary data interleaved and with a shared dictionary.

Checksums are pretty much random, so I don't expect to any improvement by compressing it, instead I fear it would negatively affect the compression if stored in the Main header.

Main header
-----------
The segment is ZSTD dictionary compressed (to be defined). Both uncompressed and compressed size must be a part of the ZSTD header (confirm if this is also the case for compressed size).

The decompressed data contains the following:
- Decoding info
- File headers
- Folder headers
- Text lenghts

**Decoding info**

| Field       | Type   | Notes                               |
|-------------|--------|-------------------------------------|
|File count   | uint32 | The amount of files                 |
|Folder count | uint32 | Inconsistent type with folder ID!!! |
|FxSF version | uint16 | Major, minor ?                      |
|User version | uint16 | Major minor ?                       |

flags for CRC32 section


**File header**
Next follows `File count` file headers. The order is used to coorelate it with the text strings.

| Field           | Type   | Description |
|-----------------|--------|-------------|
| Offset | uint64 | The offset in the data section to where the file data is stored |
| File size       | uint64 | The uncompressed file size |
| Compressed size | uint64 | The actual size for storing it in the archive |
| Folder          | uint16 | ID of the folder this file is placed in |
| compression     | uint8  | Compression method used |
| flags           | uint8  | Misc stuff |
| user-data       | uint32 | Room for custom extensions |

The file header is slightly processed in order to compress better. The `File size` have been substracted with the `Compressed size`. The `Offset` have been substracted with the ending of the previous file, i.e. `offset - (prev.offset + prev.compressed_size)`.

**Folder header**

| Field  | Type   | Description             |
|--------|--------|-------------------------|
| Parent | uint16 | ID of the parent folder |

The root folder is identified by its parent folder being itself. To improve compression the most refered folder should have the ID `0`. Order specifies the folder ID.

TODO: Check how much this matter in reality. Might go for an order which is more efficient to process.

TODO: The type is smaller than the max amount of folders

**Text lenght**

| Field  | Type   | Description                           |
|--------|--------|---------------------------------------|
| Lenght | uint16 | Amount of bytes including ending "\0" |

TODO: Forced text encoding to UTF-8 for custom extensions?