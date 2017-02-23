#include <QCoreApplication>
#include <QDir>
#include <QFile>

#include <iostream>
#include <memory>
#include <cstring>

#include <zstd.h>

namespace zstd{
	uint64_t maxSize( uint64_t uncompressed_size );
	std::pair<std::unique_ptr<char[]>, uint64_t> compress( const void* in_data, uint64_t in_size );
	/*
	ByteView compress( const char* data, char* out );
	Buffer   compress( const char* data            );
	bool   decompress( const char* data, char* out );
	*/
};

uint64_t zstd::maxSize( uint64_t uncompressed_size )
	{ return ZSTD_compressBound( uncompressed_size ); }

std::pair<std::unique_ptr<char[]>, uint64_t> zstd::compress( const void* in_data, uint64_t in_size ){
	//Prepare
	auto buf_size = zstd::maxSize( in_size );
	auto buf = std::make_unique<char[]>( buf_size );
	
	//Compress
	auto result = ZSTD_compress( buf.get(), buf_size, in_data, in_size, 11 );
	if( ZSTD_isError(result) )
		return { std::make_unique<char[]>(0), 0 };
	
	return { std::move( buf ), result };
}


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

struct HeaderHeader{
	char magic[4];
	char magic_custom[4];
	uint32_t header_size;
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
		
	public:
	//	Archive( Reader& reader );
		
		auto fileCount() const{ return files.size(); }
		
		void write( Writer& writer );
};

void Archive::write( Writer& writer ){
	//Construct headerheader
	HeaderHeader headerheader;
	headerheader.magic[0] = 'F';
	headerheader.magic[1] = 'x';
	headerheader.magic[2] = 'S';
	headerheader.magic[3] = 'F';
	headerheader.magic_custom[0] = 0;
	headerheader.magic_custom[1] = 0;
	headerheader.magic_custom[2] = 0;
	headerheader.magic_custom[3] = 0;
	
	//Construct header
	auto size = 8 + files.size() * sizeof(File);
	auto buf = std::make_unique<char[]>( size );
	struct HeaderStart{
		uint32_t file_count;
		uint32_t folder_count;
	} start;
	start.file_count = files.size();
	start.folder_count = folders.size();
	std::memcpy( buf.get(), &start, sizeof(HeaderStart) );
	
	for( unsigned i=files.size()-1; i>0; i-- ){
		std::cout << "normal : " << files[i].file_start() << " " << files[i].filesize << " " << files[i].compressed_size << std::endl;
		files[i].compress( files[i-1] );
		std::cout << "changed: " << files[i].file_start() << " " << files[i].filesize << " " << files[i].compressed_size << std::endl;
	}
	if( files.size() > 0 )
		files[0].compress();
	std::memcpy( buf.get() + sizeof(HeaderStart), files.data(), sizeof(File)*files.size() );
	
	//Compress header
	auto compressed = zstd::compress( buf.get(), size );
	headerheader.header_size = compressed.second;
	
	//Write result
	writer.write( &headerheader, sizeof(headerheader) );
	writer.write( compressed.first.get(), compressed.second );
//	writer.write( buf.get(), size );
}


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

unsigned ArchiveConstructor::getFolderPos( std::string dir, bool create ){
	return 0; //TODO:
}

void ArchiveConstructor::addFile( std::string name, std::string dir, unsigned compressed_size, unsigned size ){
	ArchiveFile f;
	f.name = name;
	f.folder_id = getFolderPos( dir );
	f.size = size;
	f.compressed_size = compressed_size;
	files.push_back( f );
}

Archive ArchiveConstructor::createHeader(){
	//TODO: create folder map
	//TODO: sort by how often they are used
	Archive arc;
	arc.folders.push_back( 0 );
	
	uint64_t current_offset = 0;
	for( auto& file : files ){
		File f;
		f.offset = current_offset;
		f.filesize = file.size;
		f.compressed_size = file.compressed_size;
		f.folder = 0; //TODO:
		f.format = 1; //TODO:
		f.flags  = 0; //TODO:
		f.user   = 0; //TODO:
		arc.files.push_back( f );
		
		current_offset += f.compressed_size;
	}
	
	return arc;
}

std::vector<std::pair<QDir,QFileInfo>> allFiles( QDir current ){
	auto files = current.entryList();
	
	std::vector<std::pair<QDir,QFileInfo>> result;
	result.reserve( files.size() );
	
	for( auto& file : files ){
		if( file == "." || file == ".." )
			continue;
		
		auto path = current.absolutePath() + "/" + file;
		QFileInfo file_info( path );
		if( file_info.isDir() ){
			auto subfiles = allFiles( path );
			result.reserve( result.size() + subfiles.size() );
			for( auto& subfile : subfiles )
				result.emplace_back( subfile );
		}
		else
			result.emplace_back( current, file_info );
	}
	
	return result;
}

template<typename T>
struct SimpleIt{
	T object;
	SimpleIt(T object) : object(object) { }
	
	bool operator!=(SimpleIt other) const
		{ return object != other.object; }
	
	SimpleIt& operator++(){
		object++;
		return *this;
	}
	T operator*(){ return object; }
};

template<typename T>
struct CounterIterator{
	T first = {};
	T last;
	CounterIterator(T last) : last(last) {}
	
	SimpleIt<T> begin() const{ return {first}; }
	SimpleIt<T> end()   const{ return {last }; }
};

template<typename T>
CounterIterator<T> upTo(T last){ return {last}; }

class QtWriter : public Writer{
	private:
		QFile file;
	public:
		QtWriter( QString path ) : file(path) { file.open(QIODevice::WriteOnly); }
		
		bool write( const void* buffer, uint64_t amount ) override{
			//TODO: fail if amount larger than int64_t MAX
			return file.write( static_cast<const char*>(buffer), amount ) == int64_t(amount);
		}
		bool seek( uint64_t position ) override
			{ return file.seek( position ); }
};

int main(int argc, char* argv[]){
	QCoreApplication app(argc, argv);
	
	ArchiveConstructor arc;
	auto dir = QDir(app.arguments()[1]);
	
	auto files = allFiles( dir );
	for( auto i : upTo(files.size()) ){
		QFile f( files[i].second.absoluteFilePath() );
		if( !f.open(QIODevice::ReadOnly) ){
			std::cout << "Failed to read: " << files[i].second.absoluteFilePath().toLocal8Bit().constData() << std::endl;
			return -1;
		}
		auto buf = f.readAll();
		
	//	std::cout << files[i].second.absoluteFilePath().toLocal8Bit().constData() << std::endl;
		auto compressed = zstd::compress( buf.data(), buf.size() );
		if( compressed.second > buf.size() )
			std::cout << files[i].second.fileName().toLocal8Bit().constData() << std::endl;
	//	std::cout << "\tCompressed: " << compressed.second << std::endl;
	//	std::cout << "\tNormal    : " << buf.size() << std::endl;
		
		arc.addFile( files[i].second.fileName().toUtf8().constData(), "", compressed.second, buf.size() );
	}
	
	auto header = arc.createHeader();
	QtWriter writer( "test.fxsf" );
	header.write( writer );
	
	return 0;
}