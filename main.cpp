#include "lib/FxSF/Archive.hpp"
#include "lib/FxSF/ArchiveConstructor.hpp"
#include "lib/FxSF/zstd++.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QDebug>


#include <iostream>


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

int main(int argc, char* argv[]){
	QCoreApplication app(argc, argv);
	
	FxSF::ArchiveConstructor arc;
	auto dir = QDir(app.arguments()[1]);
	
	auto files = allFiles( dir );
	
	{	QtWriter temp( "temp.dat" );
		//TODO: Make temporary file
		for( auto& file : files ){
			auto buf = readFile( file.second.absoluteFilePath() );
			
			auto compressed = zstd::compress( buf.data(), buf.size() );
			if( compressed.second > uint64_t(buf.size()) )
				std::cout << file.second.fileName().toLocal8Bit().constData() << std::endl;
			temp.write( compressed.first.get(), compressed.second );
			
			arc.addFile(
					file.second.fileName().toUtf8().constData()
				,	relativeTo( dir, file.second ).toUtf8().constData()
				,	compressed.second, buf.size()
				);
			//TODO: Empty folders?
		}
	}
	
	auto header = arc.createHeader();
	
	{	QtWriter writer( "test.fxsf" );
		header.write( writer );
	}
		
	//Append "temp.dat"
	if( !copyContentsInto( {"temp.dat"}, {"test.fxsf"} ) )
		return -1;
	
	{	QtReader reader( "test.fxsf" );
		FxSF::Archive in( reader );
		for( auto& file : in ){
			//TODO: Folder name
			std::cout << file.name().start << "\n";
		}
	}
	
	return 0;
}