#include "paDemo.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    paDemo w;
    w.show();
    return a.exec();
}
