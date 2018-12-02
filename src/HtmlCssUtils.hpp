/* Code from Qt Forum, by Phil Weinstein */
/* https://forum.qt.io/topic/45970/qfont-support-for-css-font-spec-encoding */
/* I only extracted from HTML and re-indented. */
#pragma once
#ifndef HtmlCssUtilsINCLUDED
# define HtmlCssUtilsINCLUDED

# include <QString>
# include <QFont>

namespace HtmlCssUtils
{
    QString encodeCssFont (const QFont& refFont);
    QFont decodeCssFontString (const QString& cssFontStr);
}

#endif
