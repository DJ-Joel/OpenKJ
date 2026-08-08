#include "ytdlpdownloader.h"
#include <QFileInfo>
#include <QStringList>
#include <QStandardPaths>
#include <QDir>
#include <QUuid>
#include <QFile>
#include <QTextStream>

YtDlpDownloader::YtDlpDownloader(QObject *parent) : QObject(parent)
{
    m_logger = spdlog::get("logger");
}

YtDlpDownloader::~YtDlpDownloader()
{
    cancel();
}

bool YtDlpDownloader::isRunning() const
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

void YtDlpDownloader::cancel()
{
    if (m_process && m_process->state() != QProcess::NotRunning)
    {
        m_logger->info("{} Cancelling in-progress download", m_loggingPrefix);
        m_cancelled = true;
        m_process->kill();
        m_process->waitForFinished(3000);
    }
    m_process.reset();
}

void YtDlpDownloader::download(const QString &ytDlpPath, const QString &url, const QString &outputPathNoExt)
{
    if (isRunning())
    {
        m_logger->warn("{} download() called while a download is already running, ignoring", m_loggingPrefix);
        return;
    }

    if (ytDlpPath.trimmed().isEmpty() || !QFileInfo::exists(ytDlpPath))
    {
        m_logger->error("{} yt-dlp path is not configured or does not exist: {}", m_loggingPrefix, ytDlpPath.toStdString());
        emit failed("yt-dlp path is not configured or does not exist. Set it in Settings -> External.");
        return;
    }

    QDir outputDir = QFileInfo(outputPathNoExt).dir();
    if (!outputDir.exists())
    {
        m_logger->error("{} Download folder does not exist: {}", m_loggingPrefix, outputDir.absolutePath().toStdString());
        emit failed("The configured download folder does not exist. Set one in Settings -> External.");
        return;
    }

    // Dedicated temp files for the two --print-to-file values, kept
    // deliberately separate from stdout - see the class comment for why.
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString uid = QUuid::createUuid().toString(QUuid::Id128);
    m_filePathTempFile = QDir(tempDir).filePath("okj-dl-path-" + uid + ".txt");
    m_durationTempFile = QDir(tempDir).filePath("okj-dl-duration-" + uid + ".txt");
    QFile::remove(m_filePathTempFile);
    QFile::remove(m_durationTempFile);

    m_cancelled = false;
    m_process = std::make_unique<QProcess>();
    connect(m_process.get(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &YtDlpDownloader::processFinished);
    connect(m_process.get(), &QProcess::errorOccurred, this, &YtDlpDownloader::processErrorOccurred);
    connect(m_process.get(), &QProcess::readyReadStandardOutput, this, &YtDlpDownloader::readyReadStandardOutput);

    QStringList args {
        "--no-playlist",
        "-f", "best[ext=mp4]/best",
        "-o", outputPathNoExt + ".%(ext)s",
        "--print-to-file", "after_move:filepath", m_filePathTempFile,
        "--print-to-file", "after_move:%(duration)s", m_durationTempFile,
        url
    };
    m_logger->info("{} Starting download via yt-dlp: {}", m_loggingPrefix, url.toStdString());
    m_process->start(ytDlpPath, args);
}

void YtDlpDownloader::readyReadStandardOutput()
{
    if (!m_process || m_cancelled)
        return;
    QString output = QString::fromUtf8(m_process->readAllStandardOutput());
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines)
    {
        QString trimmed = line.trimmed();
        if (!trimmed.isEmpty())
            emit progress(trimmed);
    }
}

void YtDlpDownloader::processErrorOccurred(QProcess::ProcessError error)
{
    Q_UNUSED(error)
    if (m_cancelled)
        return;
    QString errStr = m_process ? m_process->errorString() : QString("unknown error");
    m_logger->error("{} yt-dlp process error: {}", m_loggingPrefix, errStr.toStdString());
    emit failed("Failed to launch yt-dlp: " + errStr);
}

void YtDlpDownloader::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (!m_process)
        return;

    if (m_cancelled)
    {
        m_logger->info("{} yt-dlp process finished after being cancelled - not reporting as a failure", m_loggingPrefix);
        return;
    }

    QString stdErr = QString::fromUtf8(m_process->readAllStandardError());

    if (exitStatus != QProcess::NormalExit || exitCode != 0)
    {
        m_logger->error("{} yt-dlp exited with code {}: {}", m_loggingPrefix, exitCode, stdErr.toStdString());
        emit failed(stdErr.trimmed().isEmpty() ? QString("yt-dlp exited with code %1").arg(exitCode) : stdErr.trimmed());
        return;
    }

    if (!QFile::exists(m_filePathTempFile))
    {
        m_logger->error("{} yt-dlp exited successfully but reported no final file path", m_loggingPrefix);
        emit failed("Download finished but yt-dlp didn't report where the file was saved.");
        return;
    }

    QFile filePathFile(m_filePathTempFile);
    QString finalFilePath;
    if (filePathFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        finalFilePath = QString::fromUtf8(filePathFile.readAll()).trimmed();
        // yt-dlp reports the path using Windows' native backslashes. Qt's
        // own file scanning (DbUpdater, via QDirIterator) consistently uses
        // forward slashes internally regardless of OS. Left unnormalized,
        // the same physical file ends up recorded under two textually
        // different path strings - one from here, one from a later
        // DirectoryMonitor rescan of the same folder - and the database's
        // UNIQUE constraint on path never catches it as a duplicate, since
        // it compares strings literally, not filesystem-aware.
        finalFilePath = QDir::fromNativeSeparators(finalFilePath);
        filePathFile.close();
    }
    QFile::remove(m_filePathTempFile);

    int durationSecs = 0;
    if (QFile::exists(m_durationTempFile))
    {
        QFile durationFile(m_durationTempFile);
        if (durationFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QString durationStr = QString::fromUtf8(durationFile.readAll()).trimmed();
            bool ok = false;
            double durationDouble = durationStr.toDouble(&ok);
            if (ok)
                durationSecs = static_cast<int>(durationDouble);
            durationFile.close();
        }
        QFile::remove(m_durationTempFile);
    }

    if (finalFilePath.isEmpty() || !QFile::exists(finalFilePath))
    {
        m_logger->error("{} Reported final file path doesn't exist: {}", m_loggingPrefix, finalFilePath.toStdString());
        emit failed("Download finished but the resulting file couldn't be found.");
        return;
    }

    m_logger->info("{} Download finished: {}", m_loggingPrefix, finalFilePath.toStdString());
    emit finished(finalFilePath, durationSecs);
}
