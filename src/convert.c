/*
 * Convert date and time data formats.
 *
 *	mkdatestring(time)		Convert # of seconds since 1/1/70
 *					to a date string
 *	mktimestring(time, dur)		Convert # of seconds since 1/1/70
 *					to a time-of-day or duration string
 *	parse_datestring(text)		Interpret the date string, and
 *					return the # of seconds since 1/1/70
 *	parse_timestring(text)		Interpret the time string, and
 *					return the # of seconds since 1/1/70
 */


#include "config.h"
#include <X11/Xos.h>
#include <stdio.h>
#include <time.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include "grok.h"
#include "form.h"
#include "proto.h"

char *weekday_name[7] =	    { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
char *alt_weekday_name[7] = { "Mo",  "Di",  "Mi",  "Do",  "Fr",  "Sa",  "So" };


/*
 * return some sensible string representation of a date (no time). Time 0
 * is not represented as 1.1.1970 or 31.12.1969 but as an empty string,
 * because if the user leaves a time input field empty it should stay empty.
 * I hereby stipulate that nothing of significance happened on 1.1.1970.
 */

char *mkdatestring(
	time_t			time)		/* date in seconds */
{
	static char		buf[40];	/* string representation */
	struct tm		*tm;		/* time to m/d/y conv */

	if (!time)
		return("");
	tm = localtime(&time);
	if (pref.mmddyy)
		sprintf(buf, "%2d/%02d/%02d", tm->tm_mon+1, tm->tm_mday,
					      tm->tm_year % 100);
	else
		sprintf(buf, "%2d.%02d.%02d", tm->tm_mday, tm->tm_mon+1,
					      tm->tm_year % 100);
	return(buf);
}


/*
 * return some sensible string representation of a time (no date)
 */

char *mktimestring(
	time_t			time,		/* date in seconds */
	BOOL			dur)		/* duration, not time-of-day */
{
	static char		buf[40];	/* string representation */
	struct tm		*tm, tmbuf;	/* time to m/d/y conv */

	if (dur) {
		tm = &tmbuf;
		tm->tm_hour =  time / 3600;
		tm->tm_min  = (time / 60) % 60;
	} else
		tm = localtime(&time);
	if (pref.ampm && !dur)
		sprintf(buf, "%2d:%02d%c", tm->tm_hour%12 ? tm->tm_hour%12 :12,
					   tm->tm_min,
					   tm->tm_hour < 12 ? 'a' : 'p');
	else
		sprintf(buf, "%02d:%02d",  tm->tm_hour, tm->tm_min);
	return(buf);
}


/*
 * parse the date string, and return the number of seconds. The default
 * time argument is for the +x notation, it's typically the trigger date
 * of the appointment. Use 0 if it's not available; today is default.
 * Doesn't know about alpha month names yet. The time on the recognized
 * date is set to 0:00.
 */

time_t parse_datestring(
	char		*text)		/* input string */
{
	time_t		today;		/* current date in seconds */
	struct tm	*tm;		/* today's date */
	long		num[3];		/* m,d,y or d,m,y */
	int		nnum;		/* how many numbers in text */
	long		i;		/* tmp counter */
	char		*p;		/* text scan pointer */
	char		buf[10];	/* lowercase weekday name */
	BOOL		mmddyy = pref.mmddyy;

	today = time(0);				/* today's date */
	tm = localtime(&today);
	tm->tm_hour = tm->tm_min = tm->tm_sec = 0;
	tm->tm_hour = 12;
	while (*text == ' ' || *text == '\t')		/* skip blanks */
		text++;
	for (p=text; *p; p++)				/* -> lowercase */
		if (*p >= 'A' && *p <= 'Z')
			*p += 'a' - 'A';
							/* today, tomorrow? */
	if (!strncmp(text, "tod", 3) ||
	    !strncmp(text, "heu", 3) ||
	    !strncmp(text, "auj", 3) || !*text)
		return(today);

	if (!strncmp(text, "tom", 3) ||
	    !strncmp(text, "mor", 3) ||
	    !strncmp(text, "dem", 3))
		return(today + 86400);

	if (!strncmp(text, "yes", 3) ||
	    !strncmp(text, "ges", 3) ||
	    !strncmp(text, "hie", 3))
		return(today - 86400);

	if (!strncmp(text, "ueb", 3))
		return(today + 2*86400);
							/* weekday name? */
	for (i=0; i < 7; i++) {
		strcpy(buf, weekday_name[i]);
		*buf += 'a' - 'A';
		if (!strncmp(buf, text, strlen(buf)))
			break;
		strcpy(buf, alt_weekday_name[i]);
		*buf += 'a' - 'A';
		if (!strncmp(buf, text, strlen(buf)))
			break;
	}
	if (i < 7) {
		i = (i - tm->tm_wday + 8) % 7;
		return(today + i*86400);
	}
							/* d/m/y numbers? */
	num[0] = num[1] = num[2] = 0;
	p = text;
	for (nnum=0; nnum < 3; nnum++) {
		if (!*p)
			break;
		while (*p >= '0' && *p <= '9')
			num[nnum] = num[nnum]*10 + *p++ - '0';
		while (*p && !(*p >= '0' && *p <= '9')) {
			if (*p == '.')
				mmddyy = FALSE;
			if (*p == '/')
				mmddyy = TRUE;
			p++;
		}
	}
	if (nnum == 0)					/* ... no numbers */
		return(today);
	if (nnum == 3) {				/* ... have year? */
		if (num[2] < 70)
			num[2] += 100;
		if (num[2] > 1900)
			num[2] -= 1900;
		if (num[2] < 70 || num[2] > 138)
			num[2]  = tm->tm_year;
		tm->tm_year = num[2];
	}
	if (nnum == 1) {				/* ... day only */
		if (num[0] < tm->tm_mday)
			if (++tm->tm_mon == 12) {
				tm->tm_mon = 0;
				tm->tm_year++;
			}
		tm->tm_mday = num[0];
	} else {					/* ... d.m or m/d */
		if (mmddyy) {
			i      = num[0];
			num[0] = num[1];
			num[1] = i;
		}
		if (nnum < 3 && num[1]*100+num[0] <
						(tm->tm_mon+1)*100+tm->tm_mday)
			tm->tm_year++;
		tm->tm_mday = num[0];
		tm->tm_mon  = num[1]-1;
	}
	return(mktime(tm));
}


/*
 * parse the time string, and return the number of seconds since midnight.
 * The time returned is in Unix format, in seconds since 1/1/1970. In
 * particular, midnight is not necessarily 0, depending on the time zone.
 */

time_t parse_timestring(
	char			*text,		/* input string */
	BOOL			dur)		/* duration, not time-of-day */
{
	time_t			today;		/* current date in seconds */
	struct tm		*tm;		/* m/d/y to time conv */
	long			num[3];		/* h,m,s */
	int			nnum;		/* how many numbers in text */
	int			ndigits;	/* digit counter */
	char			*p = text;	/* text pointer */
	int			i;		/* text index, backwards*/
	char			ampm = 0;	/* 0, 'a', or 'p' */
	int			h, m, s;	/* hours, minutes, seconds */

	while (*p == ' ' || *p == '\t')
		p++;
	i = strlen(p)-1;
	while (i && (p[i] == ' ' || p[i] == '\t'))
		i--;
	if (i && p[i] == 'm')
		i--;
	if (i && (p[i] == 'a' || p[i] == 'p'))
		ampm = p[i--];
	while (i && (p[i] == ' ' || p[i] == '\t'))
		i--;
	num[0] = num[1] = num[2] = 0;
	for (nnum=0; i >= 0 && nnum < 3; nnum++) {
		ndigits = 0;
		while (i >= 0 && (p[i] < '0' || p[i] > '9'))
			i--;
		while (i >= 0 && p[i] >= '0' && p[i] <= '9' && ndigits++ < 2)
			num[nnum] += (p[i--] - '0') * (ndigits==1 ? 1 : 10);
	}
	if (ampm && nnum == 1) {
		nnum = 2;
		num[1] = num[0];
		num[0] = 0;
	}
	switch(nnum) {
	  case 0:	h = 0;		m = 0;		s = 0;		break;
	  case 1:	h = 0;		m = num[0];	s = 0;		break;
	  case 2:	h = num[1];	m = num[0];	s = 0;		break;
	  case 3:	h = num[2];	m = num[1];	s = num[0];	break;
	}
	if (pref.ampm) {
		if (ampm == 'a' && h == 12)
			h = 0;
		if (ampm == 'p' && h < 12)
			h += 12;
	}
	today = time(0);
	tm = localtime(&today);
	tm->tm_hour = h;
	tm->tm_min  = m;
	tm->tm_sec  = s;
	return(dur ? 3600*h + 60*m + s : mktime(tm));
}


/*
 * parse string with both date and time
 */

time_t parse_datetimestring(
	char			*text)		/* input string */
{
	struct tm	tm1, tm2;	/* for adding date and time */
	time_t		t;		/* parsed time */

	while (*text == ' ' || *text == '\t')
		text++;
	t = parse_datestring(text);
	tm1 = *localtime(&t);
	while (*text && *text != ' ' && *text != '\t')
		text++;
	while (*text == ' ' || *text == '\t')
		text++;
	t = parse_timestring(text, FALSE);
	tm2 = *localtime(&t);
	tm1.tm_hour = tm2.tm_hour;
	tm1.tm_min  = tm2.tm_min;
	tm1.tm_sec  = tm2.tm_sec;
	return(mktime(&tm1));
}
