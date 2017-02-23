/*	This file is part of FxSF, which is free software and is licensed
 * under the terms of the GNU GPL v3.0. (see http://www.gnu.org/licenses/ ) */

#ifndef ARCHIVE_CONSTRUCTOR_HPP
#define ARCHIVE_CONSTRUCTOR_HPP

#include "Archive.hpp"

#include <string>
#include <vector>

namespace FxSF{
	

class ArchiveConstructor{
	private:
		struct ArchiveFile{
			std::string name;
			unsigned folder_id;
			uint64_t compressed_size;
			uint64_t size;
		};
		std::vector<ArchiveFile> files;
		
		struct ArchiveFolder{
			std::string folder;
		};
		std::vector<ArchiveFolder> folders;
		
		unsigned getFolderPos( std::string dir, bool create=true );
	
	public:
		ArchiveConstructor(){
			folders.push_back( {} );
		}
		void addFile( std::string name, std::string dir, unsigned compressed_size, unsigned size );
		
		Archive createHeader();
};

	
}

#endif
