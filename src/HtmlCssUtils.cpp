/* Code from Qt Forum, by Phil Weinstein */
/* https://forum.qt.io/topic/45970/qfont-support-for-css-font-spec-encoding */
/* I only extracted from HTML and re-indented. */
#include "HtmlCssUtils.hpp"
#include <QList>
#include <QStringList>


QString HtmlCssUtils::encodeCssFont (const QFont& refFont)
{
    //-----------------------------------------------------------------------
    // This function assembles a CSS Font specification string from
    // a QFont. This supports most of the QFont attributes settable in
    // the Qt 4.8 and Qt 5.3 QFontDialog.
    //
    // (1) Font Family
    // (2) Font Weight (just bold or not)
    // (3) Font Style (possibly Italic or Oblique)
    // (4) Font Size (in either pixels or points)
    // (5) Decorations (possibly Underline or Strikeout)
    //
    // Not supported: Writing System (e.g. Latin).
    //
    // See the corresponding decode function, below.
    // QFont decodeCssFontString (const QString cssFontStr)
    //-----------------------------------------------------------------------

    QStringList fields; // CSS font attribute fields

    // ***************************************************
    // ***  (1) Font Family: Primary plus Substitutes  ***
    // ***************************************************

    const QString family = refFont.family();

    // NOTE [9-2014, Qt 4.8.6]: This isn't what I thought it was. It
    // does not return a list of "fallback" font faces (e.g. Georgia,
    // Serif for "Times New Roman"). In my testing, this is always
    // returning an empty list.
    //
    QStringList famSubs = QFont::substitutes (family);

    if (!famSubs.contains (family))
	famSubs.prepend (family);

    static const QChar DBL_QUOT ('"');
    const int famCnt = famSubs.count();
    QStringList famList;
    for (int inx = 0; inx < famCnt; ++inx)
    {
	// Place double quotes around family names having space characters,
	// but only if double quotes are not already there.
	//
	const QString fam = famSubs [inx];
	if (fam.contains (' ') && !fam.startsWith (DBL_QUOT))
	    famList << (DBL_QUOT + fam + DBL_QUOT);
	else
	    famList << fam;
    }

    const QString famStr = QString ("font-family: ") + famList.join (", ");
    fields << famStr;

    // **************************************
    // ***  (2) Font Weight: Bold or Not  ***
    // **************************************

    const bool bold = refFont.bold();
    if (bold)
	fields << "font-weight: bold";

    // ****************************************************
    // ***  (3) Font Style: possibly Italic or Oblique  ***
    // ****************************************************

    const QFont::Style style = refFont.style();
    switch (style)
    {
      case QFont::StyleNormal: break;
      case QFont::StyleItalic: fields << "font-style: italic"; break;
      case QFont::StyleOblique: fields << "font-style: oblique"; break;
    }

    // ************************************************
    // ***  (4) Font Size: either Pixels or Points  ***
    // ************************************************

    const double sizeInPoints = refFont.pointSizeF(); // <= 0 if not defined.
    const int sizeInPixels = refFont.pixelSize();     // <= 0 if not defined.
    if (sizeInPoints > 0.0)
	fields << QString ("font-size: %1pt") .arg (sizeInPoints);
    else if (sizeInPixels > 0)
	fields << QString ("font-size: %1px") .arg (sizeInPixels);

    // ***********************************************
    // ***  (5) Decorations: Underline, Strikeout  ***
    // ***********************************************

    const bool underline = refFont.underline();
    const bool strikeOut = refFont.strikeOut();

    if (underline && strikeOut)
	fields << "text-decoration: underline line-through";
    else if (underline)
	fields << "text-decoration: underline";
    else if (strikeOut)
	fields << "text-decoration: line-through";

    const QString cssFontStr = fields.join ("; ");
    return cssFontStr;
}


QFont HtmlCssUtils::decodeCssFontString (const QString& cssFontStr)
{
    //-----------------------------------------------------------------------
    // This function creates a QFont from the provided CSS Font
    // specification string. This supports most of the QFont attributes
    // settable in the Qt 4.8 and Qt 5.3 QFontDialog.
    //
    // (1) Font Family
    // (2) Font Weight (just bold or not)
    // (3) Font Style (possibly Italic or Oblique)
    // (4) Font Size (in either pixels or points)
    // (5) Decorations (possibly Underline or Strikeout)
    //
    // Not supported (defaulted): Writing System (e.g. Latin).
    //
    // See the corresponding encode function, above.
    // QString encodeCssFont (const QFont&)
    //-----------------------------------------------------------------------

    QFont retFont;

    QStringList fields = cssFontStr .split (';');
    const int fieldCnt = fields.count();

    for (int inx = 0; inx < fieldCnt; ++inx)
    {
	const QString field = fields [inx] .trimmed();
	if (field.isEmpty()) continue;
	//----------------------------

	const QStringList keyAndValue = field.split (':');
	if (keyAndValue.count() != 2) continue;
	//-------------------------------------

	const QString key = keyAndValue [0] .trimmed() .toLower();
	const QString val = keyAndValue [1] .trimmed();
	const QString valLower = val .toLower();

	// *************************
	// ***  (1) Font Family  ***
	// *************************

	if (key .contains ("font-family"))
	{
	    QStringList famList = val.split (',');
	    if (!famList.isEmpty())
	    {
		// Use only the first family in a comma separated list
		QString fam = famList [0] .trimmed();

		// Remove leading and trailing double quotes
		static const QChar DBL_QUOT ('"');
		if (fam .startsWith (DBL_QUOT))
		    fam = fam .mid (1) .trimmed();
		if (fam .endsWith (DBL_QUOT))
		{ fam .chop (1); fam = fam.trimmed(); }

		if (!fam.isEmpty())
		    retFont .setFamily (fam);
	    }
	}

	// ********************************
	// ***  (2) Font Weight (Bold)  ***
	// ********************************

	else if (key .contains ("font-weight"))
	{
	    if (valLower .contains ("bold"))
		retFont .setBold (true);
	}

	// ********************************************
	// ***  (3) Font Style (Italic or Oblique)  ***
	// ********************************************

	else if (key .contains ("font-style"))
	{
	    if (valLower .contains ("italic"))
		retFont .setStyle (QFont::StyleItalic);
	    else if (valLower .contains ("oblique"))
		retFont .setStyle (QFont::StyleOblique);
	}

	// ************************************************
	// ***  (4) Font Size: either Pixels or Points  ***
	// ************************************************

	else if (key .contains ("font-size"))
	{
	    bool numOk (false);
	    QString numPart (valLower);
	    numPart.chop (2);
	    const double num = numPart.toDouble (&numOk);

	    if (numOk && (num > 0.0))
	    {
		if (valLower.endsWith ("px"))
		    retFont .setPixelSize (int (num));
		else if (valLower.endsWith ("pt"))
		    retFont .setPointSizeF (num);
	    }
	}

	// ***********************************************
	// ***  (5) Decorations: Underline, Strikeout  ***
	// ***********************************************

	else if (key .contains ("text-decoration"))
	{
	    if (valLower.contains ("underline"))
		retFont .setUnderline (true);
	    if (valLower.contains ("line-through"))
		retFont .setStrikeOut (true);
	}

    } // end for (int inx = 0; inx < fieldCnt; ++inx)

    return retFont;
}
