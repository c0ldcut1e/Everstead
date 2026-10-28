#pragma once

#include <QAbstractItemModel>
#include <QByteArray>
#include <QObject>
#include <QString>
#include <QStringConverter>
#include <QStringList>
#include <QVariantList>
#include <QtTypes>

class QFile;
class GameLogModel;
class QNetworkAccessManager;
class QNetworkReply;
class QProcess;
class QSocketNotifier;
class QTimer;

class LauncherState : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString latestVersion READ GetLatestVersion NOTIFY Changed)
    Q_PROPERTY(QString launcherVersion READ GetLauncherVersion CONSTANT)
    Q_PROPERTY(QString selectedVersion READ GetSelectedVersion NOTIFY Changed)
    Q_PROPERTY(QStringList versions READ GetVersions NOTIFY Changed)
    Q_PROPERTY(QStringList installedVersions READ GetInstalledVersions NOTIFY Changed)
    Q_PROPERTY(QString platform READ GetPlatform NOTIFY Changed)
    Q_PROPERTY(QString status READ GetStatus NOTIFY Changed)
    Q_PROPERTY(QString error READ GetError NOTIFY Changed)
    Q_PROPERTY(QString actionText READ GetActionText NOTIFY Changed)
    Q_PROPERTY(double progress READ GetProgress NOTIFY Changed)
    Q_PROPERTY(bool loading READ IsLoading NOTIFY Changed)
    Q_PROPERTY(bool platformAvailable READ IsPlatformAvailable NOTIFY Changed)
    Q_PROPERTY(bool installed READ IsInstalled NOTIFY Changed)
    Q_PROPERTY(bool gameRunning READ IsGameRunning NOTIFY Changed)
    Q_PROPERTY(bool launcherUpdateActive READ IsLauncherUpdateActive NOTIFY Changed)
    Q_PROPERTY(QString launcherUpdateStatus READ GetLauncherUpdateStatus NOTIFY Changed)
    Q_PROPERTY(QString launcherUpdateError READ GetLauncherUpdateError NOTIFY Changed)
    Q_PROPERTY(double launcherUpdateProgress READ GetLauncherUpdateProgress NOTIFY Changed)
    Q_PROPERTY(QVariantList profiles READ GetProfiles NOTIFY ProfilesChanged)
    Q_PROPERTY(int activeProfile READ GetActiveProfile NOTIFY Changed)
    Q_PROPERTY(QAbstractItemModel *gameLogModel READ GetGameLogModel CONSTANT)
    Q_PROPERTY(bool logWindowVisible READ IsLogWindowVisible WRITE SetLogWindowVisible NOTIFY LogWindowVisibleChanged)
    Q_PROPERTY(QString theme READ GetTheme WRITE SetTheme NOTIFY SettingsChanged)
    Q_PROPERTY(bool minimizeWhilePlaying READ IsMinimizeWhilePlaying WRITE SetMinimizeWhilePlaying NOTIFY SettingsChanged)
    Q_PROPERTY(QString changelog READ GetChangelog NOTIFY ChangelogChanged)
    Q_PROPERTY(QString changelogError READ GetChangelogError NOTIFY ChangelogChanged)
    Q_PROPERTY(bool changelogLoading READ IsChangelogLoading NOTIFY ChangelogChanged)

public:
    LauncherState(QObject *parent = nullptr);

    QString GetLatestVersion() const;
    QString GetLauncherVersion() const;
    QString GetSelectedVersion() const;
    QStringList GetVersions() const;
    QStringList GetInstalledVersions() const;
    QString GetPlatform() const;
    QString GetStatus() const;
    QString GetError() const;
    QString GetActionText() const;
    double GetProgress() const;
    bool IsLoading() const;
    bool IsPlatformAvailable() const;
    bool IsInstalled() const;
    bool IsGameRunning() const;
    bool IsLauncherUpdateActive() const;
    QString GetLauncherUpdateStatus() const;
    QString GetLauncherUpdateError() const;
    double GetLauncherUpdateProgress() const;
    QVariantList GetProfiles() const;
    int GetActiveProfile() const;
    QAbstractItemModel *GetGameLogModel() const;
    bool IsLogWindowVisible() const;
    void SetLogWindowVisible(bool visible);
    QString GetTheme() const;
    void SetTheme(const QString &theme);
    bool IsMinimizeWhilePlaying() const;
    void SetMinimizeWhilePlaying(bool minimize);
    QString GetChangelog() const;
    QString GetChangelogError() const;
    bool IsChangelogLoading() const;

    Q_INVOKABLE void CheckForLauncherUpdates();
    Q_INVOKABLE void RefreshChangelog();
    Q_INVOKABLE void Refresh();
    Q_INVOKABLE void SelectVersion(const QString &version);
    Q_INVOKABLE void InstallVersion(const QString &version);
    Q_INVOKABLE void RemoveVersion(const QString &version);
    Q_INVOKABLE void Play();
    Q_INVOKABLE void CreateProfile();
    Q_INVOKABLE void DeleteProfile(int index);
    Q_INVOKABLE void SaveProfile(int index, const QString &name, const QVariantList &environment);
    Q_INVOKABLE void SetProfileVersion(int index, const QString &version);
    Q_INVOKABLE void SetProfileShowLogs(int index, bool showLogs);
    Q_INVOKABLE void AddProfileVariable(int index);
    Q_INVOKABLE void RemoveProfileVariable(int profileIndex, const QString &variableId);
    Q_INVOKABLE void PlayProfile(int index);
    Q_INVOKABLE void OpenDataDirectory();

