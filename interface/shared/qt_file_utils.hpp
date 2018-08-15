/*	This file is part of FxSF, which is free software and is licensed
 * under the terms of the GNU GPL v3.0. (see http://www.gnu.org/licenses/ ) */

#ifndef QT_FILE_UTILS_HPP
#define QT_FILE_UTILS_HPP

#include <FxSF/Archive.hpp>

#include <QDir>
#include <QFile>
#include <QFileInfo>



struct File{
	QDir dir;       ///File is relative to this directory
	QFileInfo info; ///The file referenced
	
	File( QDir dir, QFileInfo info ) : dir(dir), info(info) {}
};

std::vector<File> allFiles( QDir current, QDir parent );
std::vector<File> allFiles( QStringList args );


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

QByteArray readFile( QString path );
bool copyContentsInto( QFileInfo sourcePath, QFileInfo destination );

#endif
