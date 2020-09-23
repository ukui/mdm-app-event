#include <unistd.h>
#include <QApplication>

#include "mdm_app_event.h"

int main(int argc, char *argv[])
{
    if (setuid(0) != 0)
        qDebug() << "enhance permission error";
    QApplication a(argc, argv);
    // a.setSetuidAllowed(true);
    MdmAppEvent event;

    return a.exec();
}
