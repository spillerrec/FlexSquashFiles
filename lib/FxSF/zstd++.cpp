#include "zstd++.hpp"

#include <zstd.h>

uint64_t zstd::maxSize( uint64_t uncompressed_size )
	{ return ZSTD_compressBound( uncompressed_size ); }

std::pair<std::unique_ptr<char[]>, uint64_t> zstd::compress( const void* in_data, uint64_t in_size ){
	//Prepare
	auto buf_size = zstd::maxSize( in_size );
	auto buf = std::make_unique<char[]>( buf_size );
	
	//Compress
	auto result = ZSTD_compress( buf.get(), buf_size, in_data, in_size, ZSTD_maxCLevel() );
	if( ZSTD_isError(result) )
		return { std::make_unique<char[]>(0), 0 };
	
	return { std::move( buf ), result };
}

uint64_t zstd::getUncompressedSize( const void* in_data, uint64_t in_size ){
	return ZSTD_getDecompressedSize( in_data, in_size );
}

std::pair<std::unique_ptr<char[]>, uint64_t> zstd::decompress( const void* in_data, uint64_t in_size ){
	//Get output-size and prepare a buffer for it
	auto u_size = zstd::getUncompressedSize( in_data, in_size );
	auto u_buf = std::make_unique<char[]>( u_size );
	
	//Decompress
	if( ZSTD_isError( ZSTD_decompress( u_buf.get(), u_size, in_data, in_size ) ) )
		return { std::make_unique<char[]>(0), 0 };
	return { std::move( u_buf ), u_size };
	
}
