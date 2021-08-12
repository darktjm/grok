add_compile_definitions(LIB="${GROK_LIB}" PATH=LIB)

set(GROK_BIN "${CMAKE_INSTALL_PREFIX}/bin"
    CACHE PATH "Where to install the grok binary")
mark_as_advanced(GROK_BIN)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTORCC ON)
find_package(Qt5 COMPONENTS Widgets PrintSupport REQUIRED)

# Needed for private interface to retrieve standard button names
# This will likely break in the near future.
foreach(dir ${Qt5Widgets_INCLUDE_DIRS})
	foreach(lib Core Gui)
		if(dir MATCHES "/Qt${lib}\$")
			include_directories(${dir}/${Qt5${lib}_VERSION}
					    ${dir}/${Qt5${lib}_VERSION}/Qt${lib})
		endif()
	endforeach()
endforeach()

# For Grok.xpm
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../misc)

# From auto-generated grok.pro:
# Die on use of any symbols deprecated before version 6
add_compile_definitions(QT_DISABLE_DEPRECATED_BEFORE=0x060000)
# Warn about any other deprecated symbols
add_compile_definitions(QT_DEPRECATED_WARNINGS)

# CMake doesn't support yacc or byacc, but grok needs bison now, anyway:
# default yacc on gentoo defines yyparse with C linkage, but I use C++
# http://dinosaur.compilertools.net/#yacc  version 1.9.1
# byacc works correctly, but isn't reentrant (even with -P and YYPURE=1)
# https://invisible-island.net/byacc/byacc.html version 20180609
# bison works correctly, and is reentrant version 3.2.1 (and probably eariler)
# I also now use bison-specific features (%code, %destructor, purity),
# only some of which could be done differently with great effort
find_package(BISON REQUIRED)
BISON_TARGET(parser parser.y
	     # CMake puts output in source dir by default, which I don't like
             ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp
	     DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.h
	     VERBOSE ${CMAKE_CURRENT_BINARY_DIR}/parser.output}
	     COMPILE_FLAGS "-p parser")
# This suppresses a cmake warning
set_property(SOURCE ${BISON_parser_OUTPUTS} PROPERTY SKIP_AUTOMOC ON)

include(FindPkgConfig)
pkg_search_module(ODBC odbc libiodbc)
if(ODBC_FOUND)
  set(odbc_SRCS formodbc.c odbc-supt.c)
  include_directories($(ODBC_INCLUDE_DIRS))
  include(FindBoost)
  # can't give "COMPONENTS preprocessor", so just assume it's there
  find_package(Boost REQUIRED)
  include_directories(${Boost_INCLUDE_DIRS})
  # I use asprintf in odbc-supt.c; use GNU version if available
  include(CheckCXXSourceCompiles)
  check_cxx_source_compiles([[
	#define _GNU_SOURCE
	#include <stdio.h>
	
	int main(void)
	{
		char *s;
		asprintf(&s, "hello, world!");
		return 0;
	}
	]] HAS_ASPRINTF)
endif()

# Prevent cmake from recompiling the same code for every executable
set(grok_common_SRCS
	chart.c convert.c dbase.c dbfile.c formfile.c formop.c
	util.c layout-qss.h ${odbc_SRCS}
)

set(grok_private_SRCS
	canvdraw.c cardwin.c chartdrw.c editwin.c eval.c evalfunc.c
	formwin.c help.c main.c mainwin.c popup.c prefwin.c query.c
	querywin.c refwin.c sectwin.c sumwin.c template.c templmk.c
	templwin.c HtmlCssUtils.cpp saveprint.c ${BISON_parser_OUTPUT_SOURCE}
	canv-widget.h chart-widget.h
#	grok-res.qrc
)

set(grok_SRCS ${grok_common_SRCS} ${grok_private_SRCS})
# I don't want to rename the original C sources
# I still can't do this by simply changing extensions-related vars, but
# at least I don't have to copy compiler info like in qmake.
set(grok_C ${grok_SRCS})
list(FILTER grok_C INCLUDE REGEX "\\.c\$")
set_source_files_properties(${grok_C} PROPERTIES LANGUAGE CXX)
# note that clang++ will generate a warning for every file; suppress
# this by adding -xc++ to CMAKE_CXX_FLAGS and -xnone to CMAKE_EXE_LINKER_FLAGS.
# -xnone may also need to be added to CMAKE_MODULE_LINKER_FLAGS,
# CMAKE_SHARED_LINKER_FLAGS and CMAKE_STATIC_LINKER_FLAGS, depending.
# The -xc++ suppresses the warning and the -xnone prevents massive link errors
# On the other hand, just suppressing the warning with -Wno-deprecated
# is easier and also works, at least until they decide to remove that "feature".

set(grok_lib_SRCS ${grok_common_SRCS})
add_library(groklib STATIC ${grok_lib_SRCS})
target_link_libraries(groklib Qt5::Widgets Qt5::PrintSupport ${ODBC_LIBRARIES})

add_executable(grok ${grok_private_SRCS})
target_link_libraries(grok Qt5::Widgets Qt5::PrintSupport ${ODBC_LIBRARIES} groklib)

if(ODBC_FOUND)
  # here is a demo/test program, not installed
  set_source_files_properties(odbc-types.c PROPERTIES LANGUAGE CXX)
  add_executable(odbc-types odbc-types.c odbc-supt.c)
  target_link_libraries(odbc-types ${ODBC_LIBRARIES})

  # grok isn't ready yet, so here's another test/demo program
  add_executable(odbc-test odbc-test.cpp)
  target_link_libraries(odbc-test Qt5::Widgets ${ODBC_LIBRARIES} groklib)
endif()

install(TARGETS grok DESTINATION ${GROK_BIN})

# Following is a test.  Only a test.  I'm not using iconv yet.
find_package(Iconv)
if(Iconv_FOUND)
	include_directories(${Iconv_INCLUDE_DIRS})
	target_link_libraries(grok ${Iconv_LIBRARIES})
	include(CheckCXXSourceRuns)
	set(CMAKE_REQUIRED_INCLUDES ${Iconv_INCLUDE_DIRS})
	set(CMAKE_REQUIRED_LIBRARIES ${Iconv_LIBRARIES})
	set(langinfo_src [[
		#include <stdio.h>
		#include <stdlib.h>
		#include <iconv.h>
		#include <locale.h>
		#include <langinfo.h>
		
		int main(void)
		{
			setlocale(LC_CTYPE, "");
			const char *s = nl_langinfo(CODESET);
			if(!s)
				return 1;
			iconv_t i = iconv_open(s, "UTF-8");
			if(i == (iconv_t)-1)
				return 2;
			iconv_close(i);
			i = iconv_open("UTF-8", s);
			if(i == (iconv_t)-1)
				return 3;
			iconv_close(i);
			return 0;
		}
		]])
	# running won't work for cross-compiling, so I'll just assume it works
#	check_cxx_source_runs("${langinfo_src}" LANGINFO_WORKS)
	check_cxx_source_compiles("${langinfo_src}" LANGINFO_WORKS)
endif()

configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
