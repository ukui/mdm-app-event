#include <QApplication>

#include "mdm_app_event.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MdmAppEvent event;

    return a.exec();
}
