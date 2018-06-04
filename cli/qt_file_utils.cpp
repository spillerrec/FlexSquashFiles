/*	This file is part of FxSF, which is free software and is licensed
 * under the terms of the GNU GPL v3.0. (see http://www.gnu.org/licenses/ ) */

#include "qt_file_utils.hpp"

#include <iostream>


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

