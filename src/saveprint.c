// copied from https://stackoverflow.com/questions/12253363/persisting-serializing-qprinter-qprintdialog-between-execution
/* The only changes I made were reindenting and #if-ing out the samples */
/* Now I've also added some headers */
#include "config.h"
#include <QtWidgets>
#include <QtPrintSupport>
#include "grok.h"
#include "form.h"
#include "proto.h"
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
	writeStreamElement(os, printer.pageLayout().pageSize().id());
	writeStreamElement(os, printer.collateCopies       ());
	writeStreamElement(os, printer.colorMode           ());
	writeStreamElement(os, printer.copyCount           ());
	writeStreamElement(os, printer.creator             ());
        writeStreamElement(os, printer.docName             ());
        writeStreamElement(os, 0 /*printer.doubleSidedPrinting ()*/);
        writeStreamElement(os, printer.duplex              ());
        writeStreamElement(os, printer.fontEmbeddingEnabled());
        writeStreamElement(os, printer.fullPage            ());
        writeStreamElement(os, printer.pageLayout().orientation());
        writeStreamElement(os, printer.outputFileName      ());
        writeStreamElement(os, printer.outputFormat        ());
        writeStreamElement(os, printer.pageOrder           ());
        writeStreamElement(os, printer.pageLayout().pageSize().id());
        writeStreamElement(os, printer.paperSource         ());
        writeStreamElement(os, printer.printProgram        ());
        writeStreamElement(os, printer.printRange          ());
        writeStreamElement(os, printer.printerName         ());
        writeStreamElement(os, printer.resolution          ());
        writeStreamElement(os, printer.pageLayout().pageSize().windowsId());

#if 0
        qreal left, top, right, bottom;
        printer.getMargins(&left, &top, &right, &bottom, QPrinter::Millimeter);
        os << left << top << right << bottom;
#else
        QMarginsF m = printer.pageLayout().margins(QPageLayout::Unit::Millimeter);
        os << m.left() << m.top() << m.right() << m.bottom();
#endif

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
    printer.setPageSize(QPageSize                 (readStreamElement<QPageSize::PageSizeId>              (is)));
        printer.setCollateCopies            (readStreamElement<bool>                   (is));
        printer.setColorMode                (readStreamElement<QPrinter::ColorMode>    (is));
        printer.setCopyCount                (readStreamElement<int>                    (is));
        printer.setCreator                  (readStreamElement<QString>                (is));
        printer.setDocName                  (readStreamElement<QString>                (is));
        /*printer.setDoubleSidedPrinting      (*/readStreamElement<bool>               (is)/*)*/;
        printer.setDuplex                   (readStreamElement<QPrinter::DuplexMode>   (is));
        printer.setFontEmbeddingEnabled     (readStreamElement<bool>                   (is));
        printer.setFullPage                 (readStreamElement<bool>                   (is));
        printer.setPageOrientation          (readStreamElement<QPageLayout::Orientation>  (is));
        printer.setOutputFileName           (readStreamElement< QString >              (is));
        printer.setOutputFormat             (readStreamElement<QPrinter::OutputFormat> (is));
        printer.setPageOrder                (readStreamElement<QPrinter::PageOrder>    (is));
        printer.setPageSize(QPageSize       (readStreamElement<QPageSize::PageSizeId>  (is)));
        printer.setPaperSource              (readStreamElement<QPrinter::PaperSource>  (is));
        printer.setPrintProgram             (readStreamElement<QString>                (is));
        printer.setPrintRange               (readStreamElement<QPrinter::PrintRange>   (is));
        printer.setPrinterName              (readStreamElement<QString>                (is));
        printer.setResolution               (readStreamElement<int>                    (is));
        printer.setPageSize(QPageSize(QPageSize::id (readStreamElement<int>          (is))));

        qreal left, top, right, bottom;
        is >> left >> top >> right >> bottom;

    printer.setPageMargins(QMarginsF(left, top, right, bottom), QPageLayout::Unit::Millimeter);

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
