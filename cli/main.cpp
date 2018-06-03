#include "lib/FxSF/Archive.hpp"
#include "lib/FxSF/ArchiveConstructor.hpp"
#include "lib/FxSF/zstd++.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QDebug>

#include <cxxopts.hpp>

#include <iostream>

struct File{
	QDir dir;
	QFileInfo info;
	
	File( QDir dir, QFileInfo info ) : dir(dir), info(info) {}
};

std::vector<File> allFiles( QDir current, QDir parent ){
	auto files = current.entryList();
	
	std::vector<File> result;
	result.reserve( files.size() );
	
	for( auto& file : files ){
		if( file == "." || file == ".." )
			continue;
		
		auto path = current.absolutePath() + "/" + file;
		QFileInfo file_info( path );
		if( file_info.isDir() ){
			auto subfiles = allFiles( path, parent );
			result.reserve( result.size() + subfiles.size() );
			for( auto& subfile : subfiles )
				result.emplace_back( subfile );
		}
		else
			result.emplace_back( parent, file_info );
	}
	
	return result;
}
std::vector<File> allFiles( QStringList args ){
	std::vector<File> result;
	
	for( auto arg : args ){
		if( QFileInfo( arg ).isDir() ){
			auto files = allFiles( arg, arg ); //TODO: Include folder?
			for( auto file : files )
				result.push_back( file );
		}
		else
			result.emplace_back( QDir( arg ).dirName(), QFileInfo( arg ) );
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


class QtIOWrapper : public FxSF::Reader, public FxSF::Writer{
	private:
		QFile file;
	public:
		QtIOWrapper( QString path, QIODevice::OpenModeFlag mode ) : file(path) { file.open(mode); }
		
		bool read( void* buffer, uint64_t amount ) override{
			//TODO: fail if amount larger than int64_t MAX
			return file.read( static_cast<char*>(buffer), amount ) == int64_t(amount);
		}
		bool write( const void* buffer, uint64_t amount ) override{
			//TODO: fail if amount larger than int64_t MAX
			return file.write( static_cast<const char*>(buffer), amount ) == int64_t(amount);
		}
		bool seek( uint64_t position ) override
			{ return file.seek( position ); }
};

struct QtReader : public QtIOWrapper{
	QtReader( QString path ) : QtIOWrapper( path, QIODevice::ReadOnly ) { }
};

struct QtWriter : public QtIOWrapper{
	QtWriter( QString path ) : QtIOWrapper( path, QIODevice::WriteOnly ) { }
};

QByteArray readFile( QString path ){
	QFile f( path );
	if( !f.open(QIODevice::ReadOnly) ){
		std::cout << "Failed to read: " << path.toLocal8Bit().constData() << std::endl;
		return {};
	}
	return f.readAll();
}

bool copyContentsInto( QFileInfo sourcePath, QFileInfo destination ){
	QFile in(   sourcePath.filePath() );
	QFile out( destination.filePath() );
	
	if( !in .open( QIODevice::ReadOnly  ) )
		return false;
	if( !out.open( QIODevice::WriteOnly | QIODevice::Append ) )
		return false;
	
	QByteArray buf;
	do{
		buf = in.read( 4*1024 );
		out.write( buf );
	} while( buf.size() > 0 );
	
	return true;
}

QString relativeTo( QDir parent, QFileInfo child ){
	auto path = child.absolutePath();
	auto parent_path = parent.absolutePath();
	
	//Special case, we are in the root folder
	if( path.size() <= parent_path.size() )
		return "";
	
	return path.right( path.size() - parent_path.size() - 1 );
}

QString fromFxsfString( FxSF::String s )
	{ return QString( s.start ); }
	
QString folderPath( const FxSF::Archive& arc, unsigned id ){
	auto parent = arc.folderParent(id);
	auto current = fromFxsfString(arc.folderName(id));
	if(id == parent)
		return current;
	return folderPath( arc, parent ) + "/" + current;
}

int compress( std::vector<File> files, QString outpath ){
	FxSF::ArchiveConstructor arc;
	{	QtWriter temp( "temp.dat" );
		//TODO: Make temporary file
		for( auto& file : files ){
			auto buf = readFile( file.info.absoluteFilePath() );
			
			auto compressed = zstd::compress( buf.data(), buf.size() );
			if( compressed.second > uint64_t(buf.size()) )
				std::cout << file.info.fileName().toLocal8Bit().constData() << std::endl;
			temp.write( compressed.first.get(), compressed.second );
			
			arc.addFile(
					file.info.fileName().toUtf8().constData()
				,	relativeTo( file.dir, file.info ).toUtf8().constData()
				,	compressed.second, buf.size()
				);
			//TODO: Empty folders?
		}
	}
	
	auto header = arc.createHeader();
	
	{	QtWriter writer( outpath );
		header.write( writer );
	}
		
	//Append "temp.dat"
	if( !copyContentsInto( {"temp.dat"}, {outpath} ) )
		return -1;
	QFile::remove( "temp.dat" );
	
	return 0;
}

void list_archive( QString path ){
	QtReader reader( path );
	FxSF::Archive in( reader );
	for( auto& file : in ){
		//TODO: Folder name
		std::cout << "Dir: " << folderPath(in, file.folder()).toLocal8Bit().constData() << "/";
		std::cout << file.name().start << "\n";
	}
}

//TODO: Use exceptions instead?
bool extract( QString archive_path, QString output_path ){
	//Read archive
	QtReader reader( archive_path );
	FxSF::Archive in( reader );
	
	//Extract each file
	for( auto& file : in ){
		//Seek to position
		auto offset = file.fileStart() + in.dataOffset();
		reader.seek( offset );
		
		//Create buffer
		auto size = file.compressedSize();
		auto buffer = std::make_unique<char[]>( size );
		
		//Read data
		if( !reader.read( buffer.get(), size ) ){
			std::cout << "File read failed\n";
			return false;
		}
		
		//Decompress
		auto data = zstd::decompress( buffer.get(), size );
		
		//Construct paths and dirs
		auto folder = output_path + folderPath(in, file.folder());
		auto filename = fromFxsfString( file.name() );
		QDir().mkpath( folder );
		
		//Write data to resulting file
		QFile out_file( folder + "/" + filename );
		if( !out_file.open( QIODevice::WriteOnly ) ){
			std::cout << "Couldn't open output file: " << filename.toLocal8Bit().constData() << '\n';
			return false;
		}
		out_file.write( data.first.get(), data.second );
	}
	
	return true;
}

int main(int argc, const char* argv[]){
	cxxopts::Options options( "FxSF", "CLI archiver for the FxSF format" );
	options.add_options()
		( "c,compress", "Compress files" )
		( "x,extract",  "Extract files" )
		( "l,list",  "Show contents of archive", cxxopts::value<std::string>() )
		( "o,outpath",  "Extract files", cxxopts::value<std::string>() )
		;
	auto result = options.parse( argc, argv );
	
	//Get all file arguments, and convert to QString with the "correct" encoding
	//TODO: Encoding will still fail on Windows if they fall outside the 8-bit encoding
	QStringList args;
	for( int i=1; i<argc; i++ )
		args << QString::fromLocal8Bit( argv[i] );
	
	
	if( (result.count( "compress" ) + result.count( "extract" ) + result.count( "list" )) != 1 ){
		std::cout << "Select one of compress or extract options\n";
		return -1;
	}
	
	if( result.count( "compress" ) ){
		if( !compress( allFiles( args ), result["outpath"].as<std::string>().c_str() ) )
			return -1;
	}
	if( result.count( "extract" ) ){
		for( auto arg : args )
			if( !extract( arg, result["outpath"].as<std::string>().c_str() ) )
				return -1;
	}
	if( result.count( "list" ) ){		
		list_archive( result["list"].as<std::string>().c_str() );
	}
	
	return 0;
}