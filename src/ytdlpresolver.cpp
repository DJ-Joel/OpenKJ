#include "ytdlpresolver.h"
#include <QFileInfo>
#include <QStringList>

YtDlpResolver::YtDlpResolver(QObject *parent) : QObject(parent)
{
    m_logger = spdlog::get("logger");
}

YtDlpResolver::~YtDlpResolver()
{
    cancel();
}

bool YtDlpResolver::isRunning() const
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

void YtDlpResolver::cancel()
{
    if (m_process && m_process->state() != QProcess::NotRunning)
    {
        m_logger->info("{} Cancelling in-progress yt-dlp resolve", m_loggingPrefix);
        m_cancelled = true;
        m_process->kill();
        m_process->waitForFinished(3000);
    }
    m_process.reset();
}

void YtDlpResolver::resolve(const QString &ytDlpPath, const QString &url)
{
    if (isRunning())
    {
        m_logger->warn("{} resolve() called while a resolve is already running, ignoring", m_loggingPrefix);
        return;
    }

    if (ytDlpPath.trimmed().isEmpty() || !QFileInfo::exists(ytDlpPath))
    {
        m_logger->error("{} yt-dlp path is not configured or does not exist: {}", m_loggingPrefix, ytDlpPath.toStdString());
        emit failed("yt-dlp path is not configured or does not exist. Set it in Settings -> External.");
        return;
    }

    m_cancelled = false;
    m_process = std::make_unique<QProcess>();
    connect(m_process.get(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &YtDlpResolver::processFinished);
    connect(m_process.get(), &QProcess::errorOccurred, this, &YtDlpResolver::processErrorOccurred);

    QStringList args { "--no-playlist", "-f", "best[ext=mp4]/best", "-g", url };
    m_logger->info("{} Resolving stream URL via yt-dlp: {}", m_loggingPrefix, url.toStdString());
    m_process->start(ytDlpPath, args);
}

void YtDlpResolver::processErrorOccurred(QProcess::ProcessError error)
{
    Q_UNUSED(error)
    if (m_cancelled)
        return;
    QString errStr = m_process ? m_process->errorString() : QString("unknown error");
    m_logger->error("{} yt-dlp process error: {}", m_loggingPrefix, errStr.toStdString());
    emit failed("Failed to launch yt-dlp: " + errStr);
}

void YtDlpResolver::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (!m_process)
        return;

    if (m_cancelled)
    {
        m_logger->info("{} yt-dlp process finished after being cancelled - not reporting as a failure", m_loggingPrefix);
        return;
    }

    QString stdOut = QString::fromUtf8(m_process->readAllStandardOutput());
    QString stdErr = QString::fromUtf8(m_process->readAllStandardError());

    if (exitStatus != QProcess::NormalExit || exitCode != 0)
    {
        m_logger->error("{} yt-dlp exited with code {}: {}", m_loggingPrefix, exitCode, stdErr.toStdString());
        emit failed(stdErr.trimmed().isEmpty() ? QString("yt-dlp exited with code %1").arg(exitCode) : stdErr.trimmed());
        return;
    }

    QStringList lines = stdOut.split('\n', Qt::SkipEmptyParts);
    if (lines.isEmpty())
    {
        m_logger->error("{} yt-dlp produced no output", m_loggingPrefix);
        emit failed("yt-dlp did not return a stream URL.");
        return;
    }

    if (lines.size() > 1)
    {
        m_logger->warn("{} yt-dlp returned {} lines - using the first one only; playback may be video-only (no audio)",
                        m_loggingPrefix, lines.size());
    }

    QString resolvedUrl = lines.first().trimmed();
    m_logger->info("{} Resolved stream URL successfully", m_loggingPrefix);
    emit resolved(resolvedUrl);
}
