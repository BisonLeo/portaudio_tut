#include "loggerUtil.h"
#include <ctime>

#ifdef UNICODE
#include <wtypes.h>
#endif

paDemo* mwptr = NULL;


void initMwLog(paDemo* mainwindow) {
	mwptr = mainwindow;
}

void insertEndQString(const QString& s)
{
	if (mwptr != NULL) {
		QTextCursor textcursor = mwptr->ui.txtLogbook->textCursor();
		textcursor.movePosition(QTextCursor::End);
		mwptr->ui.txtLogbook->setTextCursor(textcursor);
		mwptr->ui.txtLogbook->insertPlainText(s);
		mwptr->ui.txtLogbook->update();
		textcursor.movePosition(QTextCursor::End);
		mwptr->ui.txtLogbook->setTextCursor(textcursor);
	}
}
void showTime(void) {
	static time_t lasttime = 0, nowtime = 0;
	static struct tm* timeinfo = NULL;
	static char buffer[80];
	auto currentTime = std::chrono::system_clock::now();
	auto transformed = currentTime.time_since_epoch().count() / 10000;
	int millis = transformed % 1000;

	time(&nowtime);
	if (nowtime - lasttime > 5 ) {
		timeinfo = localtime(&nowtime);

		strftime(buffer, sizeof(buffer), "\n====%d-%m-%Y %H:%M:%S.", timeinfo);
		snprintf(buffer, sizeof(buffer), "%s%03d====\n", buffer, millis);
		insertEndQString(QString(buffer));
		lasttime = nowtime;
	}
}

void writeLogA(LPCCH fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    size_t len;
    len = _vsnprintf(NULL, 0, fmt, ap);
    char* buf = new char[len + 1];
    int r = _vsnprintf(buf, len + 1, fmt, ap);
    va_end(ap);
    showTime();
    insertEndQString(QString::fromStdString(buf));
    delete[] buf;

}

void writeLogT(LPCTCH fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	size_t len;
#ifdef UNICODE
	len = _vsnwprintf(NULL, 0, fmt, ap);
#else
	len = _vsnprintf(NULL, 0, fmt, ap);
#endif
	TCHAR *buf = new TCHAR[len+1];
	int r = _vsnwprintf(buf, len+1, fmt, ap);
	va_end(ap);
	showTime();
#ifdef UNICODE
	insertEndQString(QString::fromStdWString(buf));
#else
	insertEndQString(QString::fromStdString(buf));
#endif
	delete[] buf;

}
