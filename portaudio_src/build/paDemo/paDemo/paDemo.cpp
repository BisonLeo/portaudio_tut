#include "paDemo.h"
#include "loggerUtil.h"

extern int devListMain();
paDemo::paDemo(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    initMwLog(this);
}

void paDemo::on_btnStart_clicked()
{
    // must add /utf-8 compiling option in msvc as this file is saved using encoding of 'utf-8'
    // such as writeLog(L"%s", L"hihi我\nhihi我\nhihi我\nhihi我\nhihi我\nhihi我\nhihi我\nhihi我\nhihi我\nhihi我\nhihi我\n");
    devListMain();
}
