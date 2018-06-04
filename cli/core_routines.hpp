/*	This file is part of FxSF, which is free software and is licensed
 * under the terms of the GNU GPL v3.0. (see http://www.gnu.org/licenses/ ) */

#ifndef CORE_ROUTINES_HPP
#define CORE_ROUTINES_HPP

#include <vector>
class QString;
class File;

int compress( std::vector<File> files, QString outpath );
void list_archive( QString path );
bool extract( QString archive_path, QString output_path );

#endif
