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
	uint32_t folder;
	uint8_t  format;
	uint8_t  flags    = { 0 };
	uint16_t reserved = { 0 };
	uint64_t user     = { 0 };
	
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
	String() {}
	String( char* start, unsigned length )
		: start(start), length(length) {}
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
		std::vector<uint32_t> folders;
		std::vector<uint32_t> checksums;
		
		std::vector<String> strings;
		std::unique_ptr<char[]> text_buffer;
		unsigned buffer_size{ 0 };
		
		uint64_t textRealSize() const;
		auto checksumFileSize() const
			{ return checksums.size() * sizeof(uint32_t); };
		
		class Iterator{
			private:
				Archive* arc;
				size_t id;
				
				auto& file()       { return arc->files[id]; }
				auto& file() const { return arc->files[id]; }
				
			public:
				Iterator( Archive& arc, size_t id )
					: arc(&arc), id(id) {}
					
				auto fileStart() const{ return file().file_start(); }
				auto fileEnd()   const{ return file().file_end(); }
				auto fileSize()  const{ return file().filesize; }
				
				auto name() const{ return arc->strings[id]; }
				auto folder() const{ return file().folder; }
				
				bool operator!=(const Iterator& other) const
					{ return arc != other.arc || id != other.id; }
				
				Iterator& operator++(){
					id++;
					return *this;
				}
				auto& operator*()       { return *this; }
				auto& operator*() const { return *this; }
		};
		
		Archive() {}
		
	public:
		Archive( Reader& reader );
		
		auto fileCount() const{ return files.size(); }
		
		void write( Writer& writer );
		
		Iterator begin(){ return {*this, 0}; }
		Iterator end(){ return {*this, files.size()}; }
		
		auto folderName( unsigned id ) const
			{ return strings[fileCount() + id]; }
		auto folderParent( unsigned id ) const
			{ return folders[id]; }
};

	
	
}

#endif
