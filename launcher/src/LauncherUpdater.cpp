#include "LauncherUpdater.h"

#if defined(Q_OS_WIN)
#include <windows.h>

#include <tlhelp32.h>
#elif defined(Q_OS_LINUX)
#include <cerrno>
#include <csignal>
#endif

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QStringList>
#include <QThread>

int LauncherUpdater::Apply(const QString &sourceDirectory, const QString &targetDirectory, const QString &executableName,
                           const QString &cleanupDirectory)
{
    QDir source(sourceDirectory);
    QDir target(targetDirectory);
    QDir cleanup(cleanupDirectory);
    QString sourceExecutable = source.filePath(executableName);
    if (!source.isAbsolute() || !target.isAbsolute() || source.canonicalPath() == target.canonicalPath() || executableName.isEmpty() ||
        QFileInfo(executableName).fileName() != executableName || !QFileInfo(sourceExecutable).isFile() ||
        QDir(QCoreApplication::applicationDirPath()).canonicalPath() != source.canonicalPath() || cleanup.canonicalPath().isEmpty() ||
        !source.canonicalPath().startsWith(cleanup.canonicalPath() + "/"))
    {
        return 1;
    }

    QFile marker(cleanup.filePath("applying.pid"));
    if (!marker.open(QIODevice::WriteOnly | QIODevice::Truncate) || marker.write(QByteArray::number(QCoreApplication::applicationPid())) <= 0)
    {
        return 1;
    }
    marker.close();

    if (!QDir().mkpath(target.absolutePath()))
    {
        return 1;
    }

    QStringList directories;
    QStringList files;
    QDirIterator iterator(source.absolutePath(), QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (iterator.hasNext())
    {
        QString path   = iterator.next();
        QFileInfo info = iterator.fileInfo();
        if (info.isDir())
        {
            directories.append(path);
        }
        else if (info.isFile())
        {
            files.append(path);
        }
        else
        {
            return 1;
        }
    }

    for (int i = 0; i < directories.size(); i++)
    {
        if (!target.mkpath(source.relativeFilePath(directories[i])))
        {
            return 1;
        }
    }

    QString targetExecutable = target.filePath(executableName);
    QString backupExecutable = targetExecutable + ".updating";
    if (QFile::exists(targetExecutable))
    {
        QFile::remove(backupExecutable);

#if defined(Q_OS_WIN)
        // Launchers that never exit keep the Qt DLLs locked and make every copy fail, so we wait for them and then kill them
        QString targetNativePath = QDir::toNativeSeparators(QFileInfo(targetExecutable).canonicalFilePath());
        HANDLE snapshot          = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        PROCESSENTRY32W entry;
        entry.dwSize = sizeof(entry);
        if (snapshot != INVALID_HANDLE_VALUE && Process32FirstW(snapshot, &entry))
        {
            do
            {
                HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE | PROCESS_TERMINATE, FALSE, entry.th32ProcessID);
                if (!process)
                {
                    continue;
                }

                wchar_t imagePath[MAX_PATH * 4];
                DWORD imagePathSize = MAX_PATH * 4;
                if (QueryFullProcessImageNameW(process, 0, imagePath, &imagePathSize) &&
                    QString::fromWCharArray(imagePath, imagePathSize).compare(targetNativePath, Qt::CaseInsensitive) == 0 &&
                    WaitForSingleObject(process, 5000) == WAIT_TIMEOUT)
                {
                    TerminateProcess(process, 1);
                    WaitForSingleObject(process, 5000);
                }

                CloseHandle(process);
            } while (Process32NextW(snapshot, &entry));
        }

        if (snapshot != INVALID_HANDLE_VALUE)
        {
            CloseHandle(snapshot);
        }
#endif

        bool renamed = false;
        for (int attempt = 0; attempt < 300; attempt++)
        {
            if (QFile::rename(targetExecutable, backupExecutable))
            {
                renamed = true;
                break;
            }

            QThread::msleep(50);
        }

        if (!renamed)
        {
            return 1;
        }
    }
    else if (!QFile::exists(backupExecutable))
    {
        return 1;
    }

    for (int i = 0; i < files.size(); i++)
    {
        if (source.relativeFilePath(files[i]) == executableName)
        {
            files.removeAt(i);
            break;
        }
    }
    files.append(sourceExecutable);

    bool installed = true;
    for (int i = 0; i < files.size(); i++)
    {
        QString relativePath = source.relativeFilePath(files[i]);
        QString targetPath   = target.filePath(relativePath);
        if (!QDir().mkpath(QFileInfo(targetPath).dir().absolutePath()))
        {
            installed = false;
            break;
        }

        bool copied = false;
        for (int attempt = 0; attempt < 300; attempt++)
        {
            if (QFile::exists(targetPath) && !QFile::remove(targetPath))
            {
                QThread::msleep(50);
                continue;
            }

            if (QFile::copy(files[i], targetPath))
            {
                copied = true;
                break;
            }

            QThread::msleep(50);
        }

        if (!copied || !QFile::setPermissions(targetPath, QFile::permissions(files[i])))
        {
            installed = false;
            break;
        }
    }

    if (!installed)
    {
        QFile::remove(targetExecutable);
        QFile::rename(backupExecutable, targetExecutable);
        return 1;
    }

    QFile::remove(backupExecutable);

    QStringList arguments{"--cleanup-launcher-update", cleanup.absolutePath()};
    return QProcess::startDetached(target.filePath(executableName), arguments, target.absolutePath()) ? 0 : 1;
}

bool LauncherUpdater::Cleanup(const QString &directory)
{
    QDir updateRoot(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/launcher-updates");
    QDir updateDirectory(directory);
    if (updateDirectory.dirName().isEmpty() || QFileInfo(updateDirectory.absolutePath()).dir().canonicalPath() != updateRoot.canonicalPath())
    {
        return false;
    }

    for (int attempt = 0; attempt < 300; attempt++)
    {
        if (!updateDirectory.exists() || updateDirectory.removeRecursively())
        {
            return true;
        }

        QThread::msleep(50);
    }

    return false;
}

bool LauncherUpdater::IsApplying() const
{
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    QDir updateRoot(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/launcher-updates");
    QStringList updateDirectories = updateRoot.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (int i = 0; i < updateDirectories.size(); i++)
    {
        QString markerPath = updateRoot.filePath(updateDirectories[i] + "/applying.pid");
        QFileInfo markerInfo(markerPath);
        if (!markerInfo.isFile())
        {
            continue;
        }

        QFile marker(markerPath);
        if (!marker.open(QIODevice::ReadOnly))
        {
            return true;
        }

        bool validPid = false;
        qint64 pid    = marker.readAll().trimmed().toLongLong(&validPid);
        if (!validPid || pid <= 0)
        {
            QFile::remove(markerPath);
            continue;
        }

#if defined(Q_OS_WIN)
        HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, (DWORD) pid);
        if (!process)
        {
            if (GetLastError() == ERROR_ACCESS_DENIED)
            {
                return true;
            }

            QFile::remove(markerPath);
            continue;
        }

        DWORD exitCode = 0;
        bool running   = GetExitCodeProcess(process, &exitCode) && exitCode == STILL_ACTIVE;
        CloseHandle(process);
        if (running)
        {
            return true;
        }

        QFile::remove(markerPath);
#else
        if (kill((pid_t) pid, 0) == 0 || errno == EPERM)
        {
            return true;
        }

        QFile::remove(markerPath);
#endif
    }
#endif

    return false;
}
