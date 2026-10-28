#include "LauncherState.h"

#include <algorithm>
#include <vector>

#if defined(Q_OS_WIN)
#include <windows.h>
#elif defined(Q_OS_LINUX)
#include <cerrno>
#include <fcntl.h>
#include <pty.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#include "GameLogModel.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSettings>
#include <QSocketNotifier>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextList>
#include <QTimer>
#include <QUrl>
#include <QUuid>
#include <QVersionNumber>

#include <archive.h>
#include <archive_entry.h>

#if defined(Q_OS_LINUX)
#include <zstd.h>
#endif

const QStringList LauncherState::s_themes = {"forest", "ocean", "amethyst", "ember", "graphite"};

LauncherState::LauncherState(QObject *parent) : QObject(parent)
{
    m_networkAccessManager  = new QNetworkAccessManager(this);
    m_manifestReply         = nullptr;
    m_downloadReply         = nullptr;
    m_downloadFile          = nullptr;
    m_launcherReply         = nullptr;
    m_launcherDownloadReply = nullptr;
    m_launcherDownloadFile  = nullptr;
    m_changelogReply        = nullptr;
    m_gameProcess           = new QProcess(this);
    m_gameLogModel          = new GameLogModel(this);
    m_gameLogUpdateTimer    = new QTimer(this);
#if defined(Q_OS_LINUX)
    m_gamePtyNotifier = nullptr;
    m_gamePtyMaster   = -1;
    m_gamePtySlave    = -1;
#endif
    m_platform               = GetPlatform();
    m_downloadedBytes        = 0;
    m_downloadSize           = 0;
    m_progress               = 0.0;
    m_launcherUpdateProgress = 0.0;
    m_reinstallingExecutable = false;
    m_loading                = false;
    m_platformAvailable      = false;
    m_installed              = false;
    m_gameRunning            = false;
    m_launcherUpdateActive   = true;
    m_profilesLoaded         = false;
    m_changelogLoading       = false;
    m_launchAfterLoad        = false;
    m_launchAfterInstall     = false;
    m_installAfterLoad       = false;
    m_captureGameLogs        = false;
    m_logWindowVisible       = false;
    m_activeProfile          = -1;
    m_status                 = "Checking for updates";
    m_launcherUpdateStatus   = "Checking for launcher updates";
    m_gameLogDecoder         = QStringDecoder(QStringDecoder::Utf8);
    m_gameLogUpdateTimer->setSingleShot(true);
    m_gameLogUpdateTimer->setInterval(16);
    QSettings settings;
    m_theme                = settings.value("theme").toString();
    m_minimizeWhilePlaying = settings.value("minimizeWhilePlaying", false).toBool();
    if (!s_themes.contains(m_theme))
    {
        m_theme = s_themes[0];
    }

#if defined(Q_OS_WIN)
    m_gameProcess->setCreateProcessArgumentsModifier(
            [](QProcess::CreateProcessArguments *arguments)
            {
                arguments->flags |= CREATE_NO_WINDOW;
            });
#endif
    connect(m_gameProcess, &QProcess::readyReadStandardOutput, this, &LauncherState::GameReadyRead);
    connect(m_gameProcess, &QProcess::finished, this, &LauncherState::GameFinished);
    connect(m_gameProcess, &QProcess::errorOccurred, this, &LauncherState::GameError);
    connect(m_gameLogUpdateTimer, &QTimer::timeout, this, &LauncherState::GameLogChanged);
#if defined(Q_OS_LINUX)
    connect(m_gameProcess, &QProcess::started, this,
            [this]
            {
                if (m_gamePtySlave >= 0)
                {
                    close(m_gamePtySlave);
                    m_gamePtySlave = -1;
                }
            });
    InstallDesktopEntry("everstead-launcher", "Everstead Launcher", ":/icon/Launcher.png", false);
#endif
}

QString LauncherState::GetLatestVersion() const
{
    return m_latestVersion;
}

QString LauncherState::GetLauncherVersion() const
{
    return EVERSTEAD_LAUNCHER_VERSION;
}

QString LauncherState::GetSelectedVersion() const
{
    return m_selectedVersion;
}

QStringList LauncherState::GetVersions() const
{
    return m_versions;
}

QStringList LauncherState::GetInstalledVersions() const
{
    return m_installedVersions;
}

QString LauncherState::GetPlatform() const
{
#if defined(Q_OS_WIN)
    if (QSysInfo::currentCpuArchitecture() == "x86_64")
    {
        return "windows-x64";
    }
#elif defined(Q_OS_LINUX)
    if (QSysInfo::currentCpuArchitecture() == "x86_64")
    {
        return "linux-x64";
    }

    if (QSysInfo::currentCpuArchitecture() == "aarch64")
    {
        return "linux-arm64";
    }
#endif

    return "unsupported";
}

QString LauncherState::GetStatus() const
{
    return m_status;
}

QString LauncherState::GetError() const
{
    return m_error;
}

QString LauncherState::GetActionText() const
{
    if (m_gameRunning)
    {
        return "STOP";
    }

    if (m_loading)
    {
        return m_launchAfterInstall ? "INSTALLING" : "CHECKING";
    }

    if (!m_error.isEmpty())
    {
        return "TRY AGAIN";
    }

    return m_installed ? "PLAY" : "INSTALL";
}

double LauncherState::GetProgress() const
{
    return m_progress;
}

bool LauncherState::IsLoading() const
{
    return m_loading;
}

bool LauncherState::IsPlatformAvailable() const
{
    return m_platformAvailable;
}

bool LauncherState::IsInstalled() const
{
    return m_installed;
}

bool LauncherState::IsGameRunning() const
{
    return m_gameRunning;
}

bool LauncherState::IsLauncherUpdateActive() const
{
    return m_launcherUpdateActive;
}

QString LauncherState::GetLauncherUpdateStatus() const
{
    return m_launcherUpdateStatus;
}

QString LauncherState::GetLauncherUpdateError() const
{
    return m_launcherUpdateError;
}

double LauncherState::GetLauncherUpdateProgress() const
{
    return m_launcherUpdateProgress;
}

QVariantList LauncherState::GetProfiles() const
{
    return m_profiles;
}

int LauncherState::GetActiveProfile() const
{
    return m_activeProfile;
}

QAbstractItemModel *LauncherState::GetGameLogModel() const
{
    return m_gameLogModel;
}

bool LauncherState::IsLogWindowVisible() const
{
    return m_logWindowVisible;
}

void LauncherState::SetLogWindowVisible(bool visible)
{
    if (m_logWindowVisible == visible)
    {
        return;
    }

    m_logWindowVisible = visible;
    emit LogWindowVisibleChanged();
}

QString LauncherState::GetTheme() const
{
    return m_theme;
}

void LauncherState::SetTheme(const QString &theme)
{
    if (theme == m_theme || !s_themes.contains(theme))
    {
        return;
    }

    m_theme = theme;
    QSettings().setValue("theme", theme);
    emit SettingsChanged();
}

