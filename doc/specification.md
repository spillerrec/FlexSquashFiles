The format contains 3 main segments:
- HeaderHeader, a very small pre-header
- Header, all meta-data required to access and decompress files
- Data, the compressed files referenced in the header

TODO: Everything here is little endian. Any reason for big-endian?
TODO: Is alignment important?
TODO: Bit 0 is the least significant bit, bit 7 is the most significant bit

HeaderHeader
--------

| Field            | Type/Size | Description                           |
| ---------------- | --------- | ------------------------------------- |
| Magic            | char[4]   | Always "FxSF"                         |
| Custom Magic     | char[4]   | User-definable magic code             |
| Header size      | uint32    | Length of the header segment in bytes |
| Main header size | uint32    | Length of first compressed segment    |

If `Custom magic` contains anything else than `"\0\0\0\0"` then the format is a custom extension.
Applications not recognizing this string are not allowed to modify the file without removing any custom features and should obviously warn the user in the progress. (Expert tools may allow modifications specified by the user.)

`Header size` is the total size of the header before data begins to appear. Decoders should always use if later versions use a bigger header. If `Header size` is `0`, then the archive is empty and it should not attempt to parse the header segment.

**TODO:** Reserve the two highest bytes of `Main header size` to select compression method? `0` would be ZSTD so we don't have to worry about it for now. Or we could use `0` for `Main header size` to indicate compression when it has a header (`Header size` is non-zero).


**Rationale**
Including an user-definable magic code makes it easy to identify a custom file format using this archive format as a base. ZIP based formats need to resort forcing the first file to be an uncompressed recognizable string, which even then is not guarantied to be at a fixed position. With FxSF, checking the first 8 bytes is all there is needed for a codec to tell if it can decode a file. This is also vastly less space required than ZIP which needs to specify a file header (including a filename).
It also ensures that conforming implementations know that special restrictions might apply.


Header
------
The header is ZSTD compressed with dictionary compression.

| Field        | Compression | Optional | Desccription                        |
| ------------ | ----------- | -------- | ----------------------------------- |
| Main header  | Yes         | No       | Contain file and folder information |
| Text         | Yes         | No       | Contains all text strings           |
| Checksums    | No          | Yes      | CRC32 for verification              |
| Dictionaries | N/A         | Yes      | Dictionaries for shared compression |
| User-data    | N/A         | Yes      | Area for extensions                 |

The main header and text segments are individually ZSTD compressed each with global dictionaries which are stored in the decoder. The ZSTD frames must contain the decompressed sizes. The compressed size of `Main header` (used for decompressing) is defined in the `HeaderHeader`, the compressed size of the `Text` segment is specified in the `Main header`.

`Checksums` contains an array of all files in the order the files are defined in the main header. Each checksum is 4 bytes in CRC32 format. (TODO: specify CRC32, it should be the same as ZIP as it seems to be defacto for archive formats and I see no advantages to using something else.)
It is optional, which is defined in the main header.

`User-data` is a space custom extensions can store anything, and extends from the end of the checksums to the `header leght`. If no custom extension is defined, this segment must not be present.

**TODO:** Dictionary encoding, we should have an way of storing those dictionaries in a way we can refer to them easily. We can't have them as files, as they would be extracted. Or perhaps files with special flags? Also I have seen that compressing the dictionaries can be very beneficial, so this should be supported.

**Rationale**
The separation of the text strings and rest of the header is of two reasons. Filenames/paths are the only thing which have a variable length, by storing the text separately we can read the rest of the headers directly into the arrays with a few calls. Furthermore we can load the entire text segment into memory and easily create a text table to avoid creating lots of small allocations.
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

| Field            | Type   | Notes                                 |
| ---------------- | ------ | ------------------------------------- |
| FxSF version     | uint8  | Currently 0 (unstable)                |
| User version     | uint8  | Custom extension version, 0 if not    |
| Main header size | uint8  | Size of header in multipes of 4       |
| Flags            | uint8  | Decoding flags, see below             |
| File count       | uint32 | The amount of files                   |
| Folder count     | uint32 | The amount of folders                 |
| Text size        | uint32 | The size of the text segment in bytes |

*Main header size:*

For forward compability, this is the size of this header times 4 bytes. This is currently always `4`, i.e. `16 bytes`.

*Decoding flags:*

| Bit  | Description                     |
| ---- | ------------------------------- |
| 0    | Checksums disabled (if `1`)     |
| 1    | Dictionaries available (if `1`) |
| 2-7  | Reserved, always `0`            |

**File header**
Next follows `File count` file headers. The order is used to coorelate it with the text strings.

| Field           | Type   | Description                              |
| --------------- | ------ | ---------------------------------------- |
| Offset          | uint64 | The offset in the data section to where the file data is stored |
| File size       | uint64 | The uncompressed file size               |
| Compressed size | uint64 | The actual size for storing it in the archive |
| Folder          | uint32 | ID of the folder this file is placed in  |
| compression     | uint8  | Compression method used                  |
| flags           | uint8  | Misc stuff                               |
| reserved        | uint16 | Reserved for future versions, must be 0  |
| user-data       | uint64 | Room for custom extensions               |

The `Offset` is the amount of bytes from the start of the data section to where the file data is stored, using `Compressed size` amount of bytes. Files may be placed anywhere in the data section, in any order. Areas in the data section not covered by any files must be zero. (**Rationale:** Force a consistent handling of undefined areas, as many file systems support zero block optimization.)

