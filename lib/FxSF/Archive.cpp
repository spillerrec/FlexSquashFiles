#include "Archive.hpp"
#include "zstd++.hpp"

#include <cstring>
#include <iostream>

using namespace FxSF;


struct HeaderHeader{
	char magic[4];
	char magic_custom[4];
	uint32_t header_size;
};

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
	struct HeaderStart{
		uint32_t file_count;
		uint32_t folder_count;
	} start;
	start.file_count = files.size();
	start.folder_count = folders.size();
	std::memcpy( buf.get(), &start, sizeof(HeaderStart) );
	
	for( unsigned i=files.size()-1; i>0; i-- ){
		std::cout << "normal : " << files[i].file_start() << " " << files[i].filesize << " " << files[i].compressed_size << std::endl;
		files[i].compress( files[i-1] );
		std::cout << "changed: " << files[i].file_start() << " " << files[i].filesize << " " << files[i].compressed_size << std::endl;
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