bool LauncherState::IsMinimizeWhilePlaying() const
{
    return m_minimizeWhilePlaying;
}

void LauncherState::SetMinimizeWhilePlaying(bool minimize)
{
    if (minimize == m_minimizeWhilePlaying)
    {
        return;
    }

    m_minimizeWhilePlaying = minimize;
    QSettings().setValue("minimizeWhilePlaying", minimize);
    emit SettingsChanged();
}

QString LauncherState::GetChangelog() const
{
    return m_changelog;
}

QString LauncherState::GetChangelogError() const
{
    return m_changelogError;
}

bool LauncherState::IsChangelogLoading() const
{
    return m_changelogLoading;
}

void LauncherState::CheckForLauncherUpdates()
{
    if (m_launcherReply || m_launcherDownloadReply)
    {
        return;
    }

    qInfo() << "Checking for launcher updates";
    m_launcherUpdateActive   = true;
    m_launcherUpdateProgress = 0.0;
    m_launcherUpdateError.clear();
    m_launcherUpdateStatus = "Checking for launcher updates";
    QNetworkRequest request{QUrl("https://pub-0174d9cb4ae44e93967c2a673e1db201.r2.dev/manifest.json")};
    m_launcherReply = m_networkAccessManager->get(request);
    connect(m_launcherReply, &QNetworkReply::finished, this, &LauncherState::LauncherRootManifestFinished);
    emit Changed();
}

void LauncherState::RefreshChangelog()
{
    if (m_launcherUpdateActive || m_changelogReply)
    {
        return;
    }

    m_changelogLoading = true;
    m_changelogError.clear();
    QNetworkRequest request{QUrl("https://pub-0174d9cb4ae44e93967c2a673e1db201.r2.dev/changelog.md")};
    m_changelogReply = m_networkAccessManager->get(request);
    connect(m_changelogReply, &QNetworkReply::finished, this, &LauncherState::ChangelogFinished);
    emit ChangelogChanged();
}

void LauncherState::Refresh()
{
    if (m_launcherUpdateActive || m_loading)
    {
        return;
    }

    qInfo() << "Checking launcher manifests";
    m_loading           = true;
    m_platformAvailable = false;
    m_installed         = false;
    m_progress          = 0.0;
    m_error.clear();
    m_status = "Checking available versions";
    QNetworkRequest request{QUrl("https://pub-0174d9cb4ae44e93967c2a673e1db201.r2.dev/manifest.json")};
    m_manifestReply = m_networkAccessManager->get(request);
    connect(m_manifestReply, &QNetworkReply::finished, this, &LauncherState::RootManifestFinished);
    emit Changed();
}

void LauncherState::SelectVersion(const QString &version)
{
    if (m_loading || !m_versions.contains(version))
    {
        return;
    }

    LoadVersion(version);
}

void LauncherState::InstallVersion(const QString &version)
{
    if (m_loading || m_gameRunning || !m_versions.contains(version))
    {
        return;
    }

    m_installAfterLoad = true;
    LoadVersion(version);
}

void LauncherState::RemoveVersion(const QString &version)
{
    if (m_loading || m_gameRunning || !m_installedVersions.contains(version))
    {
        return;
    }

    qInfo() << "Removing version" << version;
    if (!QDir(GetVersionsDirectory() + "/" + version).removeRecursively())
    {
        SetFailure("Could not remove version " + version);
        UpdateInstalledVersions();
        return;
    }

    if (version == m_selectedVersion)
    {
        m_installed = false;
        m_status    = "Version " + version + " is ready to install";
    }

    UpdateInstalledVersions();
    emit Changed();
}

void LauncherState::CreateProfile()
{
    QVariantMap profile;
    profile.insert("name", "Profile " + QString::number(m_profiles.size() + 1));
    profile.insert("version", "Latest");
    profile.insert("showLogs", false);
    profile.insert("environment", QVariantList());
    m_profiles.append(profile);
    SaveProfiles();
    emit ProfilesChanged();
}

void LauncherState::DeleteProfile(int index)
{
    if (index < 0 || index >= m_profiles.size() || m_gameRunning && index == m_activeProfile)
    {
        return;
    }

    m_profiles.removeAt(index);
    if (m_activeProfile > index)
    {
        m_activeProfile--;
    }

    SaveProfiles();
    emit ProfilesChanged();
    emit Changed();
}

void LauncherState::SaveProfile(int index, const QString &name, const QVariantList &environment)
{
    if (index < 0 || index >= m_profiles.size())
    {
        return;
    }

    QVariantMap profile = m_profiles[index].toMap();
    profile.insert("name", name.isEmpty() ? "Unnamed profile" : name);
    profile.insert("environment", environment);
    m_profiles[index] = profile;
    SaveProfiles();
    emit ProfilesChanged();
}

void LauncherState::SetProfileVersion(int index, const QString &version)
{
    if (index < 0 || index >= m_profiles.size() || version != "Latest" && !m_versions.contains(version))
    {
        return;
    }

    QVariantMap profile = m_profiles[index].toMap();
    profile.insert("version", version);
    m_profiles[index] = profile;
    SaveProfiles();
    emit ProfilesChanged();
}

void LauncherState::SetProfileShowLogs(int index, bool showLogs)
{
    if (index < 0 || index >= m_profiles.size())
    {
        return;
    }

    QVariantMap profile = m_profiles[index].toMap();
    profile.insert("showLogs", showLogs);
    m_profiles[index] = profile;
    SaveProfiles();
    emit ProfilesChanged();
}

void LauncherState::AddProfileVariable(int index)
{
    if (index < 0 || index >= m_profiles.size())
    {
        return;
    }

    QVariantMap profile      = m_profiles[index].toMap();
    QVariantList environment = profile.value("environment").toList();
    QVariantMap variable;
    variable.insert("id", QUuid::createUuid().toString(QUuid::WithoutBraces));
    variable.insert("name", "");
    variable.insert("value", "");
    environment.append(variable);
    profile.insert("environment", environment);
    m_profiles[index] = profile;
    SaveProfiles();
    emit ProfilesChanged();
}

void LauncherState::RemoveProfileVariable(int profileIndex, const QString &variableId)
{
    int variableIndex = GetProfileVariableIndex(profileIndex, variableId);
    if (variableIndex < 0)
    {
        return;
    }

    QVariantMap profile      = m_profiles[profileIndex].toMap();
    QVariantList environment = profile.value("environment").toList();
    environment.removeAt(variableIndex);
    profile.insert("environment", environment);
    m_profiles[profileIndex] = profile;
    SaveProfiles();
    emit ProfilesChanged();
}

void LauncherState::PlayProfile(int index)
{
    if (index < 0 || index >= m_profiles.size() || m_loading || m_gameRunning && index != m_activeProfile)
    {
        return;
    }

    if (m_gameRunning)
    {
        Play();
        return;
    }

    QVariantMap profile     = m_profiles[index].toMap();
    QString version         = profile.value("version").toString();
    QString resolvedVersion = version == "Latest" ? m_latestVersion : version;
    if (resolvedVersion.isEmpty() || !m_versions.contains(resolvedVersion))
    {
        SetFailure("This profile has no available version");
        return;
    }

    m_activeProfile   = index;
    m_launchAfterLoad = true;
    if (m_selectedVersion != resolvedVersion || !m_platformAvailable)
    {
        LoadVersion(resolvedVersion);
        return;
    }

    m_launchAfterLoad = false;
    Play();
}

