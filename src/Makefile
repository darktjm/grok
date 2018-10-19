#
# grok Makefile. Use this by running "make -f Makefile" if you don't have
# xmkmf and/or imake. Otherwise, run "xmkmf" and "make".
#

GBIN = /usr/local/bin
GLIB = /usr/local/lib

#DEBUG = -g
DEBUG = -O2 -s

SGI5_C	= -DSYSV -DSVR4 -wlint -woff 813,827,852,819,826,822
SGI6_C	= -DSYSV -DSVR4 -woff 1185 -n32
LINUX_C	= -I/usr/X11/include
HP_C	= -Aa -I/usr/include/Motif1.2 -I/usr/include/X11R5 -I/usr/include\
	  -D_INCLUDE_HPUX_SOURCE -D_INCLUDE_POSIX_SOURCE -D_INCLUDE_XOPEN_SOURCE
IBM_C	= -I/usr/include/X11 -D_POSIX_SOURCE -D_XOPEN_SOURCE -D_ANSI_C_SOURCE
OSF_C	=
SUN_C	=
SUNG_C	= -ansi -I/usr/5include -I/usr/include/X11
SOL_C	= -I/usr/local/X11R5/include -I/usr/local/X11R5/include/X11
ULTRIX_C=
CDC_C	=
MIPS_C	= -systype sysv -Wf,-XNd4000 -DPID_T="int" -I/usr
DG_C	=
SNI_C	= -DSYSV
SCO_C	=
NCR_C	=
USL_C	= -I/usr/X/include
BSDI_C	= -I/usr/X11/include
BSDIG_C	= -I/usr/X11/include -ansi



#
# loader flags. Requires X11, Xt, and Motif libraries. PW is for regcmp/regex.
#
SGI5_L	= -lXm_s -lXt_s -lX11_s -lPW -lm
SGI6_L	= -lXm -lXt -lX11 -lPW -lm
LINUX_L	= -L/usr/X11R6/lib -lXm -lXt -lX11 -lm
HP_L	= -L/usr/lib/X11R5 -L/usr/lib/Motif1.2 -lXm -lXt -lX11 -lc -lm
IBM_L	= -lXm -lXt -lX11 -lPW -lm
OSF_L	= -lXm -lXt -lX11 -lm
SUN_L	= -lXm -lXt -lX11 -lm
SUNG_L	= -L/usr/5lib -lXm -lXt -lX11 -lm
SOL_L	= -L/usr/local/X11R5/lib -lXm -lXt -lX11 -lelf -lgen -lm
ULTRIX_L= -lXm -lXt -lX11 -lm
CDC_L	= -lXm -lXt -lX11 -lgen -lnsl -lsocket -lm
MIPS_L	= -lXm -lXt -lX11 -lbsd -lsun -lc -lPW -lm
DG_L	= -lXm -lXt -lX11 -lPW -lm
SNI_L	= -lXm -lXt -lX11 -lsocket -lnsl -lXmu -lm
SCO_L	= -lXm -lXt -lXmu -lX11 -lPW -lm -lsocket
NCR_L	= -lXm -lXt -lX11 -lXext -lnsl -lgen -lm
USL_L	= -B static -lXm -B dynamic -lXaw -lXext -lXmu -lXt -lX11 -lm -lnsl -lgen -lm
USL_Lm	= -lXm -lXaw -lXext -lXmu -lXt -lX11 -lm -lnsl -lgen -lm
BSDI_L	= -lXm -lXt -lX11 -lm
BSDIG_L	= -L/usr/X11/lib -lXm -lXt -lX11 -lm



#---------------------------------------------------------------
# available options from the list at the top of this Makefile, add an echo
# here, and add a target in the next section. Please mail patches to me.

