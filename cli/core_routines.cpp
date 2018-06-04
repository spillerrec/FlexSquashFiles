/*	This file is part of FxSF, which is free software and is licensed
 * under the terms of the GNU GPL v3.0. (see http://www.gnu.org/licenses/ ) */

#include "core_routines.hpp"

#include "qt_file_utils.hpp"

#include "lib/FxSF/Archive.hpp"
#include "lib/FxSF/ArchiveConstructor.hpp"
#include "lib/FxSF/zstd++.hpp"

#include <QString>
#include <QDir>
#include <QFileInfo>

#include <iostream>


static QString relativeTo( QDir parent, QFileInfo child ){
	auto path = child.absolutePath();
	auto parent_path = parent.absolutePath();
	
	//Special case, we are in the root folder
	if( path.size() <= parent_path.size() )
		return "";
	
	return path.right( path.size() - parent_path.size() - 1 );
}

static QString fromFxsfString( FxSF::String s )
	{ return QString( s.start ); }
	
static QString folderPath( const FxSF::Archive& arc, unsigned id ){
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


