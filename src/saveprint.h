// copied from https://stackoverflow.com/questions/12253363/persisting-serializing-qprinter-qprintdialog-between-execution
/* The only changes I made were reindenting and #if-ing out the samples */
/* Qt doesn't provide a way of saving print settings between runs.
 * I was going to write my own, but then I found this and decided to use it.
 * rather than writing my own */
///////////////////////////////////////////////////////////////////////////////
// Write all available Attributes from QPrinter into stream
///////////////////////////////////////////////////////////////////////////////

template <typename t> void  writeStreamElement(QDataStream &os, t param)
{
	int i = static_cast<int>(param);
	os << i;
}
template <>           void writeStreamElement<QString>(QDataStream &os, QString s)
{
	os << s;
}

QDataStream& operator<<(QDataStream &os, const QPrinter &printer)
{
	writeStreamElement(os, printer.printerName         ());
	writeStreamElement(os, printer.pageSize            ());
	writeStreamElement(os, printer.collateCopies       ());
	writeStreamElement(os, printer.colorMode           ());
	writeStreamElement(os, printer.copyCount           ());
	writeStreamElement(os, printer.creator             ());
        writeStreamElement(os, printer.docName             ());
        writeStreamElement(os, printer.doubleSidedPrinting ());
        writeStreamElement(os, printer.duplex              ());
        writeStreamElement(os, printer.fontEmbeddingEnabled());
        writeStreamElement(os, printer.fullPage            ());
        writeStreamElement(os, printer.orientation         ());
        writeStreamElement(os, printer.outputFileName      ());
        writeStreamElement(os, printer.outputFormat        ());
        writeStreamElement(os, printer.pageOrder           ());
        writeStreamElement(os, printer.paperSize           ());
        writeStreamElement(os, printer.paperSource         ());
        writeStreamElement(os, printer.printProgram        ());
        writeStreamElement(os, printer.printRange          ());
        writeStreamElement(os, printer.printerName         ());
        writeStreamElement(os, printer.resolution          ());
        writeStreamElement(os, printer.winPageSize         ());

        qreal left, top, right, bottom;
        printer.getPageMargins(&left, &top, &right, &bottom, QPrinter::Millimeter);
        os << left << top << right << bottom;

        Q_ASSERT_X(os.status() == QDataStream::Ok, __FUNCTION__, QString("Stream status = %1").arg(os.status()).toStdString().c_str());
        return os;
}
///////////////////////////////////////////////////////////////////////////////
// Read all available Attributes from tream into QPrinter
///////////////////////////////////////////////////////////////////////////////


template <typename t> t readStreamElement(QDataStream &is)
{
        int i;
        is >> i;
        return static_cast<t>(i);
}
template <> QString readStreamElement<QString>(QDataStream &is)
{
        QString s;
        is >> s;
        return s;
}

QDataStream& operator>>(QDataStream &is,  QPrinter &printer)
{

        printer.setPrinterName              (readStreamElement<QString>                (is));
        printer.setPageSize                 (readStreamElement<QPrinter::PaperSize>    (is));
        printer.setCollateCopies            (readStreamElement<bool>                   (is));
        printer.setColorMode                (readStreamElement<QPrinter::ColorMode>    (is));
        printer.setCopyCount                (readStreamElement<int>                    (is));
        printer.setCreator                  (readStreamElement<QString>                (is));
        printer.setDocName                  (readStreamElement<QString>                (is));
        printer.setDoubleSidedPrinting      (readStreamElement<bool>                   (is));
        printer.setDuplex                   (readStreamElement<QPrinter::DuplexMode>   (is));
        printer.setFontEmbeddingEnabled     (readStreamElement<bool>                   (is));
        printer.setFullPage                 (readStreamElement<bool>                   (is));
        printer.setOrientation              (readStreamElement<QPrinter::Orientation>  (is));
        printer.setOutputFileName           (readStreamElement< QString >              (is));
        printer.setOutputFormat             (readStreamElement<QPrinter::OutputFormat> (is));
        printer.setPageOrder                (readStreamElement<QPrinter::PageOrder>    (is));
        printer.setPaperSize                (readStreamElement<QPrinter::PaperSize>    (is));
        printer.setPaperSource              (readStreamElement<QPrinter::PaperSource>  (is));
        printer.setPrintProgram             (readStreamElement<QString>                (is));
        printer.setPrintRange               (readStreamElement<QPrinter::PrintRange>   (is));
        printer.setPrinterName              (readStreamElement<QString>                (is));
        printer.setResolution               (readStreamElement<int>                    (is));
        printer.setWinPageSize              (readStreamElement<int>                    (is));

        qreal left, top, right, bottom;
        is >> left >> top >> right >> bottom;

        printer.setPageMargins(left, top, right, bottom, QPrinter::Millimeter);

        Q_ASSERT_X(is.status() == QDataStream::Ok, __FUNCTION__, QString("Stream status = %1").arg(is.status()).toStdString().c_str());

        return is;
}

#if 0
///////////////////////////////////////////////////////////////////////////////
// persist settings
///////////////////////////////////////////////////////////////////////////////
QPrinter *sgPrinter =...;
...
QByteArray byteArr;
QDataStream os(&byteArr, QIODevice::WriteOnly);
os << *sgPrinter;
QSettings settings("justMe", "myApp"));
settings.setValue("printerSetup", byteArr.toHex());

///////////////////////////////////////////////////////////////////////////////
// restore settings
///////////////////////////////////////////////////////////////////////////////
QByteArray printSetUp = settings.value("printerSetup").toByteArray();
printSetUp = QByteArray::fromHex(printSetUp);
QDataStream is(&printSetUp, QIODevice::ReadOnly);
is >> *sgPrinter;
#endif