void LauncherState::OpenDataDirectory()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/Everstead";
    QDir().mkpath(path);
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void LauncherState::Play()
{
    if (m_gameRunning)
    {
        m_status = "Stopping game";
        qInfo() << "Stopping game";
        m_gameProcess->kill();
        emit Changed();
        return;
    }

    if (m_loading)
    {
        return;
    }

    if (!m_error.isEmpty())
    {
        Refresh();
        return;
    }

    if (!m_platformAvailable)
    {
        SetFailure("This version is unavailable on " + m_platform);
        return;
    }

    QString executablePath = GetInstallDirectory() + "/" + m_gameExecutable;
    if (!m_installed)
    {
        if (QDir(GetInstallDirectory() + "/assets").exists() && !QFileInfo(executablePath).isFile())
        {
            ReinstallExecutable();
            return;
        }

        StartInstall(true);
        return;
    }

    if (!QFileInfo(executablePath).isFile())
    {
        ReinstallExecutable();
        return;
    }

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    bool showLogs                   = false;
    if (m_activeProfile >= 0 && m_activeProfile < m_profiles.size())
    {
        QVariantMap profile             = m_profiles[m_activeProfile].toMap();
        QVariantList profileEnvironment = profile.value("environment").toList();
        showLogs                        = profile.value("showLogs").toBool();
        for (const QVariant &variableValue : profileEnvironment)
        {
            QVariantMap variable = variableValue.toMap();
            QString name         = variable.value("name").toString();
            if (!name.isEmpty())
            {
                environment.insert(name, variable.value("value").toString());
            }
        }
    }

    m_captureGameLogs = showLogs;
#if defined(Q_OS_LINUX)
    CloseGamePty();
    m_gameProcess->setChildProcessModifier({});
    InstallDesktopEntry("everstead", "Everstead", GetInstallDirectory() + "/assets/icon.png", true);
    environment.insert("SDL_VIDEO_WAYLAND_WMCLASS", "everstead");
    environment.insert("SDL_VIDEO_X11_WMCLASS", "everstead");
#endif
    if (m_captureGameLogs)
    {
        m_gameLogUpdateTimer->stop();
        m_gameLogModel->Clear();
        m_gameLogDecoder = QStringDecoder(QStringDecoder::Utf8);
        emit GameLogChanged();
        SetLogWindowVisible(true);
#if defined(Q_OS_LINUX)
        if (!OpenGamePty())
        {
            SetFailure("Could not open the game log terminal");
            return;
        }
#else
        m_gameProcess->setProcessChannelMode(QProcess::MergedChannels);
#endif
    }
    else
    {
        m_gameProcess->setProcessChannelMode(QProcess::ForwardedChannels);
    }

    qInfo() << "Launching" << executablePath;
    m_gameProcess->setProcessEnvironment(environment);
    m_gameProcess->setWorkingDirectory(GetInstallDirectory());
    m_gameProcess->start(executablePath, {});
    m_gameRunning = true;
    m_status      = "Game is running";
    emit Changed();
}

void LauncherState::StartInstall(bool launch)
{
    QDir(GetStagingDirectory()).removeRecursively();
    QDir().mkpath(GetStagingDirectory());
    m_loading                = true;
    m_progress               = 0.0;
    m_reinstallingExecutable = false;
    m_launchAfterInstall     = launch;
    m_status                 = "Preparing " + m_selectedVersion;
    qInfo() << "Installing version" << m_selectedVersion;
    StartDownload(m_gamePackage);
    emit Changed();
}

void LauncherState::UpdateInstalledVersions()
{
    m_installedVersions.clear();
    QStringList directories = QDir(GetVersionsDirectory()).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &directory : directories)
    {
        if (!directory.endsWith(".installing") && QDir(GetVersionsDirectory() + "/" + directory + "/assets").exists())
        {
            m_installedVersions.append(directory);
        }
    }

    std::sort(m_installedVersions.begin(), m_installedVersions.end(),
              [](const QString &a, const QString &b)
              {
                  return QVersionNumber::compare(QVersionNumber::fromString(a), QVersionNumber::fromString(b)) > 0;
              });
}

void LauncherState::LoadVersion(const QString &version)
{
    m_loading                = true;
    m_platformAvailable      = false;
    m_installed              = false;
    m_progress               = 0.0;
    m_reinstallingExecutable = false;
    m_error.clear();
    m_selectedVersion = version;
    m_status          = "Loading version " + version;
    qInfo() << "Loading version manifest" << version;
    QNetworkRequest request{QUrl("https://pub-0174d9cb4ae44e93967c2a673e1db201.r2.dev/versions/" + version + "/manifest.json")};
    m_manifestReply = m_networkAccessManager->get(request);
    connect(m_manifestReply, &QNetworkReply::finished, this, &LauncherState::VersionManifestFinished);
    emit Changed();
}

void LauncherState::ApplyRootManifest(const QByteArray &data)
{
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
    {
        SetFailure("The update manifest is invalid");
        return;
    }

    QJsonObject manifest = document.object();
    m_latestVersion      = manifest.value("latest").toString();
    m_versions.clear();
    QJsonArray versions = manifest.value("versions").toArray();
    for (const QJsonValue &value : versions)
    {
        QString version = value.toString();
        if (!version.isEmpty() && version != "." && version != ".." && !version.contains('\\') && QFileInfo(version).fileName() == version &&
            !m_versions.contains(version))
        {
            m_versions.append(version);
        }
    }

    std::sort(m_versions.begin(), m_versions.end(),
              [](const QString &a, const QString &b)
              {
                  return QVersionNumber::compare(QVersionNumber::fromString(a), QVersionNumber::fromString(b)) > 0;
              });

    if (m_latestVersion.isEmpty() || !m_versions.contains(m_latestVersion))
    {
        SetFailure("The update manifest has no valid latest version");
        return;
    }

    UpdateInstalledVersions();
    LoadVersion(m_latestVersion);
}

