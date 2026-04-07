#include <QGuiApplication>
#include <QLibraryInfo>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>

#include "app/application_controller.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QCoreApplication::setOrganizationName(QStringLiteral("FastSurfer"));
    QCoreApplication::setApplicationName(QStringLiteral("Luminescent MRI"));
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    ApplicationController controller;
    QQmlApplicationEngine engine;

    QCoreApplication::addLibraryPath(QLibraryInfo::path(QLibraryInfo::PluginsPath));
    engine.addImportPath(QLibraryInfo::path(QLibraryInfo::QmlImportsPath));

    engine.rootContext()->setContextProperty(QStringLiteral("controller"), &controller);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.load(QUrl(QStringLiteral("qrc:/FastSurferDesktop/qml/qtquickdesktop/Main.qml")));

    return app.exec();
}