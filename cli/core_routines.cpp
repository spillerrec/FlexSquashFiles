/*	This file is part of FxSF, which is free software and is licensed
 * under the terms of the GNU GPL v3.0. (see http://www.gnu.org/licenses/ ) */

#include "core_routines.hpp"

#include "qt_file_utils.hpp"

#include "lib/FxSF/Archive.hpp"
#include "lib/FxSF/ArchiveConstructor.hpp"
#include "lib/FxSF/checksum.hpp"
#include "lib/FxSF/zstd++.hpp"

#include <QString>
#include <QDir>
#include <QFileInfo>

#include <iostream>


static QString relativeTo( QDir parent, QFileInfo child, QString root="" ){
	auto path = child.absolutePath();
	auto parent_path = parent.absolutePath();
	
	//Special case, we are in the root folder
	if( path.size() <= parent_path.size() )
		return root;
	
	auto relative = path.right( path.size() - parent_path.size() - 1 );
	if( root.size() > 0 )
		relative = root + '/' + relative;
	
	return relative;
}

//Convert from UTF-8 FxSF string to QString
static QString fromFxsfString( FxSF::String s )
	{ return QString( s.start ); }
	
static QString folderPath( const FxSF::Archive& arc, unsigned id ){
	//Recursively construct the complete folder path
	auto parent = arc.folderParent(id);
	auto current = fromFxsfString(arc.folderName(id));
	if(id == parent)
		return current;
	return folderPath( arc, parent ) + "/" + current;
}

int compress( std::vector<File> files, QString outpath, CompressSettings settings ){
	//TODO: Fail if no files
	//TODO: Add everything into a folder called <outpath> if --autodir specified
	QString base_folder = "";
	//TODO: get outpath without extension for this
	//TODO: Rename base folder if everything is in one folder?
	
	FxSF::ArchiveConstructor arc;
	{	QtWriter temp( "temp.dat" );
		//TODO: Make temporary file
		for( auto& file : files ){
			auto buf = readFile( file.info.absoluteFilePath() );
			
			auto qstr = []( QString s )
				{ return std::string( s.toUtf8().constData() ); };
			FxSF::ArchiveFileInfo info(
					qstr(file.info.fileName())
				,	qstr(relativeTo( file.dir, file.info, base_folder ))
				,	buf.size()
				);
			
			info.setChecksum( checksum::crc32( buf.data(), buf.size() ) );
			
			//Try compressing the data
			auto compressed = zstd::compress( buf.data(), buf.size() );
			
			if( compressed.second < uint64_t(buf.size()) ){
				//Write compressed stream if small enough
				info.setCompression( FxSF::Compressor::ZSTD, compressed.second );
				
				temp.write( compressed.first.get(), compressed.second );
			}
			else //Write uncompressed data
				temp.write( buf.data(), buf.size() );
			
			arc.addFile( info );
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
		std::cout << fromFxsfString( file.name() ).toLocal8Bit().constData() << " - " << file.checksum() << "\n";
	}
}

//TODO: Use exceptions instead?
bool extract( QString archive_path, QString output_path, ExtractSettings settings ){
	//Read archive
	QtReader reader( archive_path );
	FxSF::Archive in( reader );
	
	//TODO: --autodir: put everything in a folder if multiple files in root
	QString extra_path = "";
	//TODO: count amount of files in root
	
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
			if( settings.ignore_errors )
				continue;
			else
				return false;
		}
		
		//Decompress
		if( file.isCustomCodec() ){
			std::cout << "Using custom codec, bailing...\n";
			return false; //TODO: Allow extracting the file
		}
		
		//Decode data
		std::pair<std::unique_ptr<char[]>, uint64_t> data;
		switch( file.getFormat() ){
			case FxSF::Compressor::NONE:
					data = std::make_pair( std::move( buffer ), size );
				break;
			case FxSF::Compressor::ZSTD:
					data = std::move( zstd::decompress( buffer.get(), size ) );
				break;
			default:
				std::cout << "Not yet implemented codec\n";
				if( settings.ignore_errors )
					continue;
				else
					return false;
		}
		
		//Construct paths and dirs
		auto folder = output_path + extra_path + folderPath(in, file.folder());
		auto filename = fromFxsfString( file.name() );
		QDir().mkpath( folder );
		
		//Verify
		auto checksum = checksum::crc32( data.first.get(), data.second );
		if( checksum != file.checksum() ){
			std::cout << "Checksum failed for " << filename.toLocal8Bit().constData() << ": " << checksum << " vs. " << file.checksum() << '\n';
			//TODO: Checksum formatting
			if( settings.ignore_errors )
				continue;
			else
				return false;
		}
		
		//Write data to resulting file
		QFile out_file( folder + "/" + filename );
		if( !out_file.open( QIODevice::WriteOnly ) ){
			std::cout << "Couldn't open output file: " << filename.toLocal8Bit().constData() << '\n';
			if( settings.ignore_errors )
				continue;
			else
				return false;
		}
		out_file.write( data.first.get(), data.second );
	}
	
	return true;
}


