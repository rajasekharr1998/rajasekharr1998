#ifdef J2534_ENABLE_QT

#include "ui/QtMainWindow.h"

#include <QApplication>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    ui::QtMainWindow window;
    window.resize(900, 600);
    window.show();
    return app.exec();
}

#else
int main() { return 0; }
#endif