signals:
    void Changed();
    void ProfilesChanged();
    void GameLogChanged();
    void LogWindowVisibleChanged();
    void ChangelogChanged();
    void SettingsChanged();

private:
    struct DownloadFile
    {
        QString file;
        QString sha256;
        qint64 size;
    };

    void LoadVersion(const QString &version);
    void StartInstall(bool launch);
    void UpdateInstalledVersions();
    void ApplyRootManifest(const QByteArray &data);
    void RootManifestFinished();
    void VersionManifestFinished();
    void DownloadReadyRead();
    void DownloadProgress(qint64 received, qint64 total);
    void DownloadFinished();
    void GameReadyRead();
    void AppendGameLogOutput(const QByteArray &output);
#if defined(Q_OS_LINUX)
    bool OpenGamePty();
    void GamePtyReadyRead();
    void CloseGamePty();
    void InstallDesktopEntry(const QString &name, const QString &title, const QString &iconPath, bool hidden) const;
#endif
    void GameFinished();
    void GameError();
    void ReinstallExecutable();
    void StartDownload(const DownloadFile &file);
    void LauncherRootManifestFinished();
    void LauncherManifestFinished();
    void LauncherVersionManifestFinished();
    void LauncherDownloadReadyRead();
    void LauncherDownloadProgress(qint64 received, qint64 total);
    void LauncherDownloadFinished();
    void StartLauncherDownload();
    void StartLauncherInstaller();
    void ChangelogFinished();
    void LoadProfiles();
    void SaveProfiles() const;
    int GetProfileVariableIndex(int profileIndex, const QString &variableId) const;
#if defined(Q_OS_LINUX)
    bool DecompressZstd(const QString &inputPath, const QString &outputPath) const;
#endif
    bool ExtractArchive(const QString &archivePath, const QString &outputDirectory) const;
    bool VerifyFile(const QString &path, const DownloadFile &file) const;
    QString GetVersionsDirectory() const;
    QString GetInstallDirectory() const;
    QString GetStagingDirectory() const;
    QString GetProfilesPath() const;
    QString GetLauncherUpdateDirectory() const;
    QString GetLauncherExecutableName() const;
    void SetFailure(const QString &message);
    void SetLauncherUpdateFailure(const QString &message);

    QNetworkAccessManager *m_networkAccessManager;
    QNetworkReply *m_manifestReply;
    QNetworkReply *m_downloadReply;
    QFile *m_downloadFile;
    QNetworkReply *m_launcherReply;
    QNetworkReply *m_launcherDownloadReply;
    QFile *m_launcherDownloadFile;
    QNetworkReply *m_changelogReply;
    QProcess *m_gameProcess;
    GameLogModel *m_gameLogModel;
    QTimer *m_gameLogUpdateTimer;
#if defined(Q_OS_LINUX)
    QSocketNotifier *m_gamePtyNotifier;
    int m_gamePtyMaster;
    int m_gamePtySlave;
#endif
    DownloadFile m_gamePackage;
    DownloadFile m_launcherPackage;
    QByteArray m_rootManifestData;
    QString m_latestVersion;
    QString m_selectedVersion;
    QString m_gameExecutable;
    QStringList m_versions;
    QStringList m_installedVersions;
    QString m_platform;
    QString m_status;
    QString m_error;
    QStringDecoder m_gameLogDecoder;
    QString m_launcherManifestDirectory;
    QString m_launcherUpdateVersion;
    QString m_launcherUpdateStatus;
    QString m_launcherUpdateError;
    QString m_theme;
    QString m_changelog;
    QString m_changelogError;
    QVariantList m_profiles;
    qint64 m_downloadedBytes;
    qint64 m_downloadSize;
    double m_progress;
    double m_launcherUpdateProgress;
    bool m_reinstallingExecutable;
    bool m_loading;
    bool m_platformAvailable;
    bool m_installed;
    bool m_gameRunning;
    bool m_launcherUpdateActive;
    bool m_profilesLoaded;
    bool m_changelogLoading;
    bool m_launchAfterLoad;
    bool m_launchAfterInstall;
    bool m_installAfterLoad;
    bool m_captureGameLogs;
    bool m_logWindowVisible;
    bool m_minimizeWhilePlaying;
    int m_activeProfile;

    static const QStringList s_themes;
};
