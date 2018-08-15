cmake_minimum_required(VERSION 3.0)

find_package(Qt5Core REQUIRED)

set(SOURCES_SHARED
	shared/core_routines.cpp
	shared/qt_file_utils.cpp
	)
set(SOURCES_CLI
	cli/main.cpp
	)

include_directories("${PROJECT_SOURCE_DIR}/lib")
add_executable(fxsf_cli 
	${SOURCES_SHARED}
	${SOURCES_CLI}
	)
target_link_libraries(fxsf_cli fxsf Qt5::Core)

# C++14
set_property(TARGET fxsf_cli PROPERTY CXX_STANDARD 14)
set_property(TARGET fxsf_cli PROPERTY CXX_STANDARD_REQUIRED ON)
