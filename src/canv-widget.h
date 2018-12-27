#include <QtWidgets>
#include "grok.h"
#include "form.h"

// This is a nasty hack to allow canvas preferences to be set in QSS
// modeled after layout-qss.h.
// It's slightly easier for me than adding them to global preferences.
// Since I have to use a custom widget for the canvas anyway, I went ahead and
// made this more than just a QSS provider by overriding paintEvent.

/*
 * mouse position classifications (from canvdraw.c)
 */

typedef enum {
	M_OUTSIDE = 0,		/* not near any item */
	M_INSIDE,		/* inside an item, but not near an edge */
	M_DIVIDER,		/* on grip between static part and card */
	M_TOP,			/* near top edge */
	M_BOTTOM,		/* near bottom edge */
	M_LEFT,			/* near left edge */
	M_RIGHT,		/* near right edge */
	M_XMID,			/* near X divider in an input/date/time item */
	M_YMID			/* near Y divider in a note/view item */
} MOUSE;


class GrokCanvas : public QDialog {
    Q_OBJECT
  public:
    GrokCanvas();

  private:
    QWidget *font[F_NFONTS];
    void canvas_callback(QMouseEvent *, int);
  public:
    // formerly expose_callback +
    void paintEvent(QPaintEvent *);
    // needs too much from here
    void redraw_canvas_item(QPainter &painter, const QRect &clip, ITEM *item);
    void mousePressEvent(QMouseEvent *e) { canvas_callback(e, 1); }
    void mouseReleaseEvent(QMouseEvent *e) { canvas_callback(e, -1); }
    void mouseMoveEvent(QMouseEvent *e) { canvas_callback(e, 0); }
    void closeEvent(QCloseEvent *e);
    void resizeEvent(QResizeEvent *);
    QRubberBand *rb = 0; // current rubber band
  public:
    void draw_rubberband(
	bool		draw,		/* draw or undraw */
	int		x,		/* position of box */
	int		y,
	int		xs,		/* size of box */
	int		ys,
	bool		isrect = true); /* is it a rectngle or line? */
  private:
    // formerly statics in canvas_callback()
    int		nitem;		/* item on which pen was pressed */
    MOUSE	mode;		/* what's being moved: M_* */
    int		down_x, down_y;	/* pos where pen was pressed down */
    int		state;		/* button/modkey mask when pressed */
    bool	moving;		/* this is not a selection, move box */

    // The foregound color of the widget is what's used for widget frames
    // and dividers.  Formerly colCanvFrame, but I decided to merge it
    // with colCanvBack into a single colCanv.  So, set colCanv property
    // to true in constructor.
    const QColor &fgcolor() { return palette().color(foregroundRole()); }
    // const QBrush &fgbrush() { return palette().brush(foregroundRole()); }

    // The background color of the widget is what's used for the main
    // background color.  Formerly colCanvBack, but as mentioned above,
    // I merged it with colCanvFrame into colCanv.  So, set colCanv
    // property to true in constructor.
    const QColor &bgcolor() { return palette().color(backgroundRole()); }
    // const QBrush &bgbrush() { return palette().brush(backgroundRole()); }

    // COL_CANVBOX
    QColor boxcolor = QColor("#808090");
  private:
    Q_PROPERTY(QColor boxcolor MEMBER boxcolor)
  public:
    // COL_CANVSEL
    QColor selcolor = QColor("#e0e060");
  private:
    Q_PROPERTY(QColor selcolor MEMBER selcolor)
  public:
    // COL_CANVTEXT
    QColor textcolor = QColor("#101010");
  private:
    Q_PROPERTY(QColor textcolor MEMBER textcolor)
  public:
    int curr_item = 0;
};
