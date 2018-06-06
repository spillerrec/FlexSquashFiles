/*	This file is part of FxSF, which is free software and is licensed
 * under the terms of the GNU GPL v3.0. (see http://www.gnu.org/licenses/ ) */

#ifndef ARCHIVE_CONSTRUCTOR_HPP
#define ARCHIVE_CONSTRUCTOR_HPP

#include "Archive.hpp"

#include <string>
#include <vector>

namespace FxSF{
	
class FoldersConstructor{
	private:
		struct ArchiveFolder{
			unsigned parent = { 0 };
			std::string folder;
			ArchiveFolder(){}
			ArchiveFolder( unsigned parent, std::string folder)
				: parent(parent), folder(folder) {}
		};
		std::vector<ArchiveFolder> folders;
		const ArchiveFolder& get( unsigned id ) const;
		unsigned getRoot() const{ return 0; }
		
	public:
		FoldersConstructor(){ folders.push_back({}); }
		unsigned getFolder( std::string folder, unsigned parent, bool create=true );
		unsigned addFolderPath( std::string path );
		const std::string& operator[](unsigned index) const
			{ return get(index).folder; }
			
		std::vector<uint32_t> folderMap() const;
		
		auto begin() const{ return folders.begin(); }
		auto end()   const{ return folders.end();   }
};

class ArchiveConstructor{
	private:
		struct ArchiveFile{
			std::string name;
			unsigned folder_id;
			uint64_t compressed_size;
			uint64_t size;
			uint32_t checksum;
		};
		std::vector<ArchiveFile> files;
		
		FoldersConstructor folders;
		
		uint64_t text_amount() const;
	
	public:
		ArchiveConstructor(){ }
		void addFile( std::string name, std::string dir, unsigned compressed_size, unsigned size );
		
		Archive createHeader();
};

	
}

#endif
