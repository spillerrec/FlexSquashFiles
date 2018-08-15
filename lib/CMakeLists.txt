cmake_minimum_required(VERSION 3.0)

set(SOURCES_LIB
	FxSF/Archive.cpp
	FxSF/ArchiveConstructor.cpp
	FxSF/checksum.cpp
	FxSF/zstd++.cpp
	)

add_library(fxsf ${SOURCES_LIB})
target_link_libraries(fxsf -lzstd)

set_property(TARGET fxsf    PROPERTY CXX_STANDARD 14)
set_property(TARGET fxsf    PROPERTY CXX_STANDARD_REQUIRED ON)
