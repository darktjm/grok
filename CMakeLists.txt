cmake_minimum_required(VERSION 3.1.0)

project(grok CXX)

set(GROK_LIB "${CMAKE_INSTALL_PREFIX}/share/grok"
    CACHE PATH "Path to grok's support files")
set(GROK_DOC "${CMAKE_INSTALL_PREFIX}/share/doc/grok"
    CACHE PATH "Where to install grok's documentation")
mark_as_advanced(GROK_DOC)

# The default is to build with no debug or optimization options
# A better default is to build with both, if available
if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
	if(CMAKE_CXX_FLAGS_RELWITHDEBINFO)
		set(CMAKE_BUILD_TYPE RelWIthDebInfo CACHE STRING "" FORCE)
	else()
		set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
	endif()
endif()

function(mangle_doc file dest)
	set(ifile "${CMAKE_CURRENT_SOURCE_DIR}/${file}")
	string(REGEX MATCH "[^/]*\$" base ${file})
	set(ofile "${CMAKE_CURRENT_BINARY_DIR}/mangled/${base}")
	configure_file(${ifile} ${ofile})
	install(FILES ${ofile} DESTINATION ${dest})
endfunction()

add_subdirectory(src)
add_subdirectory(doc)
add_subdirectory(misc)
add_subdirectory(demo)

install(FILES README.md TODO.md HISTORY DESTINATION ${GROK_DOC})
