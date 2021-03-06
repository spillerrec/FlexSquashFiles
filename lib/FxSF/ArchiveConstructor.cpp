#include "ArchiveConstructor.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <numeric>

using namespace FxSF;


const FoldersConstructor::ArchiveFolder& FoldersConstructor::get( unsigned id ) const{
	if( id > folders.size() )
		throw std::runtime_error( "Outt of bounds" );
	return folders[id];
}

/**
@param parent_id The ID of the folder this exists in
@param create If folder does not exist, create it
@return ID of this folder
*/
unsigned FoldersConstructor::getFolder( std::string folder_name, unsigned parent_id, bool create ){
	for( unsigned i=0; i<folders.size(); i++ ){
		auto& folder = folders[i];
		if( folder.parent == parent_id && folder.folder == folder_name )
			return i;
	}
	
	//No matching folder found, create or fail
	if( !create )
		throw std::runtime_error( "Folder does not exist" );
	
	folders.emplace_back( parent_id, folder_name );
	return folders.size()-1;
}

unsigned FoldersConstructor::addFolderPath( std::string path ){
	//Split into last name and parent
	auto it = std::find( path.rbegin(), path.rend(), '/' );
	auto pos = path.size() - (it - path.rbegin());
	
	auto parent = path.substr( 0, pos > 0 ? pos - 1 : 0 );
	auto name = path.substr(pos);
	
	//Trivial case
	if( name.size() <= 0 )
		return getRoot();
	
	//Recurse
	return getFolder( name, addFolderPath( parent ) );
}

std::vector<uint32_t> FoldersConstructor::folderMap() const{
	std::vector<uint32_t> map;
	map.reserve( folders.size() + 1 );
	
	for( auto folder : folders )
		map.push_back( folder.parent );
	
	return map;
}

ArchiveFileInfo::ArchiveFileInfo( std::string name, std::string dir, uint64_t size )
	:	name(name), dir(dir), size(size), compressed_size(size)
	{ }
		
void ArchiveFileInfo::setCompression( Compressor c, uint64_t compressed_size ){
	compressor = c;
	this->compressed_size = compressed_size;
}

bool ArchiveFileInfo::isValid() const{
	return name.size() > 0 && compressed_size <= size;
}


void ArchiveConstructor::addFile( ArchiveFileInfo file_info ){
	if( !file_info.isValid() ){
		std::cout << "Something wrong with ArchiveFileInfo\n";
		std::exit( -1 );
	}
	
	//TODO: Check checksum setting
	if( !file_info.checksum_set ){
		std::cout << "Missing checksum for file\n";
		std::exit( -1 );
	}
	
	ArchiveFile f;
	f.name = file_info.name;
	f.folder_id = folders.addFolderPath( file_info.dir );
	f.size = file_info.size;
	f.compressed_size = file_info.compressed_size;
	f.checksum = file_info.checksum;
	f.compressor = file_info.compressor;
	files.push_back( f );
}

uint64_t ArchiveConstructor::text_amount() const{
	return
			std::accumulate(
				files.begin(), files.end(), uint64_t(0)
				,	[](uint64_t acc, auto& file){ return acc + file.name.size() + 1; }
			)
		+
			std::accumulate(
				folders.begin(), folders.end(), uint64_t(0)
				,	[](uint64_t acc, auto& folder){ return acc + folder.folder.size() + 1; }
			)
		;
}

Archive ArchiveConstructor::createHeader(){
	//TODO: create folder map
	//TODO: sort by how often they are used
	Archive arc;
	arc.folders = folders.folderMap();
	
	uint64_t current_offset = 0;
	for( auto& file : files ){
		File f;
		f.offset = current_offset;
		f.filesize = file.size;
		f.compressed_size = file.compressed_size;
		f.folder = file.folder_id;
		f.format = uint8_t(file.compressor);
		f.flags  = 0; //TODO:
		f.user   = 0; //TODO:
		arc.files.push_back( f );
		arc.checksums.push_back( file.checksum );
		
		current_offset += f.compressed_size;
	}
	
	//Create text table
	arc.text_buffer = std::make_unique<char[]>( text_amount() );
	uint64_t offset = 0;
	auto copy_name = [&](const std::string& str){
			String s;
			s.start = arc.text_buffer.get() + offset;
			s.length = str.size() + 1;
			
			std::memcpy( s.start, str.c_str(), str.size() );
			offset += s.length;
			arc.text_buffer[offset-1] = 0;
			return s;
		};
	arc.strings.reserve( files.size() + arc.folders.size() );
	for( auto& file : files )
		arc.strings.push_back( copy_name( file.name ) );
	
	for( auto& folder : folders )
		arc.strings.push_back( copy_name( folder.folder ) );
	
	return arc;
}
