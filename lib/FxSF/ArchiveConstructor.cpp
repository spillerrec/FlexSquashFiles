#include "ArchiveConstructor.hpp"

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
	
	return arc;
}
