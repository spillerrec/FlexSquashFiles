#include "lib/FxSF/Archive.hpp"
#include "lib/FxSF/ArchiveConstructor.hpp"
#include "lib/FxSF/zstd++.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>


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

class QtWriter : public FxSF::Writer{
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
	
	FxSF::ArchiveConstructor arc;
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