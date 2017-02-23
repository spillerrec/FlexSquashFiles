#include "zstd++.hpp"

#include <zstd.h>

uint64_t zstd::maxSize( uint64_t uncompressed_size )
	{ return ZSTD_compressBound( uncompressed_size ); }

std::pair<std::unique_ptr<char[]>, uint64_t> zstd::compress( const void* in_data, uint64_t in_size ){
	//Prepare
	auto buf_size = zstd::maxSize( in_size );
	auto buf = std::make_unique<char[]>( buf_size );
	
	//Compress
	auto result = ZSTD_compress( buf.get(), buf_size, in_data, in_size, 11 );
	if( ZSTD_isError(result) )
		return { std::make_unique<char[]>(0), 0 };
	
	return { std::move( buf ), result };
}
