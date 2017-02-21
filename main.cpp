#include <QCoreApplication>
#include <QDir>

#include <iostream>
#include <memory>


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
	
	void compress( File previous ){
		offset -= previous.file_end();
		filesize -= compressed_size;
	}
	void decompress( File previous ){
		offset += previous.file_end();
		filesize += compressed_size;
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
		virtual bool write( void* buffer, uint64_t amount ) = 0;
		virtual bool seek( uint64_t position ) = 0;
};

class Archive{
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
		Archive( Reader& reader );
		
		auto fileCount() const{ return files.size(); }
};


class ArchiveConstructor{
	private:
		
	
	public:
		
};

std::vector<std::pair<QDir,QFileInfo>> allFiles( QDir current ){
	auto files = current.entryList();
	
	std::vector<std::pair<QDir,QFileInfo>> result;
	result.reserve( files.size() );
	
	for( auto& file : files ){
		if( QFileInfo(file).isDir() && file != "." && file != ".." ){
			auto subfiles = allFiles( current.path() + "/" + file );
			result.reserve( result.size() + subfiles.size() );
			for( auto& subfile : subfiles )
				result.emplace_back( subfile );
		}
		else
			result.emplace_back( current, QFileInfo{file} );
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


int main(int argc, char* argv[]){
	QCoreApplication app(argc, argv);
	
//	ArchiveConstructor arc;
	auto dir = QDir(app.arguments()[1]);
	auto temp_dir = "temp/"; //TODO:
	
	auto files = allFiles( dir );
	auto it = upTo(files.size());
	for( auto i : it ){
		std::cout << i << std::endl;
	}
	
	return 0;
}