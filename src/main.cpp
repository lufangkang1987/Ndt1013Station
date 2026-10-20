#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDir>
#include <QTimer>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QFontDatabase>
#include <QDebug>
#include "Station.h"
int main(int argc,char **argv) {
    QQuickStyle::setStyle(QStringLiteral("Fusion"));
    QGuiApplication app(argc,argv);
    if(app.arguments().contains(QStringLiteral("--smoke-test"))) {
        // The offscreen platform does not enumerate Windows system fonts.
        QFontDatabase::addApplicationFont(QStringLiteral("C:/Windows/Fonts/msyh.ttc"));
        QFontDatabase::addApplicationFont(QStringLiteral("C:/Windows/Fonts/msyhbd.ttc"));
        QFontDatabase::addApplicationFont(QStringLiteral("C:/Windows/Fonts/segoeui.ttf"));
    }
    QCoreApplication::setApplicationName(QStringLiteral("Ndt1013Station"));
    auto *provider=new FrameProvider;
    Station station(provider);
    // Locals are destroyed in reverse order: QML bindings must disappear
    // before their context object. The engine owns the image provider.
    QQmlApplicationEngine engine;
    qInfo().noquote() << QStringLiteral("Ndt1013Station | UI lifecycle fix 2026-10-20 | style:")
                      << QQuickStyle::name() << QStringLiteral("| executable:")
                      << QCoreApplication::applicationFilePath();
    engine.addImageProvider(QStringLiteral("detector"),provider);
    engine.rootContext()->setContextProperty(QStringLiteral("station"),&station);
    engine.rootContext()->setContextProperty(QStringLiteral("defaultWorkDir"),QDir::toNativeSeparators(QCoreApplication::applicationDirPath()+QStringLiteral("/work_dir/NDT1013LA")));
    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    if(engine.rootObjects().isEmpty()) return 1;
    if(app.arguments().contains(QStringLiteral("--smoke-test"))) {
        const int rawArg=app.arguments().indexOf(QStringLiteral("--raw"));
        if(rawArg>=0 && rawArg+1<app.arguments().size()) {
            if(!station.openRaw(QUrl::fromLocalFile(app.arguments().at(rawArg+1)))) return 4;
        } else { station.connectDevice(QString()); station.capture(); }
        if(station.frames()!=1) return 2;
        QTimer::singleShot(1200,&app,[&]{
            const int screenshotArg=app.arguments().indexOf(QStringLiteral("--screenshot"));
            if(screenshotArg>=0 && screenshotArg+1<app.arguments().size()) {
                auto *window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());
                if(!window || !window->grabWindow().save(app.arguments().at(screenshotArg+1))) { app.exit(3); return; }
            }
            app.quit();
        });
    }
    return app.exec();
}