void LauncherState::RootManifestFinished()
{
    QNetworkReply *reply = m_manifestReply;
    m_manifestReply      = nullptr;
    if (reply->error() != QNetworkReply::NoError)
    {
        qWarning() << "Root manifest request failed:" << reply->errorString();
        SetFailure("Could not reach the update service");
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();
    ApplyRootManifest(data);
}

void LauncherState::VersionManifestFinished()
{
    QNetworkReply *reply = m_manifestReply;
    m_manifestReply      = nullptr;
    if (reply->error() != QNetworkReply::NoError)
    {
        qWarning() << "Version manifest request failed:" << reply->errorString();
        SetFailure("Could not load the version manifest");
        reply->deleteLater();
        return;
    }

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &parseError);
    reply->deleteLater();
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
    {
        SetFailure("The version manifest is invalid");
        return;
    }

    QJsonObject manifest = document.object();
    QJsonObject release  = manifest.value("releases").toObject().value(m_platform).toObject();
    m_gamePackage.file   = release.value("file").toString();
    m_gamePackage.sha256 = release.value("sha256").toString();
    m_gamePackage.size   = release.value("size").toInteger(-1);
    m_gameExecutable     = release.value("executable").toString();
    m_loading            = false;
    QRegularExpression hashExpression("^[0-9a-fA-F]{64}$");
    m_platformAvailable = manifest.value("version").toString() == m_selectedVersion && !m_gamePackage.file.isEmpty() &&
                          hashExpression.match(m_gamePackage.sha256).hasMatch() && m_gamePackage.size > 0 && !m_gameExecutable.isEmpty() &&
                          QFileInfo(m_gamePackage.file).fileName() == m_gamePackage.file &&
                          QFileInfo(m_gameExecutable).fileName() == m_gameExecutable;
    m_installed =
            m_platformAvailable && QFile::exists(GetInstallDirectory() + "/" + m_gameExecutable) && QDir(GetInstallDirectory() + "/assets").exists();
    if (m_platformAvailable)
    {
        m_status = m_installed ? "Version " + m_selectedVersion + " is installed" : "Version " + m_selectedVersion + " is ready to install";
        qInfo() << "Version" << m_selectedVersion << (m_installed ? "is installed" : "is ready to install");
    }
    else
    {
        m_status = "Version " + m_selectedVersion + " is unavailable on this platform";
        qWarning() << "No valid build for" << m_platform << "in version" << m_selectedVersion;
    }

    if (m_launchAfterLoad)
    {
        m_launchAfterLoad = false;
        Play();
        return;
    }

    if (m_installAfterLoad)
    {
        m_installAfterLoad = false;
        if (m_platformAvailable && !m_installed)
        {
            StartInstall(false);
            return;
        }
    }

    emit Changed();
}

void LauncherState::DownloadReadyRead()
{
    m_downloadFile->write(m_downloadReply->readAll());
}

void LauncherState::DownloadProgress(qint64 received, qint64 total)
{
    m_downloadedBytes = received;
    m_downloadSize    = total > 0 ? total : m_downloadSize;
    if (m_downloadSize > 0)
    {
        m_progress = (double) m_downloadedBytes / (double) m_downloadSize;
    }

    emit Changed();
}

void LauncherState::DownloadFinished()
{
    m_downloadFile->write(m_downloadReply->readAll());
    m_downloadFile->close();
    QString path = m_downloadFile->fileName();
    m_downloadFile->deleteLater();
    m_downloadFile       = nullptr;
    QNetworkReply *reply = m_downloadReply;
    m_downloadReply      = nullptr;
    if (reply->error() != QNetworkReply::NoError)
    {
        qWarning() << "Download failed:" << reply->errorString();
        reply->deleteLater();
        SetFailure("Could not download " + QFileInfo(path).fileName());
        return;
    }

    reply->deleteLater();
    if (!VerifyFile(path, m_gamePackage))
    {
        qWarning() << "Downloaded file did not match its manifest:" << path;
        QFile::remove(path);
        SetFailure("Downloaded file did not pass verification");
        return;
    }

    m_status = "Installing game files";
    emit Changed();
    if (!ExtractArchive(path, GetStagingDirectory()))
    {
        SetFailure("Could not unpack the game files");
        return;
    }

    QFile::remove(path);
    path = GetStagingDirectory() + "/" + m_gameExecutable;
    if (!QFileInfo(path).isFile() || !QDir(GetStagingDirectory() + "/assets").exists())
    {
        SetFailure("The game package is incomplete");
        return;
    }

#if defined(Q_OS_LINUX)
    QFile executable(path);
    executable.setPermissions(executable.permissions() | QFileDevice::ExeOwner | QFileDevice::ExeGroup | QFileDevice::ExeOther);
#endif

    if (m_reinstallingExecutable)
    {
        QString executablePath = GetInstallDirectory() + "/" + m_gameExecutable;
        if ((QFileInfo::exists(executablePath) && !QFile::remove(executablePath)) || !QFile::rename(path, executablePath))
        {
            SetFailure("Could not restore the game executable");
            return;
        }

        QDir(GetStagingDirectory()).removeRecursively();
        m_loading                = false;
        m_installed              = true;
        m_progress               = 1.0;
        m_reinstallingExecutable = false;
        m_status                 = "Game executable restored";
        qInfo() << "Reinstalled executable for version" << m_selectedVersion;
        if (m_launchAfterInstall)
        {
            m_launchAfterInstall = false;
            Play();
            return;
        }

        emit Changed();
        return;
    }

    QDir(GetInstallDirectory()).removeRecursively();
    QDir versionRoot(QFileInfo(GetInstallDirectory()).dir());
    if (!versionRoot.rename(QFileInfo(GetStagingDirectory()).fileName(), QFileInfo(GetInstallDirectory()).fileName()))
    {
        SetFailure("Could not finish the installation");
        return;
    }

    m_loading   = false;
    m_installed = true;
    m_progress  = 1.0;
    m_status    = "Version " + m_selectedVersion + " is installed";
    qInfo() << "Installed version" << m_selectedVersion;
    UpdateInstalledVersions();
    if (m_launchAfterInstall)
    {
        m_launchAfterInstall = false;
        Play();
        return;
    }

    emit Changed();
}

void LauncherState::GameReadyRead()
{
    if (!m_captureGameLogs)
    {
        return;
    }

    AppendGameLogOutput(m_gameProcess->readAllStandardOutput());
}

void LauncherState::AppendGameLogOutput(const QByteArray &output)
{
    if (output.isEmpty())
    {
        return;
    }

    m_gameLogModel->AppendText(m_gameLogDecoder(output));

    if (!m_gameLogUpdateTimer->isActive())
    {
        m_gameLogUpdateTimer->start();
    }
}

#if defined(Q_OS_LINUX)
bool LauncherState::OpenGamePty()
{
    if (openpty(&m_gamePtyMaster, &m_gamePtySlave, nullptr, nullptr, nullptr) != 0)
    {
        return false;
    }

    int flags = fcntl(m_gamePtyMaster, F_GETFL);
    if (flags < 0 || fcntl(m_gamePtyMaster, F_SETFL, flags | O_NONBLOCK) != 0)
    {
        CloseGamePty();
        return false;
    }

    int master = m_gamePtyMaster;
    int slave  = m_gamePtySlave;
    m_gameProcess->setChildProcessModifier(
            [master, slave]
            {
                setsid();
                ioctl(slave, TIOCSCTTY, 0);
                dup2(slave, STDIN_FILENO);
                dup2(slave, STDOUT_FILENO);
                dup2(slave, STDERR_FILENO);
                close(master);
                if (slave > STDERR_FILENO)
                {
                    close(slave);
                }
            });
    m_gamePtyNotifier = new QSocketNotifier(m_gamePtyMaster, QSocketNotifier::Read, this);
    connect(m_gamePtyNotifier, &QSocketNotifier::activated, this, &LauncherState::GamePtyReadyRead);
    return true;
}

