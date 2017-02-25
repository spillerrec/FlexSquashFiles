/*	This file is part of FxSF, which is free software and is licensed
 * under the terms of the GNU GPL v3.0. (see http://www.gnu.org/licenses/ ) */

#ifndef ARCHIVE_HPP
#define ARCHIVE_HPP

#include <cctype>
#include <vector>
#include <memory>

namespace FxSF{
	

constexpr uint8_t CUSTOM_CODEC_OFFSET = 128;
constexpr uint8_t STREAM_PREV_CODEC = 127;

struct File{
	uint64_t offset;
	uint64_t filesize;
	uint64_t compressed_size;
	uint16_t folder;
	uint8_t  format;
	uint8_t  flags;
	uint32_t user;
	
	uint64_t file_start() const{ return offset; }
	uint64_t file_end()   const{ return file_start() + compressed_size; }
	
	void compress(){ filesize -= compressed_size; }
	void compress( File previous ){
		compress();
		offset -= previous.file_end();
	}
	void decompress(){ filesize += compressed_size; }
	void decompress( File previous ){
		decompress();
		offset += previous.file_end();
	}
	
	bool streamCompression() const{ return format == STREAM_PREV_CODEC; }
	bool customCodec()       const{ return format >= CUSTOM_CODEC_OFFSET; }
	bool getFormat()         const{ return format; }
	bool getCustomFormat()   const{ return format - CUSTOM_CODEC_OFFSET; }
	
	
};

struct String{
	char* start;
	unsigned length;
};

class Reader{
	public:
		virtual bool read( void* buffer, uint64_t amount ) = 0;
		virtual bool seek( uint64_t position ) = 0;
};

class Writer{
	public:
		virtual bool write( const void* buffer, uint64_t amount ) = 0;
		virtual bool seek( uint64_t position ) = 0;
};

class ArchiveConstructor;
class Archive{
	friend class ArchiveConstructor;
	private:
		std::vector<File> files;
		std::vector<uint16_t> folders;
		std::vector<String> strings;
		std::unique_ptr<char[]> text_buffer;
		
		class Iterator{
			private:
				Archive* arc;
				unsigned id;
				
			public:
				Iterator( Archive& arc, unsigned id )
					: arc(&arc), id(id) {}
					
				auto fileStart() const{ return arc->files[id].file_start(); }
				auto fileEnd()   const{ return arc->files[id].file_end(); }
				auto fileSize()  const{ return arc->files[id].filesize; }
		};
		
		Archive() {}
		
	public:
		Archive( Reader& reader );
		
		auto fileCount() const{ return files.size(); }
		
		void write( Writer& writer );
};

	
	
}

#endif
