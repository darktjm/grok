#include <QtWidgets>
#include "grok.h"
#include "form.h"

// This is a nasty hack to allow bar chart preferences to be set in QSS
// modeled after layout-qss.h.
// It's slightly easier for me than adding them to global preferences.
// Since I have to use a custom widget for charts anyway, I went ahead and
// made this more than just a QSS provider by overriding paintEvent.

class GrokChart : public QWidget {
    Q_OBJECT
  public:
    GrokChart(QWidget *parent) : QWidget(parent) {
	// setProperty("colStd", true);
	setProperty("colBack", true);
	// in case there is no style sheet, here are the defaults
	fillcolor[0] = QColor("#306080");
	fillcolor[1] = QColor("#a04040");
	fillcolor[2] = QColor("#a08040");
	fillcolor[3] = QColor("#408030");
	fillcolor[4] = QColor("#804080");
	fillcolor[5] = QColor("#408070");
	fillcolor[6] = QColor("#a06030");
	fillcolor[7] = QColor("#f0f0f0");
    }

    // hopefully this will not be called in a weird order
    CARD *card;
    ITEM *item = NULL;
  private:
    void chart_action_callback(QMouseEvent *, int);
  public:
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *e) { chart_action_callback(e, 1); }
    void mouseReleaseEvent(QMouseEvent *e) { chart_action_callback(e, -1); }
    void mouseMoveEvent(QMouseEvent *e) { chart_action_callback(e, 0); }
  private:
    // formerly statics in chartdrw.c
    double	xmin = 0.0, xmax = 0.0;	/* bounds of all bars in chart */
    double	ymin = 0.0, ymax = 0.0;	/* bounds of all bars in chart */
    // formerly statics in chart_action_callback()
    int	nitem;				/* item on which pen was pressed */
    int	row, comp;			/* row and column of dragged bar */
    int	down_x, down_y;			/* pos where pen was pressed down */
    double	x_val, y_val;		/* previous values of fields */
    bool	moving = false;		/* this is not a selection, move box */
    // functions needing xmin/xmax/ymin/ymax
    int pick_chart(
	CARD		*card,		/* life, the universe, and everything*/
	int		nitem,		/* # of item in form */
	int		*comp,		/* returned component # */
	int		xpick,		/* mouxe x/y pixel coordinate */
	int		ypick);


    // The foregound color of the widget is what's used for the outer border
    // on non-fat or selected bars.  Formerly colStd, so this means the widget
    // needs the colStd property set to true by default (above).
    // On the other hand, maybe use COL_CHART_BOX, which is the documented
    // color to use.
    // const QColor &fgcolor() { return palette().color(foregroundRole()); }
    // const QBrush &fgbrush() { return palette().brush(foregroundRole()); }

    // The background color of the widget is what's used for the chart
    // background color.  Formerly colBack, so this means the widget needs
    // the colBack property set to true by default (above).  This leads to
    // a conflict if people wet foreground and background color at the same
    // time in either colStd or colBack.  For now, this conflict is avoided
    // by ignoring the foreground color in favor of the box color.
    const QColor &bgcolor() { return palette().color(backgroundRole()); }
    const QBrush &bgbrush() { return palette().brush(backgroundRole()); }

    // The selection color of the widget cannot be COL_SHEET, because it
    // conflicts with the above two settings.  Instead, a new property
    // is added:
    QColor hlcolor = QColor("#f0f0f0");
  private:
    Q_PROPERTY(QColor hlcolor MEMBER hlcolor)
  public:
    // COL_CHART_AXIS
    QColor axiscolor = QColor("#101010");
  private:
    Q_PROPERTY(QColor axiscolor MEMBER axiscolor)
  public:
    // COL_CHART_GRID
    QColor gridcolor = QColor("#101010");
  private:
    Q_PROPERTY(QColor gridcolor MEMBER gridcolor)
  public:
    // COL_CHART_BOX
    QColor boxcolor = QColor("#000000");
  private:
    Q_PROPERTY(QColor boxcolor MEMBER boxcolor)
  public:
    // COL_CHART_0 .. 7
    QColor fillcolor[COL_CHART_N];
#if COL_CHART_N != 8 // better be 8, because that's all I support
#error Fix chart_qss.h
#endif
    // Qt doesn't support MEMBER fillcolor[N], so I have to use setters/getters
    // And, since moc doesn't preprocess, I have to do it manually -- ugh
  private:
    Q_PROPERTY(QColor color0 READ color0 WRITE setColor0)
    const QColor color0() { return fillcolor[0]; }
    void setColor0(QColor c) { fillcolor[0] = c; }
    Q_PROPERTY(QColor color1 READ color1 WRITE setColor1)
    const QColor color1() { return fillcolor[1]; }
    void setColor1(QColor c) { fillcolor[1] = c; }
    Q_PROPERTY(QColor color2 READ color2 WRITE setColor2)
    const QColor color2() { return fillcolor[2]; }
    void setColor2(QColor c) { fillcolor[2] = c; }
    Q_PROPERTY(QColor color3 READ color3 WRITE setColor3)
    const QColor color3() { return fillcolor[3]; }
    void setColor3(QColor c) { fillcolor[3] = c; }
    Q_PROPERTY(QColor color4 READ color4 WRITE setColor4)
    const QColor color4() { return fillcolor[4]; }
    void setColor4(QColor c) { fillcolor[4] = c; }
    Q_PROPERTY(QColor color5 READ color5 WRITE setColor5)
    const QColor color5() { return fillcolor[5]; }
    void setColor5(QColor c) { fillcolor[5] = c; }
    Q_PROPERTY(QColor color6 READ color6 WRITE setColor6)
    const QColor color6() { return fillcolor[6]; }
    void setColor6(QColor c) { fillcolor[6] = c; }
    Q_PROPERTY(QColor color7 READ color7 WRITE setColor7)
    const QColor color7() { return fillcolor[7]; }
    void setColor7(QColor c) { fillcolor[7] = c; }
};