void LauncherState::GamePtyReadyRead()
{
    QByteArray output;
    char buffer[16384];
    while (true)
    {
        ssize_t size = read(m_gamePtyMaster, buffer, sizeof(buffer));
        if (size > 0)
        {
            output.append(buffer, size);
        }
        else if (size < 0 && errno == EINTR)
        {
            continue;
        }
        else
        {
            break;
        }
    }

    AppendGameLogOutput(output);
}

void LauncherState::CloseGamePty()
{
    delete m_gamePtyNotifier;
    m_gamePtyNotifier = nullptr;
    if (m_gamePtyMaster >= 0)
    {
        close(m_gamePtyMaster);
        m_gamePtyMaster = -1;
    }

    if (m_gamePtySlave >= 0)
    {
        close(m_gamePtySlave);
        m_gamePtySlave = -1;
    }
}

// Wayland compositors ignore icons set by the window and look them up from the desktop entry that matches the app id
void LauncherState::InstallDesktopEntry(const QString &name, const QString &title, const QString &iconPath, bool hidden) const
{
    QString dataDirectory = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QString iconDirectory = dataDirectory + "/icons/hicolor/256x256/apps";
    QString iconFile      = iconDirectory + "/" + name + ".png";
    QDir().mkpath(iconDirectory);
    QDir().mkpath(dataDirectory + "/applications");
    QFile::remove(iconFile);
    QFile::copy(iconPath, iconFile);
    QFile::setPermissions(iconFile, QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::ReadOther);

    QFile entry(dataDirectory + "/applications/" + name + ".desktop");
    if (!entry.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        qWarning() << "Could not write desktop entry" << entry.fileName();
        return;
    }

    entry.write(QString("[Desktop Entry]\nType=Application\nName=%1\nExec=\"%2\"\nIcon=%3\nStartupWMClass=%4\nCategories=Game;\nNoDisplay=%5\n")
                        .arg(title, QCoreApplication::applicationFilePath(), iconFile, name, hidden ? "true" : "false")
                        .toUtf8());
}
#endif

void LauncherState::GameFinished()
{
#if defined(Q_OS_LINUX)
    if (m_captureGameLogs)
    {
        GamePtyReadyRead();
        CloseGamePty();
    }
#endif
    GameReadyRead();
    if (m_captureGameLogs)
    {
        m_gameLogUpdateTimer->stop();
        emit GameLogChanged();
    }

    m_gameRunning = false;
    m_status      = "Game closed";
    qInfo() << "Game closed";
    emit Changed();
}

void LauncherState::GameError()
{
    bool executableMissing = m_gameProcess->error() == QProcess::FailedToStart && !QFileInfo(GetInstallDirectory() + "/" + m_gameExecutable).isFile();
#if defined(Q_OS_LINUX)
    if (m_captureGameLogs)
    {
        GamePtyReadyRead();
        CloseGamePty();
    }
#endif
    GameReadyRead();
    if (m_captureGameLogs)
    {
        m_gameLogUpdateTimer->stop();
        emit GameLogChanged();
    }

    if (!m_gameRunning)
    {
        return;
    }

    m_gameRunning = false;
    if (executableMissing)
    {
        qWarning() << "Game executable disappeared before it could start";
        ReinstallExecutable();
        return;
    }

    m_status = "Could not start the game";
    qWarning() << "Could not start the game:" << m_gameProcess->errorString();
    emit Changed();
}

void LauncherState::ReinstallExecutable()
{
    QDir stagingDirectory(GetStagingDirectory());
    if ((stagingDirectory.exists() && !stagingDirectory.removeRecursively()) || !QDir().mkpath(stagingDirectory.absolutePath()))
    {
        SetFailure("Could not create the executable installation files");
        return;
    }

    m_loading                = true;
    m_progress               = 0.0;
    m_reinstallingExecutable = true;
    m_launchAfterInstall     = true;
    m_status                 = "Reinstalling missing game executable";
    qInfo() << "Reinstalling missing executable for version" << m_selectedVersion;
    StartDownload(m_gamePackage);
    emit Changed();
}

void LauncherState::StartDownload(const DownloadFile &file)
{
    m_downloadedBytes = 0;
    m_downloadSize    = file.size;
    m_progress        = 0.0;
    QString path      = GetStagingDirectory() + "/" + file.file;
    m_downloadFile    = new QFile(path, this);
    if (!m_downloadFile->open(QIODevice::WriteOnly))
    {
        m_downloadFile->deleteLater();
        m_downloadFile = nullptr;
        SetFailure("Could not create the installation files");
        return;
    }

    m_status = m_reinstallingExecutable ? "Reinstalling " + file.file : "Downloading " + file.file;
    qInfo() << "Downloading" << file.file << "for" << m_selectedVersion;
    QNetworkRequest request{QUrl("https://pub-0174d9cb4ae44e93967c2a673e1db201.r2.dev/versions/" + m_selectedVersion + "/" + file.file)};
    m_downloadReply = m_networkAccessManager->get(request);
    connect(m_downloadReply, &QNetworkReply::readyRead, this, &LauncherState::DownloadReadyRead);
    connect(m_downloadReply, &QNetworkReply::downloadProgress, this, &LauncherState::DownloadProgress);
    connect(m_downloadReply, &QNetworkReply::finished, this, &LauncherState::DownloadFinished);
}

