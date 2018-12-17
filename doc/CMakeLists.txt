set(docs
	index.html icon.jpg intro.html formedit.html grammar.html
	export.html files.html
)
foreach(doc ${docs})
	if(doc MATCHES "\\.html\$")
		mangle_doc(${doc} ${GROK_DOC}/html)
	elseif(doc STREQUAL "CMakeLists.txt")
	else()
		install(FILES ${doc} DESTINATION ${GROK_DOC}/html)
	endif()
endforeach()

set(GROK_MAN "${CMAKE_INSTALL_PREFIX}/share/man/man1"
    CACHE PATH "Where to install the man page")
mark_as_advanced(GROK_MAN)
mangle_doc(grok.1 ${GROK_MAN})