help:
	@echo "Usage:"
	@echo "  make -f Makefile SYS" 
	@echo ""
	@echo "SYS is one of the following:"
	@echo "  sgi5      All 4.x and 5.x SGI systems"
	@echo "  sgi       All 6.x SGI systems"
	@echo "  linux     Linux x86"
	@echo "  hp        HP/UX 9.x with X11R5 and Motif 1.2"
	@echo "  ibm       IBM RS6000 running AIX"
	@echo "  osf       DEC running OSF/1"
	@echo "  sun       Sun with Motif (won't work with OpenLook)"
	@echo "  sung      Sun 4.1.3/Solaris 2.2 with Motif, using gcc"
	@echo "  sol       Solaris 2.2 with Motif, using cc"
	@echo "  ultrix    Ultrix 4.3 with Motif 1.1.3"
	@echo "  cdc       CDC/Mips running EP/IX 2.1.1"
	@echo "  mips      Mips Magnum RISCos 4.52 with RISCwindows 4.0"
	@echo "  dg        Data General DG/UX, with gcc"
	@echo "  sni       SNI SINIX 5.41"
	@echo "  sco       SCO Open Desktop/Open Server 3.0"
	@echo "  ncr       NCR 3000 systems"
	@echo "  usl       USL SVR4.2 V1 with mdev package"
	@echo "  uslm      ...same with Motif run-time libraries"
	@echo "  bsdi      BSDI/386"
	@echo "  bsdig     BSDI/386 with gcc"
	@echo ""
	@echo "  make install   install compiled programs in"
	@echo "                 $(GBIN) and $(GLIB)."
	@echo "  make clean     remove everything in this dir except sources"
	@echo ""
	@echo "If you have problems, first read this Makefile. You may also"
	@echo "want to change the installation directory if you are not root."
	@echo "The installation directory is set at the top of this Makefile."
	@echo ""
	@echo "NOTE: GROK IS CURRENTLY SUPPORTED ON SGI, LINUX, HP, and IBM."
	@echo "All the other ports are best guesses. Read and edit config.h."


#---------------------------------------------------------------
sgi5:
	env CF="$(SGI5_C)" LF="$(SGI5_L)" YO="" MYCC=cc make -f Makefile all

sgi6:
	env CF="$(SGI6_C)" LF="$(SGI6_L)" YO="" MYCC=cc make -f Makefile all

sgi:
	env CF="$(SGI6_C)" LF="$(SGI6_L)" YO="" MYCC=cc make -f Makefile all

linux:
	env CF="$(LINUX_C)" LF="$(LINUX_L)" YO="" MYCC=gcc make -f Makefile all

hp:
	env CF="$(HP_C)" LF="$(HP_L)" YO="-Nm15000" MYCC=cc make -f Makefile all

ibm:
	env CF="$(IBM_C)" LF="$(IBM_L)" YO="" MYCC=cc make -f Makefile all

osf:
	env CF="$(OSF_C)" LF="$(OSF_L)" YO="" MYCC=cc make -f Makefile all

sun:
	env CF="$(SUN_C)" LF="$(SUN_L)" YO="" MYCC=acc make -f Makefile all

sung:
	env CF="$(SUNG_C)" LF="$(SUNG_L)" YO="" MYCC=gcc make -f Makefile all

sol:
	env CF="$(SOL_C)" LF="$(SOL_L)" YO="" MYCC=cc make -f Makefile all

ultrix:
	env CF="$(ULTRIX_C)" LF="$(ULTRIX_L)" YO="" MYCC=cc make -f Makefile all

cdc:
	env CF="$(CDC_C)" LF="$(CDC_L)" YO="" MYCC=cc make -f Makefile all

mips:
	env CF="$(MIPS_C)" LF="$(MIPS_L)" YO="" MYCC=cc make -f Makefile all

dg:
	env CF="$(DG_C)" LF="$(DG_L)" YO="" MYCC=gcc make -f Makefile all

sni:
	env CF="$(SNI_C)" LF="$(SNI_L)" YO="" MYCC=cc make -f Makefile all

sco:
	env CF="$(SCO_C)" LF="$(SCO_L)" YO="" MYCC=cc make -f Makefile all

ncr:
	env CF="$(NCR_C)" LF="$(NCR_L)" YO="" MYCC=cc make -f Makefile all

usl:
	env CF="$(USL_C)" LF="$(USL_L)" YO="" MYCC=cc make -f Makefile all

uslm:
	env CF="$(USL_C)" LF="$(USL_Lm)" YO="" MYCC=cc make -f Makefile all