void LauncherState::LauncherRootManifestFinished()
{
    QNetworkReply *reply = m_launcherReply;
    m_launcherReply      = nullptr;
    if (reply->error() != QNetworkReply::NoError)
    {
        qWarning() << "Launcher root manifest request failed:" << reply->errorString();
        reply->deleteLater();
        SetLauncherUpdateFailure("Could not reach the launcher update service");
        return;
    }

    m_rootManifestData = reply->readAll();
    reply->deleteLater();
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(m_rootManifestData, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
    {
        SetLauncherUpdateFailure("The root update manifest is invalid");
        return;
    }

    QString manifestPath = document.object().value("launcherManifest").toString();
    QUrl relativeUrl(manifestPath);
    QStringList pathParts = manifestPath.split('/', Qt::SkipEmptyParts);
    if (manifestPath.isEmpty() || QDir::isAbsolutePath(manifestPath) || manifestPath.contains('\\') || pathParts.contains("..") ||
        !relativeUrl.isRelative() || relativeUrl.hasQuery() || relativeUrl.hasFragment() || relativeUrl.path() != manifestPath ||
        QFileInfo(manifestPath).fileName().isEmpty())
    {
        SetLauncherUpdateFailure("The launcher manifest path is invalid");
        return;
    }

    m_launcherManifestDirectory = QFileInfo(manifestPath).path();
    QNetworkRequest request{QUrl("https://pub-0174d9cb4ae44e93967c2a673e1db201.r2.dev/" + manifestPath)};
    m_launcherReply = m_networkAccessManager->get(request);
    connect(m_launcherReply, &QNetworkReply::finished, this, &LauncherState::LauncherManifestFinished);
}

void LauncherState::LauncherManifestFinished()
{
    QNetworkReply *reply = m_launcherReply;
    m_launcherReply      = nullptr;
    if (reply->error() != QNetworkReply::NoError)
    {
        qWarning() << "Launcher manifest request failed:" << reply->errorString();
        reply->deleteLater();
        SetLauncherUpdateFailure("Could not load the launcher manifest");
        return;
    }

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &parseError);
    reply->deleteLater();
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
    {
        SetLauncherUpdateFailure("The launcher manifest is invalid");
        return;
    }

    QJsonObject manifest = document.object();
    QString latest       = manifest.value("latest").toString();
    QStringList versions;
    QJsonArray versionValues = manifest.value("versions").toArray();
    for (const QJsonValue &value : versionValues)
    {
        QString version = value.toString();
        if (!version.isEmpty() && QFileInfo(version).fileName() == version && !versions.contains(version))
        {
            versions.append(version);
        }
    }

    qsizetype latestSuffix;
    qsizetype currentSuffix;
    QVersionNumber latestVersion  = QVersionNumber::fromString(latest, &latestSuffix);
    QVersionNumber currentVersion = QVersionNumber::fromString(EVERSTEAD_LAUNCHER_VERSION, &currentSuffix);
    if (latestVersion.isNull() || latestSuffix != latest.size() || currentVersion.isNull() || currentSuffix != GetLauncherVersion().size() ||
        !versions.contains(latest))
    {
        SetLauncherUpdateFailure("The launcher manifest has no valid latest version");
        return;
    }

    if (QVersionNumber::compare(latestVersion, currentVersion) <= 0)
    {
        qInfo() << "Launcher is up to date at" << GetLauncherVersion();
        m_launcherUpdateActive = false;
        m_launcherUpdateStatus = "Launcher is up to date";
        if (!m_profilesLoaded)
        {
            LoadProfiles();
            m_profilesLoaded = true;
            emit ProfilesChanged();
        }

        RefreshChangelog();
        ApplyRootManifest(m_rootManifestData);
        return;
    }

    m_launcherUpdateVersion = latest;
    m_launcherUpdateStatus  = "Preparing launcher update " + latest;
    QNetworkRequest request{
            QUrl("https://pub-0174d9cb4ae44e93967c2a673e1db201.r2.dev/" + m_launcherManifestDirectory + "/versions/" + latest + "/manifest.json")};
    m_launcherReply = m_networkAccessManager->get(request);
    connect(m_launcherReply, &QNetworkReply::finished, this, &LauncherState::LauncherVersionManifestFinished);
    emit Changed();
}

void LauncherState::LauncherVersionManifestFinished()
{
    QNetworkReply *reply = m_launcherReply;
    m_launcherReply      = nullptr;
    if (reply->error() != QNetworkReply::NoError)
    {
        qWarning() << "Launcher version manifest request failed:" << reply->errorString();
        reply->deleteLater();
        SetLauncherUpdateFailure("Could not load the launcher version manifest");
        return;
    }

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &parseError);
    reply->deleteLater();
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
    {
        SetLauncherUpdateFailure("The launcher version manifest is invalid");
        return;
    }

    QJsonObject manifest              = document.object();
    QJsonObject release               = manifest.value("releases").toObject().value(m_platform).toObject();
    m_launcherPackage.file            = release.value("file").toString();
    m_launcherPackage.sha256          = release.value("sha256").toString();
    m_launcherPackage.size            = release.value("size").toInteger(-1);
    QRegularExpressionMatch hashMatch = QRegularExpression("^[0-9a-fA-F]{64}$").match(m_launcherPackage.sha256);
    if (manifest.value("version").toString() != m_launcherUpdateVersion || m_launcherPackage.file.isEmpty() ||
        QFileInfo(m_launcherPackage.file).fileName() != m_launcherPackage.file || GetLauncherExecutableName().isEmpty() || !hashMatch.hasMatch() ||
        m_launcherPackage.size <= 0)
    {
        SetLauncherUpdateFailure("This launcher update is unavailable on " + m_platform);
        return;
    }

    StartLauncherDownload();
}

void LauncherState::LauncherDownloadReadyRead()
{
    m_launcherDownloadFile->write(m_launcherDownloadReply->readAll());
}

void LauncherState::LauncherDownloadProgress(qint64 received, qint64 total)
{
    qint64 size = total > 0 ? total : m_launcherPackage.size;
    if (size > 0)
    {
        m_launcherUpdateProgress = (double) received / (double) size;
    }

    emit Changed();
}

void LauncherState::LauncherDownloadFinished()
{
    m_launcherDownloadFile->write(m_launcherDownloadReply->readAll());
    m_launcherDownloadFile->close();
    QString archivePath = m_launcherDownloadFile->fileName();
    m_launcherDownloadFile->deleteLater();
    m_launcherDownloadFile  = nullptr;
    QNetworkReply *reply    = m_launcherDownloadReply;
    m_launcherDownloadReply = nullptr;
    if (reply->error() != QNetworkReply::NoError)
    {
        qWarning() << "Launcher download failed:" << reply->errorString();
        reply->deleteLater();
        SetLauncherUpdateFailure("Could not download the launcher update");
        return;
    }

    reply->deleteLater();
    if (!VerifyFile(archivePath, m_launcherPackage))
    {
        qWarning() << "Launcher package did not match its manifest:" << archivePath;
        QFile::remove(archivePath);
        SetLauncherUpdateFailure("The launcher update did not pass verification");
        return;
    }

    m_launcherUpdateStatus = "Installing launcher update " + m_launcherUpdateVersion;
    emit Changed();
    QString packageDirectory = GetLauncherUpdateDirectory() + "/package";
    if (!ExtractArchive(archivePath, packageDirectory))
    {
        SetLauncherUpdateFailure("Could not unpack the launcher update");
        return;
    }

    QFile::remove(archivePath);
    StartLauncherInstaller();
}

