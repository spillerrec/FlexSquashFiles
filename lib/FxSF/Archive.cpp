#include "Archive.hpp"
#include "zstd++.hpp"

#include <cstring>
#include <iostream>
#include <numeric>
#include <zstd.h>

using namespace FxSF;

class BufferIO{
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


struct HeaderHeader{
	char     magic[4] = { 'F', 'x', 'S', 'F' };
	char     magic_custom[4] = { 0, 0, 0, 0 };
	uint32_t header_size;
	uint32_t main_header_size;
};

struct HeaderStart{
	uint8_t  version_fxsf = 0;
	uint8_t  version_user = 0;
	uint16_t flags        = 0;
	uint32_t file_count;
	uint32_t folder_count;
	uint32_t text_size;
};

Archive::Archive( Reader& reader ){
	//Read HeaderHeader
	HeaderHeader headerheader;
	reader.read( &headerheader, sizeof(headerheader) );
	
	//Read header
	auto buf = std::make_unique<char[]>( headerheader.header_size );
	reader.read( buf.get(), headerheader.header_size );
	
	//Decompress main header
	auto main_head_size = zstd::getUncompressedSize( buf.get(), headerheader.header_size );
	auto main_header = std::make_unique<char[]>( main_head_size );
	auto decompress_result = ZSTD_decompress( main_header.get(), main_head_size, buf.get(), headerheader.header_size );
	if( ZSTD_isError( decompress_result ) )
		std::cout << "Decompression failure" << std::endl;
	
	//Read HeaderStart
	HeaderStart start;
	
	BufferIO main_header_reader( main_header.get() );
	main_header_reader.copyTo( start );
	
	//Read files
	files.resize( start.file_count );
	main_header_reader.copyTo( files );
	
	//Defilter files
	if( files.size() > 0 )
		files[0].decompress();
	for( size_t i=1; i<files.size(); i++ )
		files[i].decompress( files[i-1] );
	
	//Read folders
	
	std::cout << "Files: " << start.file_count << std::endl;
	for( auto file : files )
		std::cout << "normal : " << file.file_start() << " " << file.filesize << " " << file.compressed_size << std::endl;
}

void Archive::write( Writer& writer ){
	//Construct HeaderStart
	HeaderStart start;
	start.file_count   = files.size();
	start.folder_count = folders.size();
	
	//Filter files
	for( unsigned i=files.size()-1; i>0; i-- )
		files[i].compress( files[i-1] );
	if( files.size() > 0 )
		files[0].compress();
	
	//Create header
	auto size = 8 + files.size() * sizeof(File);
	auto buf = std::make_unique<char[]>( size );
	BufferIO buf_writer( buf.get() );
	buf_writer.writeFrom( start );
	buf_writer.writeFrom( files );
	
	//Compress header
	auto compressed = zstd::compress( buf.get(), size );
	
	//TODO: Compress text
	auto text_amount = std::accumulate( strings.begin(), strings.end(), uint64_t(0), [](uint64_t acc, String& s){ return acc + s.length; } );
	start.text_size = 0;
	
	//Construct HeaderHeader
	HeaderHeader headerheader;
	headerheader.main_header_size = compressed.second;
	headerheader.header_size      = compressed.second;
	
	//Write result
	writer.write( &headerheader, sizeof(headerheader) );
	writer.write( compressed.first.get(), compressed.second );
}