The file header is slightly processed in order to compress better. The `File size` have been substracted with the `Compressed size`. The `Offset` have been substracted with the ending of the previous file, i.e. `offset - (prev.offset + prev.compressed_size)`.
The application must support proper wrap-around in case `Offset` or `Filesize` becomes negative, though an implementation should always switch to `None` compression in case the selected method increases the filesize.

Compression defines the compression method based on the following ids:

| ID     | Method       | Notes                               |
| ------ | ------------ | ----------------------------------- |
| 0      | None         | File is stored uncompressed         |
| 1      | ZSTD         | File is compressed with Zstandard   |
| 2      | LZMA         |                                     |
| 3      | LZ4          |                                     |
| 4-15   | Reserved     | Do not use                          |
| 16-31  | User defined | Defined by a custom extension       |
| 32-255 | Dictionary   | Compressed using dictionary `ID-32` |

The first 16 `ID`s are required to be implemented by a decoder to be compliant with the specification. The compression routines may however be loaded dynamically as optional dependencies, provided a proper help text is provided. An encoder is only required to support `none` and  `ZSTD` however.

`ID`s `16` to `31` can be arbritary compression schemes defined by a custom extension, and thus is only allowed to be specified for custom extensions.

The remaining 224 IDs are used for compression with custom dictionaries.

| Bit  | Description                 |
| ---- | --------------------------- |
| 0    | Compressed with next if set |
| 1-7  | Reserved                    |

To compress several small files efficiently, multiple files may be stored in a single compressed stream. In this case, the first file in the stream has the flag set and the `Compressed size` is the size of the entire compressed stream. The later files with the flag set have a `Compressed size` of zero with the same `Offset` as the first file, and the last file included in that compressed stream will have the flag unset.
**TODO**: Should we even support this? It complicates decoding quite a bit and the whole point of this format is to allow quick access to arbitrary files, using big compressed chunks defeats this. Perhaps we can get decent results by creating dictionaries?


`reserved` must be zero.

`user-data` must be zero unless this is a custom extension.

**Folder header**

| Field  | Type   | Description             |
| ------ | ------ | ----------------------- |
| Parent | uint32 | ID of the parent folder |

The root folder is identified by its parent folder being itself. To improve compression the most referred folder should have the ID `0`. Order specifies the folder ID.

**TODO:** Check how much this matter in reality. Might go for an order which is more efficient to process.

**Text length**

| Field  | Type   | Description                           |
| ------ | ------ | ------------------------------------- |
| Length | uint16 | Amount of bytes including ending "\0" |

**TODO:** Forced text encoding to UTF-8 even for custom extensions?

Text
----

The compressed size of the text segment is stored in the `HeaderHeader`. The uncompressed size is the sum of the text length, and the position of each text string is right after the previous one in the order defined in the text length.

Checksums
---------

If `Checksums disabled` is false, this segment contains `Files count` CRC32 checksums (uint32) without any compression applied.

The option of disabling checksums is mainly there for custom extensions or use-cases which have some other way of verifying the integrity of the archive. Consider for example a package system which saves a checksum for each package in its repository.

## Dictionaries

| **Field**    | **Type**                   | **Descriptionn**                                    |
| ------------ | -------------------------- | --------------------------------------------------- |
| dict_count   | uint32_t                   | Amount of dictionaries                              |
| headers      | array of Dictionary header | `dict_count` dictionary headers                     |
| dictionaries | raw data                   | `dict_count` data blocks containing each dictionary |

**Dictionary header**

| **Field**          | **Type** | **Description**                   |
| ------------------ | -------- | --------------------------------- |
| Data Size          | uint32_t | How large the raw data is         |
| Compression method | uint8_t  | What compression scheme is in use |
| Reserved           | uint8_t  | Reserved                          |
| User data          | uint16_t | Compresion method specific data   |

`Compression method` is defined the same as the file header. It is obviously not allowed to be in the dictionary range.

Currently no flags are used and this must be `0`.

`User data` is specific to a compression scheme and can hold additional setting if needed. If not used, this shall be `0`. In case 2 bytes is not enough, those settings should be embedded in the data section.

**Dictionary data**

**TODO:** Again, should the data buffers be aligned?

Each block must be next to each other.

## Dictionary types

Currently dictionaries are only defined for Zstandard

**ZSTD**

The data section contains the ZSTD dictionary as defined by ZSTD, but is additionally compressed with ZSTD. `User Data` is currently not used.

User-data
---------
This segment is the size of `Header size` subtracted with the sizes of all other segments, and may only exists in custom extensions. The contents of the segment is entirely up to the custom extension.

**Rationale:**
In case the 64 bits in the `File header` isn't enough, this segment can be used to provide extended meta-data to each file, shared dictionaries, or other info useful before starting to decode the files. Custom extensions could use an ordinary file with a specific name or position instead though.

## Archive creation overview

###Normal compression

This does not support streaming as you can't complete the header without knowing all the offsets, but has the smallest space complexity.

- Collect all files
- Create a header with all files without the compression size
- Determine the size of the header
- Start compressing files at the offset of the uncompressed header
- Update header with the compressed size
- Update header-header with the compressed header sizes / offsets
- Write header-header and the header
- Move data section to the smaller offset
- Resize the file to the new size

###Cached compression

If the files can be compressed ahead of time, streaming is possible. This approach is also likely what you want to use if you want to edit existing files.

- Collect all the files
- Compress all the files, or obtain an already compressed version (for example in another FxSF file)
- Create the complete header
- Write header-header and header
- Write each compressed file, making sure it matches the sizes specified

