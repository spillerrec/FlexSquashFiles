/*	This file is part of FxSF, which is free software and is licensed
 * under the terms of the GNU GPL v3.0. (see http://www.gnu.org/licenses/ ) */

#include "core_routines.hpp"
#include "qt_file_utils.hpp"

#include <cxxopts.hpp>

#include <iostream>

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