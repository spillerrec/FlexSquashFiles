/*	This file is part of FxSF, which is free software and is licensed
 * under the terms of the GNU GPL v3.0. (see http://www.gnu.org/licenses/ ) */

#ifndef ZSTD_PP_HPP
#define ZSTD_PP_HPP

#include <memory>
#include <utility>

namespace zstd{
	uint64_t maxSize( uint64_t uncompressed_size );
	std::pair<std::unique_ptr<char[]>, uint64_t> compress( const void* in_data, uint64_t in_size );
	
	uint64_t getUncompressedSize( const void* in_data, uint64_t in_size );
	/*
	ByteView compress( const char* data, char* out );
	Buffer   compress( const char* data            );
	bool   decompress( const char* data, char* out );
	*/
};

#endif
