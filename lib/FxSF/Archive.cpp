#include "Archive.hpp"
#include "ArchiveInternal.hpp"
#include "zstd++.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <numeric>
#include <zstd.h>

using namespace FxSF;

class BufferIO{
	//TODO: Output buffer size !!!
	private:
		char* data;
		uint64_t pos{ 0 };
	
	public:
		BufferIO( char* data ) : data( data ) { }
		
		template<typename T>
		void copyTo( std::vector<T>& vec ){
			auto size = sizeof(T)*vec.size();
			std::memcpy( vec.data(), data+pos, size );
			pos += size;
		}
		
		template<typename T>
		void copyTo( std::vector<T>& vec, unsigned amount ){
			vec.resize( amount );
			copyTo( vec );
		}
		
		template<typename T>
		void copyTo( T& t ){
			auto size = sizeof(T);
			std::memcpy( &t, data+pos, size );
			pos += size;
		}
		
		template<typename T>
		void writeFrom( std::vector<T>& vec ){
			auto size = sizeof(T)*vec.size();
			std::memcpy( data+pos, vec.data(), size );
			pos += size;
		}
		
		template<typename T>
		void writeFrom( T& t ){
			auto size = sizeof(T);
			std::memcpy( data+pos, &t, size );
			pos += size;
		}
};


Archive::Archive( Reader& reader ){
	auto error = [&]( /*int code,*/ const char* message )
		{ std::cout << message << '\n'; };
		//TODO: Report this error somehow
		
	auto read = [&]( uint64_t amount ){
			auto buf = std::make_unique<char[]>( amount );
			reader.read( buf.get(), amount );
			return std::move( buf );
		};
	
	//Read HeaderHeader
	HeaderHeader headerheader;
	reader.read( &headerheader, sizeof(headerheader) );
	data_offset = headerheader.header_size + sizeof(HeaderHeader);
	
	//Header sanity checks
	if( !headerheader.checkMagic() ){
		error( "Not a FxSF file" );
		return;
	}
	if( std::max(headerheader.header_size, headerheader.main_header_size) > 4000000000 ){
		error( "Header size is over 4G, arborting" );
		return; //TODO: change this?
	}
	
	//Read and decompress main header
	auto buf = read( headerheader.main_header_size );
	auto main_header = zstd::decompress( buf.get(), headerheader.main_header_size );
	if( !main_header.first ){
		error( "Failed to decompress header" );
		return;
	}
	
	//Read HeaderStart
	HeaderStart start;
	
	BufferIO main_header_reader( main_header.first.get() );
	main_header_reader.copyTo( start );
	
	//Read files
	main_header_reader.copyTo( files, start.file_count );
	
	//Defilter files
	if( files.size() > 0 )
		files[0].decompress();
	for( size_t i=1; i<files.size(); i++ )
		files[i].decompress( files[i-1] );
	
	//Read folders
	main_header_reader.copyTo( folders, start.folder_count );
	
	//Read text lenghts
	std::vector<uint16_t> text_lengths;
	main_header_reader.copyTo( text_lengths, start.file_count + start.folder_count );
	
	
	//Read checksums
	if( false/* !start.noChecksums()*/ ){
		checksums.resize( start.file_count );
		reader.read( checksums.data(), sizeof(uint32_t)*checksums.size() );
	}
	
	//Read Text
	auto text_buf = read( start.text_size );
	auto text_decompress = zstd::decompress( text_buf.get(), start.text_size );
	text_buffer = std::move(text_decompress.first);
	//TODO: check that output size matches text size
	if( !text_buffer ){
		std::cout << "Text decompression failure" << std::endl;
		return;
	}
	
	//Decode into strings with text lenghts
	strings.reserve( start.file_count );
	auto text_offset = text_buffer.get();
	for( unsigned i=0; i<start.file_count + start.folder_count; i++ ){
		strings.emplace_back( text_offset, text_lengths[i] );
		text_offset += text_lengths[i];
	}
	//TODO: check for bounds
	//TODO: Store final size
	if( text_decompress.second != textRealSize() ){
		error( "Decompressed text size does not match" );
		return;
	}
	
	//NOTE: Debug
	std::cout << "Files: " << start.file_count << std::endl;
	for( auto file : files )
		std::cout << "normal : " << file.file_start() << " " << file.filesize << " " << file.compressed_size << std::endl;
}

uint64_t Archive::textRealSize() const{
	return std::accumulate(
			strings.begin(), strings.end(), uint64_t(0)
		,	[](uint64_t acc, const String& s){ return acc + s.length; }
		);
}

template<typename T>
size_t vectorDataSize( const std::vector<T>& vec )
	{ return sizeof(T) * vec.size(); }

void Archive::write( Writer& writer ){
	//Compress text
	//NOTE: We need the compressed size in the header
	auto compressed_text = zstd::compress( text_buffer.get(), textRealSize() );
	
	//Construct HeaderStart
	HeaderStart start;
	start.file_count   = files.size();
	start.folder_count = folders.size();
	start.text_size = compressed_text.second;
	
	//Filter files
	for( unsigned i=files.size()-1; i>0; i-- )
		files[i].compress( files[i-1] );
	if( files.size() > 0 )
		files[0].compress();
	
	//Create text lengths
	std::vector<uint16_t> text_lengths;
	text_lengths.reserve( strings.size() );
	for( auto& string : strings )
		text_lengths.push_back( string.length );
	
	
	//Create header
	auto size = sizeof(start) + vectorDataSize( files ) + vectorDataSize( folders ) + vectorDataSize( text_lengths );
	auto buf = std::make_unique<char[]>( size );
	BufferIO buf_writer( buf.get() );
	buf_writer.writeFrom( start );
	buf_writer.writeFrom( files );
	buf_writer.writeFrom( folders );
	buf_writer.writeFrom( text_lengths );
	
	//Compress header
	auto compressed = zstd::compress( buf.get(), size );
	
	
	//Construct HeaderHeader
	HeaderHeader headerheader;
	headerheader.main_header_size = compressed.second;
	uint32_t full_size =
			compressed.second
		+	compressed_text.second
		+	checksumFileSize()
		+	0 //TODO: User-data
		;
	headerheader.header_size = full_size;
	data_offset = headerheader.header_size + sizeof(HeaderHeader);
	
	//Write result
	writer.write( &headerheader, sizeof(headerheader) );
	writer.write( compressed.first.get(), compressed.second );
	writer.write( checksums.data(), checksumFileSize() );
	writer.write( compressed_text.first.get(), compressed_text.second );
}
