#ifndef YTDLPDOWNLOADER_H
#define YTDLPDOWNLOADER_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/async_logger.h>
#include <spdlog/fmt/ostr.h>

/**
 * @brief Downloads a video (e.g. from YouTube) to a local file via a
 * user-provided yt-dlp executable. Runs asynchronously via QProcess so it
 * doesn't block the UI thread during a potentially multi-minute download,
 * and supports cancellation - unlike YtDlpResolver's quick metadata-only
 * calls, an actual download can genuinely take minutes on a slow connection.
 *
 * yt-dlp cannot guarantee the requested .mp4 container - if the best
 * available stream for a given video isn't mp4, yt-dlp would need ffmpeg to
 * remux it, which isn't bundled here. Rather than assume the download landed
 * at the exact requested path, the actual final path (and the duration) are
 * read back from yt-dlp's own --print-to-file reporting into dedicated temp
 * files, kept deliberately separate from yt-dlp's own progress output on
 * stdout - mixing structured data into the same noisy stream yt-dlp uses for
 * "[download] 45.2% of 10.00MiB..." progress lines would mean fragile
 * parsing to tell them apart. Regular stdout is left free for exactly that
 * progress text instead, relayed live via the progress() signal.
 */
class YtDlpDownloader : public QObject
{
    Q_OBJECT
private:
    std::unique_ptr<QProcess> m_process;
    bool m_cancelled{false};
    QString m_filePathTempFile;
    QString m_durationTempFile;
    std::string m_loggingPrefix{"[YtDlpDownloader]"};
    std::shared_ptr<spdlog::logger> m_logger;

public:
    explicit YtDlpDownloader(QObject *parent = nullptr);
    ~YtDlpDownloader() override;

    // Starts an async download. outputPathNoExt is the full destination
    // path WITHOUT an extension (e.g. "C:/Downloads/Artist - Title") -
    // yt-dlp appends the real extension for whatever format actually gets
    // saved, which is why the real final path is reported back rather than
    // assumed.
    void download(const QString &ytDlpPath, const QString &url, const QString &outputPathNoExt);

    // Kills any in-progress download (e.g. user cancel).
    void cancel();

    [[nodiscard]] bool isRunning() const;

signals:
    // Relays yt-dlp's own progress lines more or less verbatim, for a live
    // status display. Not guaranteed to be clean/parseable - just something
    // to show the user while a download is in flight.
    void progress(QString statusLine);

    // filePath is the REAL final path yt-dlp reports, which may have a
    // different extension than requested - see the class comment.
    // durationSecs is looked up in the same invocation, no second process
    // call needed. 0 if yt-dlp didn't report a usable duration.
    void finished(QString filePath, int durationSecs);

    void failed(QString errorMessage);

private slots:
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void processErrorOccurred(QProcess::ProcessError error);
    void readyReadStandardOutput();
};

#endif // YTDLPDOWNLOADER_H
