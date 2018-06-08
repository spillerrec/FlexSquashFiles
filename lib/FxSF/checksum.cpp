/*	This file is part of FxSF, which is free software and is licensed
 * under the terms of the GNU GPL v3.0. (see http://www.gnu.org/licenses/ ) */

#include "checksum.hpp"

#include <boost/crc.hpp>

uint32_t checksum::crc32( const void* in_data, uint64_t in_size ){
	boost::crc_32_type crc;
	crc.process_bytes( in_data, in_size );
	return crc.checksum();
}