bsdi:
	env CF="$(BSDI_C)" LF="$(BSDI_L)" YO="" MYCC=acc make -f Makefile all

bsdig:
	env CF="$(BSDIG_C)" LF="$(BSDIG_L)" YO="" MYCC=gcc make -f Makefile all



#------------------------------ don't change anything below ------------------

INC	= config.h form.h grok.h proto.h version.h resource.h\
	  bm_icon.h bm_left.h bm_right.h

SRC	= main.c mainwin.c formwin.c formop.c canvdraw.c cardwin.c \
	  formfile.c dbase.c dbfile.c popup.c util.c help.c eval.c \
	  evalfunc.c query.c querywin.c sumwin.c parser.y prefwin.c\
	  printwin.c print.c convert.c chart.c chartdrw.c editwin.c\
	  sectwin.c template.c templmk.c templwin.c
OBJ	= main.o mainwin.o formwin.o formop.o canvdraw.o cardwin.o \
	  formfile.o dbase.o dbfile.o popup.o util.o help.o eval.o \
	  evalfunc.o query.o querywin.o sumwin.o y.tab.o prefwin.o \
	  printwin.o print.o convert.o chart.o chartdrw.o editwin.o\
	  sectwin.o template.o templmk.o templwin.o

DOC	= doc/export.html doc/files.html doc/formedit.html \
	  doc/grammar.html doc/index.html doc/intro.html

DEMO	= demo/countries.db demo/countries.gf \
	  demo/phone.db demo/phone.gf \
	  demo/phonelog.db demo/phonelog.gf \
	  demo/todo.db demo/todo.gf \
	  demo/diary.db demo/diary.gf \
	  demo/passwd.gf \
	  demo/procdemo demo/procdemo.gf \
	  demo/weather.db demo/weather.gf \
	  demo/checkbook.db demo/checkbook.gf\
	  demo/www_pages.db demo/www_pages.gf\
	  demo/biblio.db demo/biblio.gf demo/biblio.mktex.pl \
	  demo/systems.gf\
	  demo/workplan.gf


all:	grok

grok:	$(OBJ)
	$(MYCC) $(DEBUG) $(CF) $(INCDIR) -o grok $(OBJ) $(LF)

.c.o:
	$(MYCC) $(DEBUG) $(CF) $(INCDIR) -c $<

y.tab.c: parser.y
	yacc -d $(YO) parser.y

mainwin.o: mainwin.c
	$(MYCC) $(DEBUG) $(CF) $(INCDIR) -c -DLIB=\"$(GLIB)\" $<

util.o: util.c
	$(MYCC) $(DEBUG) $(CF) $(INCDIR) -c -DPATH=\"$(GBIN):$(GLIB)\" $<

$(OBJ): grok.h proto.h form.h
popup.o: version.h
main.o:	resource.h
util.o:	bm_icon.h
evalfunc.o: y.tab.h
y.tab.h eval.o: y.tab.c

#---------------------------------------------------------------
install:
	-test -d $(GLIB)/demo || mkdir -p $(GLIB)/demo
	cp grok $(GBIN)
	cp ../misc/grok.hlp $(GLIB)
	cp ../demo/* ../doc/* $(GLIB)/grokdir
	-strip $(GBIN)/grok
	-chmod 755 $(GBIN)/grok
	-chmod 644 $(GLIB)/grok.hlp
	-test -d ~/.grok || (mkdir $$HOME/.grok; cp ../demo/?* $$HOME/.grok)

clean:
	-rm -f *.o y.tab.h y.tab.c lex.yy.c core grok tags y.output *.shar*

VERS	= `egrep VERSION grok/src/version.h | sed 's/.*"V//;s/".*//'`

tar:
	cd ../..; rm -rf groktmp xmbase_grok-$(VERS)
	mkdir -p ../../groktmp/src
	cd ..; cp -rp README HISTORY demo doc man misc ../groktmp
	cp -rp Makefile Makefile.auto $(SRC) $(INC) ../../groktmp/src
	cd ../..; mv groktmp xmbase_grok-$(VERS)
	cd ../..; tar cf - xmbase_grok-$(VERS) | gzip > grok/xmbase_grok-$(VERS).tar.gz
