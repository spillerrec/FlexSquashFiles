#include "ArchiveConstructor.hpp"

#include <cstring>
#include <iostream>
#include <numeric>

using namespace FxSF;

unsigned ArchiveConstructor::getFolderPos( std::string dir, bool create ){
	return 0; //TODO:
}

void ArchiveConstructor::addFile( std::string name, std::string dir, unsigned compressed_size, unsigned size ){
	ArchiveFile f;
	f.name = name;
	f.folder_id = getFolderPos( dir );
	f.size = size;
	f.compressed_size = compressed_size;
	files.push_back( f );
}

uint64_t ArchiveConstructor::text_amount() const{
	return
			std::accumulate(
				files.begin(), files.end(), uint64_t(0)
				,	[](uint64_t acc, auto& file){ return acc + file.name.size() + 1; }
			)
		+
			0 //TODO: folder names
		;
}

Archive ArchiveConstructor::createHeader(){
	//TODO: create folder map
	//TODO: sort by how often they are used
	Archive arc;
	arc.folders.push_back( 0 );
	
	uint64_t current_offset = 0;
	for( auto& file : files ){
		File f;
		f.offset = current_offset;
		f.filesize = file.size;
		f.compressed_size = file.compressed_size;
		f.folder = 0; //TODO:
		f.format = 1; //TODO:
		f.flags  = 0; //TODO:
		f.user   = 0; //TODO:
		arc.files.push_back( f );
		
		current_offset += f.compressed_size;
	}
	
	//Create text table
	auto size = text_amount();
	arc.strings.resize( files.size() );
	arc.text_buffer = std::make_unique<char[]>( text_amount() );
	uint64_t offset = 0;
	for( size_t i=0; i<files.size(); i++ ){
		auto& file = files[i];
		auto& string = arc.strings[i];
		
		string.start = arc.text_buffer.get() + offset;
		string.length = file.name.size() + 1;
		
		std::memcpy( string.start, file.name.c_str(), file.name.size() );
		offset += string.length;
		arc.text_buffer[offset-1] = 0;
	}
	
	return arc;
}
