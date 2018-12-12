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
