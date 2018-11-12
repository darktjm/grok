/* whatsThis mode may not support the help text options */
"/* Note:  Motif automatically selects a contrasting foreground color if\n"
"          none was specified, but Qt does not.  Always set both. */\n"
"/* Former background:  global default background color */\n"
"/* Note that setting this globally (using QWidget) causes strange behavior.*/\n"
"/* QWidget { background-color: #a0a0a0; color: black; } */\n"
"/* Even setting this for these few things makes things uugly, so just don't */\n"
// Why doesn't pretty much everything inherit from the first two?
// I have AA_UseStyleSHeetPropagationInWIdgetStyles set, but it doesn't
// seem to do anything.
"/* QMainWindow, QDialog, QMenu, .QWidget, .QFrame, QTreeWidget {\n"
"     background-color: #a0a0a0; color: black;\n"
"} */\n"
"/* And menus in the default style no longer highlight */\n"
"/* QMenu:selected, QMenu::tearoff:selected { background-color: black; color: #a0a0a0; } */\n"
"/* Former colStd:  standard foreground */\n"
/*   Foreground color of card next/prev pixmaps% */
/*   Foreground color of help text */
/*   Foreground color of outer border around bars in bar chart */
"*[colStd=\"true\"] { color: #101010; }\n"
"/* Former colBack:  standard background */\n"
/*   Foreground of Next/Prev search buttons% */
/*   Background of chart */
"*[colBack=\"true\"] { background-color: #b4b4b4; }\n"
/*   Background of read-only text widgets in card form */
"QLineEdit:read-only { background-color: #b4b4b4; color: black; }\n"
"/* BUG: QTextEdit's read-only flag is not detectable by :ready-only */\n"
"QTextEdit[readOnly=\"true\"] { background-color: #b4b4b4; color: black; }\n"
"/* Former colSheet:  paper-like text scroll areas */\n"
/*   Background of help text */
/*   Background of text editor text */
"*[colSheet=\"true\"], QTextEdit[colSheet=\"true\"] { background-color: #f0f0f0; color: black; }\n"
"/* Formerly colSheet, but can't be any more for technical reasons */\n"
/*   Foreground color of inner border around selected bar in bar chart */
"GrokChart { qproperty-hlcolor: #f0f0f0; }\n"
"/* Former colTextBack:  standard background of text widgets */\n"
/*   Background of editable text widgets */
"QLineEdit { background-color: #c07070; color: white; }\n"
"QTextEdit { background-color: #c07070; color: white; }\n"
"/* Former colToggle:  toggle button diamond color */\n"
/*   Checkbox and radio button background when selected */
/*     This may need work, since it replaces graphics with a solid square */
"/* This a bad idea, because it overrides style graphics completely */\n"
"/* QSS sucks in this regard, */\n"
"/* QAbstractButton::indicator:checked, QMenu::indicator:checked {\n"
"   background-color: #c00000; } */\n"
"GrokCanvas {\n"
"/* Former colCanvBack:  canvas background */\n"
/*   Form editor canvas background */
"    background-color: #b0b0b0;\n"
"/* Former colCanvFrame:  canvas frame around boxes */\n"
/*   Form editor canvas widget frame */
/*   Form editor canvas divider between static & card area */
/*   Form editor canvas frames around widgets */
"    color: #101010;\n"
"/* Former colCanvBox:  canvas box representing item */\n"
/*   Form editor canvas background color for unselected widgets */
"    qproperty-boxcolor: #808090;\n"
"/* Former colCanvSel:  canvas selected box */\n"
/*   Form editor canvas background color for selected widgets */
"    qproperty-selcolor: #e0e060;\n"
"/* Former colCanvText:  canvas text inside box */\n"
/*   Form editor canvas text color inside widgets */
"    qproperty-textcolor: #101010;\n"
"}\n"
"GrokChart {\n"
"    /* Former colChartAxis:  chart axis color */\n"
/*   Chart axis color */
"    qproperty-axiscolor: #101010;\n"
"    /* Former colChartGrid:  chart grid color */\n"
/*   Chart grid color */
"    qproperty-gridcolor: #707070;\n"
"    /* Former colChartBox:  border of each bar in the chart */\n"
"    /* Note that the chart used to use colStd, but now it uses this. */\n"
/*   Formerly unused (used colStd instead) */
"    qproperty-boxcolor: #000000;\n"
"    /* Former colChart0..colChart7:  eight colors for bars in chart */\n"
/* Color0 .. Color7 for chart bar colors */
"    qproperty-color0:         #306080; /* blue */\n"
"    qproperty-color1:         #a04040;	/* red */\n"
"    qproperty-color2:         #a08040;	/* yellow */\n"
"    qproperty-color3:         #408030;	/* green */\n"
"    qproperty-color4:         #804080;	/* magenta */\n"
"    qproperty-color5:         #408070;	/* cyan */\n"
"    qproperty-color6:         #a06030;	/* orange */\n"
"    qproperty-color7:         #f0f0f0;	/* white */\n"
"}\n"
"\n"
"/* Former fontList:  standard font; menus, text */\n"
/*   Search mode widget */
/*   Info line/mod time */
/*   A-Z/misc/all buttons */
/*   Default font where not specified */
"QWidget { font: 14px \"Helvetica\"; }\n"
"/* Former menubar*fontList:  default font for menu bar */\n"
/*   Default font for just the menu bar (not menu pulldowns)% */
"QMenuBar { font: bold 14px \"Helvetica\"; }\n"
"/* Former helpFont:  pretty font for help popups */\n"
/*   Font in text area displaying help text% */
"*[helpFont=\"true\"] { font: 14px \"Helvetica\"; }\n"
"/* Former helvFont:  Helv font for widgets */\n"
/*   Search text widget% */
/*   User widget font% */
"*[helvFont=\"true\"] { font: 14px \"Helvetica\"; }\n"
"/* Former helvObliqueFont:  HelvO font for widgets */\n"
/*   User widget font% */
"*[helvObliqueFont=\"true\"] { font: oblique 14px \"Helvetica\"; }\n"
"/* Former helvSmallFont:  HelvN font for widgets */\n"
/*   Text in form editor canvas% */
/*   User widget font% */
"*[helvSmallFont=\"true\"] { font: 12px \"Helvetica\"; }\n"
"/* Former helvLargeFont:  HelvB font for widgets */\n"
/*   User widget font% */
"*[helvLargeFont=\"true\"] { font: 18px \"Helvetica\" }\n"
"/* Former courierFont:  courier font for widgets */\n"
/*   User widget font% */
"*[courierFont=\"true\"] { font: 14px \"Courier\"; }\n"
"/* Note that this used to also be applied where text must line up, but */\n"
"/* layout widgets are used instead */\n"
/*   Template file name list widget% */
/*   Summary header and list widget% */
/*   Text editor's text widget% */
"/* QTreeWidget, QTableWidget { font: 14px \"Courier\"; } */\n"
"/* Former labelFont - font used for charts */\n"
"/* Note that charts don't have text right now, so this is unused. */\n"
/*   Unused */
"GrokChart { font: 12px \"Helvetica\"; }\n"
