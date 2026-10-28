#pragma once

#include <QString>

class LauncherUpdater
{
public:
    int Apply(const QString &sourceDirectory, const QString &targetDirectory, const QString &executableName, const QString &cleanupDirectory);
    bool Cleanup(const QString &directory);
    bool IsApplying() const;
};
