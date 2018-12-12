set(GROK_ICON "${CMAKE_INSTALL_PREFIX}/share/icons"
    CACHE PATH "Where to install the icon")
set(GROK_DESKTOP "${CMAKE_INSTALL_PREFIX}/share/applications"
    CACHE PATH "Where to install the desktop file")
mark_as_advanced(GROK_ICON GROK_DESKTOP)

mangle_doc(grok.hlp ${GROK_LIB})
install(FILES grok.desktop DESTINATION ${GROK_DESKTOP})
install(FILES Grok.xpm DESTINATION ${GROK_ICON})
