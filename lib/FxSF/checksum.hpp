/*	This file is part of FxSF, which is free software and is licensed
 * under the terms of the GNU GPL v3.0. (see http://www.gnu.org/licenses/ ) */

#ifndef CHECKSUM_HPP
#define CHECKSUM_HPP

#include <memory>
#include <utility>
#include <stdint.h>

namespace checksum{
	uint32_t crc32( const void* in_data, uint64_t in_size );
};

#endif
