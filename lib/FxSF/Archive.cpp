#include "Archive.hpp"
#include "zstd++.hpp"

#include <cstring>
#include <iostream>
#include <zstd.h>

using namespace FxSF;


struct HeaderHeader{
	char magic[4];
	char magic_custom[4];
	uint32_t header_size;
};

struct HeaderStart{
	uint32_t file_count;
	uint32_t folder_count;
};

Archive::Archive( Reader& reader ){
	//Read headerheader
	HeaderHeader headerheader;
	reader.read( &headerheader, sizeof(headerheader) );
	
	//Read header
	auto buf = std::make_unique<char[]>( headerheader.header_size );
	reader.read( buf.get(), headerheader.header_size );
	
	//Decompress main header
	auto main_head_size = zstd::getUncompressedSize( buf.get(), headerheader.header_size );
	auto main_header = std::make_unique<char[]>( main_head_size );
	auto decompress_result = ZSTD_decompress( main_header.get(), main_head_size, buf.get(), headerheader.header_size );
	if( ZSTD_isError( decompress_result ) )
		std::cout << "Decompression failure" << std::endl;
	
	HeaderStart start;
	std::memcpy( &start, main_header.get(), sizeof(start) );
	std::cout << "Files: " << start.file_count << std::endl;
	files.resize( start.file_count );
	std::cout << "Total size: " << sizeof(File)*start.file_count+sizeof(start) << std::endl;
	std::cout << "Expected: " << main_head_size << std::endl;
	std::memcpy( files.data(), main_header.get()+sizeof(start), sizeof(File)*start.file_count );
	
	if( files.size() > 0 )
		files[0].decompress();
	for( size_t i=1; i<files.size(); i++ )
		files[i].decompress( files[i-1] );
	
	for( auto file : files )
		std::cout << "normal : " << file.file_start() << " " << file.filesize << " " << file.compressed_size << std::endl;
}

void Archive::write( Writer& writer ){
	//Construct headerheader
	HeaderHeader headerheader;
	headerheader.magic[0] = 'F';
	headerheader.magic[1] = 'x';
	headerheader.magic[2] = 'S';
	headerheader.magic[3] = 'F';
	headerheader.magic_custom[0] = 0;
	headerheader.magic_custom[1] = 0;
	headerheader.magic_custom[2] = 0;
	headerheader.magic_custom[3] = 0;
	
	//Construct header
	auto size = 8 + files.size() * sizeof(File);
	auto buf = std::make_unique<char[]>( size );
	HeaderStart start;
	start.file_count = files.size();
	std::cout << "Files: " << start.file_count << std::endl;
	start.folder_count = folders.size();
	std::memcpy( buf.get(), &start, sizeof(HeaderStart) );
	
	for( unsigned i=files.size()-1; i>0; i-- ){
		std::cout << "normal : " << files[i].file_start() << " " << files[i].filesize << " " << files[i].compressed_size << std::endl;
		files[i].compress( files[i-1] );
	}
	if( files.size() > 0 )
		files[0].compress();
	std::memcpy( buf.get() + sizeof(HeaderStart), files.data(), sizeof(File)*files.size() );
	
	//Compress header
	auto compressed = zstd::compress( buf.get(), size );
	headerheader.header_size = compressed.second;
	
	//Write result
	writer.write( &headerheader, sizeof(headerheader) );
	writer.write( compressed.first.get(), compressed.second );
//	writer.write( buf.get(), size );
}
