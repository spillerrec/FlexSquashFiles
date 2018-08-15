/*	This file is part of FxSF, which is free software and is licensed
 * under the terms of the GNU GPL v3.0. (see http://www.gnu.org/licenses/ ) */

#include "../shared/core_routines.hpp"
#include "../shared/qt_file_utils.hpp"

#include <cxxopts.hpp>

#include <iostream>
#include <stdexcept>

void ensure_only_one_method( cxxopts::ParseResult& result ){
	int methods = 0;
	methods += result.count( "compress" );
	methods += result.count( "extract" );
	methods += result.count( "list" );
	methods += result.count( "version" );
	methods += result.count( "help" );
	methods += result.count( "verify" );
	
	if( methods == 0 )
		std::cout << "Please select a mode of operation or see --help for usage\n";
	if( methods > 1 )
		std::cout << "Error: only one Method can be used at once!\n";
	
	if( methods != 1 )
		std::exit( -1 );
}

int main(int argc, const char* argv[]){
	try{
		cxxopts::Options options( "FxSF", "CLI archiver for the FxSF format" );
		options.add_options( "Methods" )
			( "c,compress", "Compress files" )
			( "x,extract" , "Extract files" )
			( "l,list"    , "Show contents of archive", cxxopts::value<std::string>() )
			( "v,version" , "Show FxSF versions" )
			( "h,help"    , "Show this help text" )
			( "verify"    , "Verify file contents using its checksums" )
			;
		options.add_options( "Options" )
			( "a,autodir", "Automatically create extra directories" )
			( "o,outpath", "Output file/directory", cxxopts::value<std::string>() )
			( "s,select" , "Regex limiting which files are affected", cxxopts::value<std::string>() )
			;
		auto result = options.parse( argc, argv );
		ensure_only_one_method( result );
		
		//Get all file arguments, and convert to QString with the "correct" encoding
		//TODO: Encoding will still fail on Windows if they fall outside the 8-bit encoding
		QStringList args;
		for( int i=1; i<argc; i++ )
			args << QString::fromLocal8Bit( argv[i] );
		
		//Do common options here
		auto getQString = [&](const char* para){
				return QString::fromLocal8Bit( result[para].as<std::string>().c_str() );
			};
		bool autodir = result.count( "autodir" ) > 0;
		//TODO: Regex limiter
		
		//Compressing
		if( result.count( "compress" ) ){
			//Get parameters
			auto files = allFiles( args );
			CompressSettings settings;
			settings.autodir = autodir;
			//TODO: max size
			
			//TODO: Default output path when not specified
			auto outpath = getQString( "outpath" );
			
			if( !compress( files, outpath, settings ) )
				return -1;
		}
		
		//Extracting
		if( result.count( "extract" ) ){
			ExtractSettings settings;
			settings.autodir = autodir;
			//TODO: ignore errors
			
			for( auto arg : args ){
				//TODO: Default output path when not specified
				auto outpath = getQString( "outpath" );
				
				if( !extract( arg, outpath, settings ) && !settings.ignore_errors )
					return -1;
			}
		}
		
		//Listing
		if( result.count( "list" ) ){		
			list_archive( getQString( "list" ) );
		}
		
		//Version info
		if( result.count( "version" ) ){		
			std::cout << "FxSF v0.1\n";
			std::cout << "CLI v0.1\n";
		}
		
		//Help
		if( result.count( "help" ) ){
			std::cout << options.help( options.groups() ).c_str();
		}
		
		//Verify
		if( result.count( "verify" ) ){		
			std::cout << "Not yet implemented\n";
		}
		
		return 0;
	}
	catch( std::exception& e ){
		std::cout << "Error: " << e.what() << '\n';
		return -1;
	}
}