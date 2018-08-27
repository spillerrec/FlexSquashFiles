/*	This file is part of FxSF, which is free software and is licensed
 * under the terms of the GNU GPL v3.0. (see http://www.gnu.org/licenses/ ) */

#include "../shared/core_routines.hpp"
#include "../shared/qt_file_utils.hpp"

#include <stdexcept>
#include <QApplication>
#include <QMainWindow>

int main(int argc, char* argv[]){
	QApplication app( argc, argv );
	
	QMainWindow w;
	w.show();
	
	return app.exec();
}