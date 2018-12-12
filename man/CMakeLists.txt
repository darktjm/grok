set(GROK_MAN "${CMAKE_INSTALL_PREFIX}/share/man/man1"
    CACHE PATH "Where to install the man page")
mark_as_advanced(GROK_MAN)
mangle_doc(grok.1 ${GROK_MAN})
