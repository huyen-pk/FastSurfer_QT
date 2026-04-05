#include <QApplication>

#include "app/MainWindow.h"
#include "app/Theme.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("FastSurfer Desktop");
    app.setOrganizationName("FastSurfer");

    desktop::Theme::apply(app);

    desktop::MainWindow window;
    window.show();

    return app.exec();
}
