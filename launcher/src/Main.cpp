#include "LauncherState.h"
#include "LauncherUpdater.h"

#include <QCoreApplication>
#include <QDebug>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[])
{
    if (argc == 6 && QString::fromLocal8Bit(argv[1]) == "--apply-launcher-update")
    {
        QCoreApplication application(argc, argv);
        LauncherUpdater updater;
        return updater.Apply(QString::fromLocal8Bit(argv[2]), QString::fromLocal8Bit(argv[3]), QString::fromLocal8Bit(argv[4]),
                             QString::fromLocal8Bit(argv[5]));
    }

    QGuiApplication application(argc, argv);
    application.setApplicationName("Everstead Launcher");
    application.setOrganizationName("Everstead");
    application.setWindowIcon(QIcon(":/icon/Launcher.png"));
    application.setDesktopFileName("everstead-launcher");
    if (argc == 3 && QString::fromLocal8Bit(argv[1]) == "--cleanup-launcher-update")
    {
        LauncherUpdater updater;
        if (!updater.Cleanup(QString::fromLocal8Bit(argv[2])))
        {
            qWarning() << "Could not remove the completed launcher update";
        }
    }
    else
    {
        LauncherUpdater updater;
        if (updater.IsApplying())
        {
            return 0;
        }
    }

    LauncherState launcherState;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("launcherState", &launcherState);
    engine.loadFromModule("Everstead.Launcher", "Main");
    if (engine.rootObjects().isEmpty())
    {
        return 1;
    }

    launcherState.CheckForLauncherUpdates();
    return application.exec();
}
