#ifndef YTDLPRESOLVER_H
#define YTDLPRESOLVER_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/async_logger.h>
#include <spdlog/fmt/ostr.h>

/**
 * @brief Resolves a page URL (e.g. a YouTube link) into a direct, playable
 * media stream URL by shelling out to a user-provided yt-dlp executable.
 * Runs asynchronously via QProcess so it doesn't block the UI thread while
 * yt-dlp does its (potentially multi-second) network resolution.
 */
class YtDlpResolver : public QObject
{
    Q_OBJECT
private:
    std::unique_ptr<QProcess> m_process;
    bool m_cancelled{false};
    std::string m_loggingPrefix{"[YtDlpResolver]"};
    std::shared_ptr<spdlog::logger> m_logger;

public:
    explicit YtDlpResolver(QObject *parent = nullptr);
    ~YtDlpResolver() override;

    // Starts an async resolve. Emits resolved() or failed() when done.
    // If a resolve is already running, logs a warning and does nothing.
    void resolve(const QString &ytDlpPath, const QString &url);

    // Kills any in-progress resolve (e.g. user cancel, or a safety timeout).
    void cancel();

    [[nodiscard]] bool isRunning() const;

signals:
    void resolved(QString streamUrl);
    void failed(QString errorMessage);

private slots:
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void processErrorOccurred(QProcess::ProcessError error);
};

#endif // YTDLPRESOLVER_H
