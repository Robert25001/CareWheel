#include "Carewheel.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Carewheel w;
    w.show();
    return a.exec();
}
