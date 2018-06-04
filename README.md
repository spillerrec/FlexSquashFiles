FlexSquashFiles
===============

A non-streaming archive format intended for custom file formats. The format is optimized for being accessed often, but modified rarely.

Features
--------
- Commplete header at the start of the file, so you can seek to specific files
- Support for custom magic codes within the first 8 bytes of the file
- Header compression to reduce overhead
- Header is small-buffer optimized
- User-reserved areas in the header

Status
------
Bare-bones implementation and simplistic CLI interface