void LauncherState::StartLauncherDownload()
{
    QDir updateDirectory(GetLauncherUpdateDirectory());
    if (updateDirectory.exists() && !updateDirectory.removeRecursively())
    {
        qWarning() << "Could not clear launcher update directory" << updateDirectory.absolutePath();
        SetLauncherUpdateFailure("Could not create the launcher update files");
        return;
    }

    if (!QDir().mkpath(updateDirectory.absolutePath()))
    {
        qWarning() << "Could not create launcher update directory" << updateDirectory.absolutePath();
        SetLauncherUpdateFailure("Could not create the launcher update files");
        return;
    }

    QFile marker(updateDirectory.filePath("applying.pid"));
    if (!marker.open(QIODevice::WriteOnly | QIODevice::Truncate) || marker.write(QByteArray::number(QCoreApplication::applicationPid())) <= 0)
    {
        qWarning() << "Could not create launcher update marker" << marker.fileName() << marker.errorString();
        SetLauncherUpdateFailure("Could not create the launcher update files");
        return;
    }
    marker.close();

    QString archivePath    = updateDirectory.filePath(m_launcherPackage.file);
    m_launcherDownloadFile = new QFile(archivePath, this);
    if (!m_launcherDownloadFile->open(QIODevice::WriteOnly))
    {
        qWarning() << "Could not create launcher update archive" << archivePath << m_launcherDownloadFile->errorString();
        m_launcherDownloadFile->deleteLater();
        m_launcherDownloadFile = nullptr;
        SetLauncherUpdateFailure("Could not create the launcher update files");
        return;
    }

    m_launcherUpdateProgress = 0.0;
    m_launcherUpdateStatus   = "Downloading launcher update " + m_launcherUpdateVersion;
    qInfo() << "Downloading launcher update" << m_launcherUpdateVersion;
    QNetworkRequest request{QUrl("https://pub-0174d9cb4ae44e93967c2a673e1db201.r2.dev/" + m_launcherManifestDirectory + "/versions/" +
                                 m_launcherUpdateVersion + "/" + m_launcherPackage.file)};
    m_launcherDownloadReply = m_networkAccessManager->get(request);
    connect(m_launcherDownloadReply, &QNetworkReply::readyRead, this, &LauncherState::LauncherDownloadReadyRead);
    connect(m_launcherDownloadReply, &QNetworkReply::downloadProgress, this, &LauncherState::LauncherDownloadProgress);
    connect(m_launcherDownloadReply, &QNetworkReply::finished, this, &LauncherState::LauncherDownloadFinished);
    emit Changed();
}

void LauncherState::StartLauncherInstaller()
{
    QString executableName = GetLauncherExecutableName();
    QString executablePath;
    QDirIterator iterator(GetLauncherUpdateDirectory() + "/package", QStringList(executableName), QDir::Files, QDirIterator::Subdirectories);
    while (iterator.hasNext())
    {
        QString path = iterator.next();
        if (!executablePath.isEmpty())
        {
            SetLauncherUpdateFailure("The launcher package contains multiple executables");
            return;
        }

        executablePath = path;
    }

    if (executablePath.isEmpty())
    {
        SetLauncherUpdateFailure("The launcher package does not contain the expected executable");
        return;
    }

#if defined(Q_OS_LINUX)
    QFile executable(executablePath);
    if (!executable.setPermissions(executable.permissions() | QFileDevice::ExeOwner | QFileDevice::ExeGroup | QFileDevice::ExeOther))
    {
        SetLauncherUpdateFailure("Could not prepare the launcher executable");
        return;
    }
#endif

    QString sourceDirectory = QFileInfo(executablePath).dir().absolutePath();
    QStringList arguments{"--apply-launcher-update", sourceDirectory, QCoreApplication::applicationDirPath(), executableName,
                          GetLauncherUpdateDirectory()};
    m_launcherUpdateProgress = 1.0;
    m_launcherUpdateStatus   = "Restarting launcher";
    emit Changed();
    qint64 updaterPid = 0;
    if (!QProcess::startDetached(executablePath, arguments, sourceDirectory, &updaterPid))
    {
        SetLauncherUpdateFailure("Could not start the launcher updater");
        return;
    }

    QFile marker(GetLauncherUpdateDirectory() + "/applying.pid");
    if (!marker.open(QIODevice::WriteOnly | QIODevice::Truncate) || marker.write(QByteArray::number(updaterPid)) <= 0)
    {
        qWarning() << "Could not transfer launcher update marker" << marker.fileName() << marker.errorString();
    }

    qInfo() << "Launcher updater started for" << m_launcherUpdateVersion;
    QCoreApplication::quit();
}

