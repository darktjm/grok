#include <QtWidgets>

// This is a nasty hack required to overcome Qt's inability to use qss to
// adjust layout margins.  I also support spacing and size constraint mode.
// There is a 7-year-old (at the time of this writing) bug to address this:
//   https://bugreports.qt.io/browse/QTBUG-22862
// This solution is mostly from the Qt forum:
//   https://forum.qt.io/topic/32043
// or, if you prefer,
//   https://forum.qt.io/topic/32043/qlayout-margin-through-stylesheet-solved/

class LayoutQSS : public QWidget {
    Q_OBJECT
    // FIXME: it would be nice if the "standard" qss properties could be
    //        used here, instead of qproperty-<name>.
    //        Maybe make this a QFrame derivative, instead, but then
    //        I'd have to figure out how to detect margin changes and
    //        pass them to the layout.  If I only knew when QSS styles
    //        were applied, I could do it right after.
    // css-style margins can't be easily supported
    // so use individual int properties
    Q_PROPERTY(int margins READ getMargins WRITE setMargins)
    Q_PROPERTY(int left_margin READ getLeft WRITE setLeft)
    Q_PROPERTY(int right_margin READ getRight WRITE setRight)
    Q_PROPERTY(int top_margin READ getTop WRITE setTop)
    Q_PROPERTY(int bottom_margin READ getBottom WRITE setBottom)
    Q_PROPERTY(int spacing READ spacing WRITE setSpacing)
    Q_PROPERTY(int hspacing READ hspacing WRITE setHSpacing)
    Q_PROPERTY(int vspacing READ vspacing WRITE setVSpacing)
    Q_PROPERTY(int constraint READ constraint WRITE setConstraint)
    QMargins margins;
    QLayout *l;
    QGridLayout *gl;
    QFormLayout *fl;
    void set_margins() { if(l) l->setContentsMargins(margins); }
    void get_margins() { if(l) margins = l->contentsMargins(); }
  public:
    // init: set the layout pointers and remove widget from layout/display
    //       also, set the object name to the layout's name
    // Note that I could try automatically detecting this widget's
    // immediate layout, but I can only find its outer-most one as far
    // as I can tell (parentWidget()->layout()).  I guess I could scan
    // the whole layout's item lists recursively, but why bother?
    LayoutQSS(QLayout *nl) : l(nl), gl(dynamic_cast<QGridLayout *>(nl)),
                                fl(dynamic_cast<QFormLayout *>(nl)) {
	hide();
	// hide() should be enough.
	// setEnabled(false);
	// this is unnecessary, and triggers a parent resize for some reason
	// even though I didn't set parent...
	// resize(0, 0);
	// setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	if(l)
		setObjectName(l->objectName());
    }

    // property reader/writers
    int getMargins() {
	get_margins();
	return (margins.left()+margins.right()+margins.top()+margins.bottom())/4;
    }
    void setMargins(int m) { margins = QMargins(m, m, m, m); set_margins(); }
    int getLeft() { get_margins(); return margins.left(); }
    void setLeft(int m) { get_margins(); margins.setLeft(m); set_margins(); }
    int getRight() { get_margins(); return margins.right(); }
    void setRight(int m) { get_margins(); margins.setRight(m); set_margins(); }
    int getTop() { get_margins(); return margins.top(); }
    void setTop(int m) { get_margins(); margins.setTop(m); set_margins(); }
    int getBottom() { get_margins(); return margins.bottom(); }
    void setBottom(int m) { get_margins(); margins.setBottom(m); set_margins(); }
    int spacing() { return l ? l->spacing() : -1; }
    void setSpacing(int s) { if(l) l->setSpacing(s); }
    int hspacing() { return gl ? gl->horizontalSpacing() : fl ? fl->horizontalSpacing() : -1; }
    void setHSpacing(int s) { if(gl) gl->setHorizontalSpacing(s);
                              else if(fl) fl->setHorizontalSpacing(s); }
    int vspacing() { return gl ? gl->verticalSpacing() : fl ? fl->verticalSpacing() : -1; }
    void setVSpacing(int s) { if(gl) gl->setVerticalSpacing(s);
                              else if(fl) fl->setHorizontalSpacing(s); }
    int constraint() { return l ? l->sizeConstraint() : 0; }
    void setConstraint(int c) { if(l) l->setSizeConstraint((QLayout::SizeConstraint)c); }
};
