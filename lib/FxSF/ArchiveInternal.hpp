/*	This file is part of FxSF, which is free software and is licensed
 * under the terms of the GNU GPL v3.0. (see http://www.gnu.org/licenses/ ) */

#ifndef ARCHIVE_INTERNAL_HPP
#define ARCHIVE_INTERNAL_HPP

#include <cctype>
#include <vector>
#include <memory>

namespace FxSF{


struct HeaderHeader{
	char     magic[4] = { 'F', 'x', 'S', 'F' };
	char     magic_custom[4] = { 0, 0, 0, 0 };
	uint32_t header_size;
	uint32_t main_header_size;
	
	bool checkMagic() const{
		return magic[0] == 'F'
			&& magic[1] == 'x'
			&& magic[2] == 'S'
			&& magic[3] == 'F'
			;
	}
	bool hasCustomMagic() const{
		return magic[0] || magic[1] || magic[2] || magic[3];
	}
};

struct HeaderStart{
	uint8_t  version_fxsf = 0;
	uint8_t  version_user = 0;
	uint16_t flags        = 0;
	uint32_t file_count;
	uint32_t folder_count;
	uint32_t text_size;
	
	bool noChecksums() const{ return flags & 0x1; }
};



}

#endif