void LauncherState::ChangelogFinished()
{
    QNetworkReply *reply = m_changelogReply;
    m_changelogReply     = nullptr;
    m_changelogLoading   = false;
    if (reply->error() != QNetworkReply::NoError)
    {
        qWarning() << "Changelog request failed:" << reply->errorString();
        m_changelogError = "Could not load the changelog";
        reply->deleteLater();
        emit ChangelogChanged();
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();
    if (data.isEmpty() || data.size() > 1024 * 1024)
    {
        m_changelogError = "The changelog is invalid";
        qWarning() << m_changelogError;
        emit ChangelogChanged();
        return;
    }

    // Qt has no default spacing for markdown, so we apply GitHub's block margins
    QTextDocument document;
    document.setMarkdown(QString::fromUtf8(data));
    for (QTextBlock block = document.begin(); block.isValid(); block = block.next())
    {
        QTextBlockFormat format = block.blockFormat();
        QTextList *list         = block.textList();
        format.setTopMargin(block.blockNumber() > 0 && format.headingLevel() > 0 ? 24 : 0);
        if (list && block.next().isValid() && block.next().textList() == list)
        {
            format.setBottomMargin(4);
        }
        else
        {
            format.setBottomMargin(16);
        }

        QTextCursor(block).setBlockFormat(format);
    }

    m_changelog      = document.toHtml();
    m_changelogError = {};
    qInfo() << "Loaded changelog";
    emit ChangelogChanged();
}

void LauncherState::LoadProfiles()
{
    QFile input(GetProfilesPath());
    if (!input.open(QIODevice::ReadOnly))
    {
        return;
    }

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(input.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isArray())
    {
        qWarning() << "Profile file is invalid";
        return;
    }

    bool profilesChanged  = false;
    QVariantList profiles = document.toVariant().toList();
    for (const QVariant &profileValue : profiles)
    {
        QVariantMap profile = profileValue.toMap();
        if (!profile.value("name").toString().isEmpty() && !profile.value("version").toString().isEmpty())
        {
            if (!profile.contains("showLogs"))
            {
                profile.insert("showLogs", false);
                profilesChanged = true;
            }

            QVariantList environment = profile.value("environment").toList();
            for (int i = 0; i < environment.size(); i++)
            {
                QVariantMap variable = environment[i].toMap();
                if (variable.value("id").toString().isEmpty())
                {
                    variable.insert("id", QUuid::createUuid().toString(QUuid::WithoutBraces));
                    environment[i]  = variable;
                    profilesChanged = true;
                }
            }

            profile.insert("environment", environment);
            m_profiles.append(profile);
        }
    }

    if (profilesChanged)
    {
        SaveProfiles();
    }
}

void LauncherState::SaveProfiles() const
{
    QString path = GetProfilesPath();
    QDir().mkpath(QFileInfo(path).dir().absolutePath());
    QFile output(path);
    if (!output.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        qWarning() << "Could not save profiles";
        return;
    }

    output.write(QJsonDocument::fromVariant(m_profiles).toJson(QJsonDocument::Indented));
}

int LauncherState::GetProfileVariableIndex(int profileIndex, const QString &variableId) const
{
    if (profileIndex < 0 || profileIndex >= m_profiles.size())
    {
        return -1;
    }

    QVariantList environment = m_profiles[profileIndex].toMap().value("environment").toList();
    for (int i = 0; i < environment.size(); i++)
    {
        if (environment[i].toMap().value("id").toString() == variableId)
        {
            return i;
        }
    }

    return -1;
}

#if defined(Q_OS_LINUX)
bool LauncherState::DecompressZstd(const QString &inputPath, const QString &outputPath) const
{
    QFile input(inputPath);
    QFile output(outputPath);
    if (!input.open(QIODevice::ReadOnly) || !output.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        qWarning() << "Could not open" << inputPath << "or" << outputPath;
        return false;
    }

    ZSTD_DStream *stream = ZSTD_createDStream();
    std::vector<char> inputBuffer(ZSTD_DStreamInSize());
    std::vector<char> outputBuffer(ZSTD_DStreamOutSize());
    size_t result = 0;
    while (!input.atEnd())
    {
        qint64 read          = input.read(inputBuffer.data(), (qint64) inputBuffer.size());
        ZSTD_inBuffer source = {inputBuffer.data(), (size_t) read, 0};
        while (source.pos < source.size)
        {
            ZSTD_outBuffer target = {outputBuffer.data(), outputBuffer.size(), 0};
            result                = ZSTD_decompressStream(stream, &target, &source);
            if (ZSTD_isError(result) || output.write(outputBuffer.data(), (qint64) target.pos) != (qint64) target.pos)
            {
                qWarning() << "Could not decompress" << inputPath << (ZSTD_isError(result) ? ZSTD_getErrorName(result) : "write failed");
                ZSTD_freeDStream(stream);
                return false;
            }
        }
    }

    ZSTD_freeDStream(stream);
    return result == 0;
}
#endif

bool LauncherState::ExtractArchive(const QString &archivePath, const QString &outputDirectory) const
{
#if defined(Q_OS_LINUX)
    // System libarchive builds are not guaranteed to read zstd, so we unpack that layer ourselves
    if (archivePath.endsWith(".zst", Qt::CaseInsensitive))
    {
        QString tarPath = archivePath.chopped(4);
        bool extracted  = DecompressZstd(archivePath, tarPath) && ExtractArchive(tarPath, outputDirectory);
        QFile::remove(tarPath);
        return extracted;
    }
#endif

    struct archive *reader = archive_read_new();
    struct archive *writer = archive_write_disk_new();
    archive_read_support_filter_all(reader);
    archive_read_support_format_zip(reader);
    archive_read_support_format_tar(reader);
    archive_write_disk_set_options(writer,
                                   ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_SECURE_NODOTDOT | ARCHIVE_EXTRACT_SECURE_SYMLINKS);
    if (!QDir().mkpath(outputDirectory) || archive_read_open_filename(reader, archivePath.toUtf8().constData(), 10240) != ARCHIVE_OK)
    {
        archive_write_free(writer);
        archive_read_free(reader);
        return false;
    }

    // The secure symlink check also refuses symlinks above the output folder, like /home on Fedora Atomic, so we write to the resolved path
    QDir output(QDir(outputDirectory).canonicalPath());
    struct archive_entry *entry;
    int result = ARCHIVE_OK;
    while ((result = archive_read_next_header(reader, &entry)) == ARCHIVE_OK)
    {
        QString entryPath = QString::fromUtf8(archive_entry_pathname(entry));
        if (entryPath.isEmpty() || QDir::isAbsolutePath(entryPath) || entryPath.contains('\\') || entryPath.split('/').contains("..") ||
            archive_entry_symlink(entry) != nullptr)
        {
            archive_write_free(writer);
            archive_read_free(reader);
            return false;
        }

        int type = archive_entry_filetype(entry);
        if (type != AE_IFREG && type != AE_IFDIR)
        {
            archive_write_free(writer);
            archive_read_free(reader);
            return false;
        }

        QString outputPath = output.filePath(entryPath);
        archive_entry_set_pathname(entry, outputPath.toUtf8().constData());
        if (archive_write_header(writer, entry) != ARCHIVE_OK)
        {
            qWarning() << "Could not write" << outputPath << archive_error_string(writer);
            archive_write_free(writer);
            archive_read_free(reader);
            return false;
        }

        char buffer[16384];
        la_ssize_t bytes = 0;
        while ((bytes = archive_read_data(reader, buffer, sizeof(buffer))) > 0)
        {
            if (archive_write_data(writer, buffer, bytes) != bytes)
            {
                archive_write_free(writer);
                archive_read_free(reader);
                return false;
            }
        }

        if (bytes < 0 || archive_write_finish_entry(writer) != ARCHIVE_OK)
        {
            archive_write_free(writer);
            archive_read_free(reader);
            return false;
        }
    }

    if (result != ARCHIVE_EOF)
    {
        qWarning() << "Could not read" << archivePath << archive_error_string(reader);
    }

    archive_write_free(writer);
    archive_read_free(reader);
    return result == ARCHIVE_EOF;
}

bool LauncherState::VerifyFile(const QString &path, const DownloadFile &file) const
{
    QFile input(path);
    if (!input.open(QIODevice::ReadOnly) || input.size() != file.size)
    {
        return false;
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!input.atEnd())
    {
        hash.addData(input.read(1024 * 1024));
    }

    return QString::fromLatin1(hash.result().toHex()).compare(file.sha256, Qt::CaseInsensitive) == 0;
}

QString LauncherState::GetVersionsDirectory() const
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/Everstead/versions";
}

QString LauncherState::GetInstallDirectory() const
{
    return GetVersionsDirectory() + "/" + m_selectedVersion;
}

QString LauncherState::GetStagingDirectory() const
{
    return GetInstallDirectory() + ".installing";
}

QString LauncherState::GetProfilesPath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/profiles.json";
}

QString LauncherState::GetLauncherUpdateDirectory() const
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/launcher-updates/" + m_launcherUpdateVersion;
}

QString LauncherState::GetLauncherExecutableName() const
{
    QString name = m_launcherPackage.file;
#if defined(Q_OS_WIN)
    if (!name.endsWith(".zip", Qt::CaseInsensitive))
    {
        return {};
    }

    name.chop(4);
    return name + ".exe";
#elif defined(Q_OS_LINUX)
    if (!name.endsWith(".tar.zst", Qt::CaseInsensitive))
    {
        return {};
    }

    name.chop(8);
    return name;
#else
    return {};
#endif
}

void LauncherState::SetFailure(const QString &message)
{
    m_loading                = false;
    m_platformAvailable      = false;
    m_progress               = 0.0;
    m_launchAfterLoad        = false;
    m_launchAfterInstall     = false;
    m_installAfterLoad       = false;
    m_reinstallingExecutable = false;
    m_error                  = message;
    m_status                 = "Action failed";
    qWarning() << message;
    emit Changed();
}

void LauncherState::SetLauncherUpdateFailure(const QString &message)
{
    m_launcherUpdateProgress = 0.0;
    m_launcherUpdateError    = message;
    m_launcherUpdateStatus   = "Launcher update failed";
    qWarning() << message;
    emit Changed();
}
