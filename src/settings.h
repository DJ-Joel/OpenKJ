/*
 * Copyright (c) 2013-2019 Thomas Isaac Lightburn
 *
 *
 * This file is part of OpenKJ.
 *
 * OpenKJ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QHeaderView>
#include <QObject>
#include <QSettings>
#include <QSplitter>
#include <QStringList>
#include <QTableView>
#include <QTreeView>
#include <QWidget>
#include <QMetaType>
#include <QKeySequence>

struct SfxEntry
{
    SfxEntry();
    QString name;
    QString path;


}; Q_DECLARE_METATYPE(SfxEntry)


typedef QList<SfxEntry> SfxEntryList;

Q_DECLARE_METATYPE(QList<SfxEntry>)

class Settings : public QObject
{
    Q_OBJECT

private:
    QSettings *settings;
    bool m_safeStartupMode{false};

public:
    enum {
        LOG_LEVEL_DISABLED,
        LOG_LEVEL_CRITICAL,
        LOG_LEVEL_ERROR,
        LOG_LEVEL_WARNING,
        LOG_LEVEL_INFO,
        LOG_LEVEL_DEBUG,
        LOG_LEVEL_TRACE
    };
    int getConsoleLogLevel();
    int getFileLogLevel();
    bool tickerReducedCpuMode();
    void setTickerReducedCpuMode(bool enabled);
    void setConsoleLogLevel(int level);
    void setFileLogLevel(int level);
    int lastRunRotationTopSingerId();
    void setLastRunRotationTopSingerId(int id);
    [[nodiscard]] bool lastStartupOk() const;
    void setStartupOk(bool ok);
    [[nodiscard]] QString lastRunVersion() const;
    void setLastRunVersion(const QString &version);
    [[nodiscard]] bool safeStartupMode() const;
    void setSafeStartupMode(bool safeMode);
    [[nodiscard]] int historyDblClickAction() const;
    void setHistoryDblClickAction(int index);
    int getSystemRamSize();
    int remainRtOffset();
    int remainBtmOffset();
    qint64 hash(const QString & str);
    bool progressiveSearchEnabled();
    QString storeDownloadDir();
    QString logDir();
    QString ytDlpPath();
    QString downloadPath();
    // Cookie mode for yt-dlp invocations: "none", "browser", or "file". Used
    // to work around YouTube requiring sign-in for age-restricted videos.
    QString ytDlpCookieMode();
    void setYtDlpCookieMode(QString mode);
    // Browser keyword (e.g. "chrome", "firefox"), or "other-firefox" /
    // "other-chromium" for an unlisted Firefox/Chromium-based browser -
    // see ytDlpCookieArgs() for how those two get translated.
    QString ytDlpCookieBrowser();
    void setYtDlpCookieBrowser(QString browser);
    // Optional browser profile folder override - required for
    // "other-firefox"/"other-chromium", optional (default profile used if
    // blank) for the named browsers.
    QString ytDlpCookieProfilePath();
    void setYtDlpCookieProfilePath(QString path);
    // Path to a Netscape-format cookies.txt file, used when cookie mode is
    // "file".
    QString ytDlpCookieFilePath();
    void setYtDlpCookieFilePath(QString path);
    // Builds the --cookies-from-browser or --cookies argument pair to pass
    // to yt-dlp based on the current cookie mode, or an empty list if
    // cookies aren't configured. Every yt-dlp invocation in the app should
    // append this to its argument list.
    QStringList ytDlpCookieArgs();
    bool logShow();
    bool logEnabled();
    void setPassword(QString password);
    void clearPassword();
    bool chkPassword(QString password);
    bool passIsSet();
    void setCC(QString ccn, QString month, QString year, QString ccv, QString passwd);
    void setSaveCC(bool save);
    bool saveCC();
    void clearCC();
    void clearKNAccount();
    void setSaveKNAccount(bool save);
    bool saveKNAccount();
    bool testingEnabled();
    bool hardwareAccelEnabled();
    bool dbDoubleClickAddsSong();
    QString getCCN(const QString &password);
    QString getCCM(const QString &password);
    QString getCCY(const QString &password);
    QString getCCV(const QString &password);
    void setKaroakeDotNetUser(const QString &username, const QString &password);
    void setKaraokeDotNetPass(const QString &KDNPassword, const QString &password);
    QString karoakeDotNetUser(const QString &password);
    QString karoakeDotNetPass(const QString &password);
    enum BgMode { BG_MODE_IMAGE = 0, BG_MODE_SLIDESHOW };
    enum PreviewSize { Small, Medium, Large };
    explicit Settings(QObject *parent = 0);
    bool cdgWindowFullscreen();
    bool showCdgWindow();
    void setCdgWindowFullscreenMonitor(int monitor);
    int  cdgWindowFullScreenMonitor();
    void saveWindowState(QWidget *window);
    void restoreWindowState(QWidget *window);
    void saveColumnWidths(QTreeView *treeView);
    void saveColumnWidths(QTableView *tableView);
    void restoreColumnWidths(QTreeView *treeView);
    bool restoreColumnWidths(QTableView *tableView);
    void saveSplitterState(QSplitter *splitter);
    void restoreSplitterState(QSplitter *splitter);
    void setTickerFont(const QFont &font);
    void setApplicationFont(const QFont &font);
    // Same idea as cdgRemainFontRaw() above, for the ticker font.
    QFont tickerFontRaw();
    QFont tickerFont();
    [[nodiscard]] QFont applicationFont() const;
    // Resets the application font/size back to OpenKJ's built-in default,
    // persists it, applies it live, and returns the resulting font so the
    // caller (the Settings dialog) can update its own font controls.
    QFont resetApplicationFont();
    int tickerHeight();
    void setTickerHeight(int height);
    int tickerSpeed();
    void setTickerSpeed(int speed);
    QColor tickerTextColor();
    void setTickerTextColor(QColor color);
    bool cdgRemainEnabled();
    QColor tickerBgColor();
    void setTickerBgColor(QColor color);
    bool tickerFullRotation();
    void setTickerFullRotation(bool full);
    int tickerShowNumSingers();
    void setTickerShowNumSingers(int limit);
    void setTickerEnabled(bool enable);
    bool tickerEnabled();
    QString tickerCustomString();
    void setTickerCustomString(const QString &value);
    bool tickerShowRotationInfo();
    bool requestServerEnabled();
    void setRequestServerEnabled(bool enable);
    QString requestServerUrl();
    void setRequestServerUrl(QString url);
    int requestServerVenue();
    void setRequestServerVenue(int venueId);
    QString requestServerApiKey();
    void setRequestServerApiKey(QString apiKey);
    bool requestServerIgnoreCertErrors();
    void setRequestServerIgnoreCertErrors(bool ignore);
    bool audioUseFader();
    bool audioUseFaderBm();
    void setAudioUseFader(bool fader);
    void setAudioUseFaderBm(bool fader);
    int audioVolume();
    void setAudioVolume(int volume);
    QString cdgDisplayBackgroundImage();
    void setCdgDisplayBackgroundImage(QString imageFile);
    BgMode bgMode();
    void setBgMode(BgMode mode);
    QString bgSlideShowDir();
    void setBgSlideShowDir(QString dir);
    bool audioDownmix();
    void setAudioDownmix(bool downmix);
    bool audioDownmixBm();
    void setAudioDownmixBm(bool downmix);
    bool audioDetectSilence();
    bool audioDetectSilenceBm();
    void setAudioDetectSilence(bool enabled);
    void setAudioDetectSilenceBm(bool enabled);
    QString audioOutputDevice();
    QString audioOutputDeviceBm();
    void setAudioOutputDevice(QString device);
    void setAudioOutputDeviceBm(QString device);
    int audioBackend();
    void setAudioBackend(int index);
    QString recordingContainer();
    void setRecordingContainer(QString container);
    QString recordingCodec();
    void setRecordingCodec(QString codec);
    QString recordingInput();
    void setRecordingInput(QString input);
    QString recordingOutputDir();
    void setRecordingOutputDir(QString path);
    bool recordingEnabled();
    void setRecordingEnabled(bool enabled);
    QString recordingRawExtension();
    void setRecordingRawExtension(QString extension);
    int cdgOffsetTop();
    int cdgOffsetBottom();
    int cdgOffsetLeft();
    int cdgOffsetRight();
    bool ignoreAposInSearch();
    int videoOffsetMs();
    bool bmShowFilenames();
    void bmSetShowFilenames(bool show);
    bool bmShowMetadata();
    void bmSetShowMetadata(bool show);
    int bmVolume();
    void bmSetVolume(int bmVolume);
    int bmPlaylistIndex();
    void bmSetPlaylistIndex(int index);
    int mplxMode();
    void setMplxMode(int mode);
    bool karaokeAutoAdvance();
    int karaokeAATimeout();
    void setKaraokeAATimeout(int secs);
    bool karaokeAAAlertEnabled();
    void setKaraokeAAAlertEnabled(bool enabled);
    QFont karaokeAAAlertFont();
    void setKaraokeAAAlertFont(QFont font);
    bool showQueueRemovalWarning();
    bool showSingerRemovalWarning();
    bool showSongInterruptionWarning();
    bool showSongPauseStopWarning();
    QColor alertTxtColor();
    QColor alertBgColor();
    bool bmAutoStart();
    void setBmAutoStart(bool enabled);
    int cdgDisplayOffset();
    QFont bookCreatorTitleFont();
    QFont bookCreatorArtistFont();
    QFont bookCreatorHeaderFont();
    QFont bookCreatorFooterFont();
    QString bookCreatorHeaderText();
    QString bookCreatorFooterText();
    bool bookCreatorPageNumbering();
    int bookCreatorSortCol();
    double bookCreatorMarginRt();
    double bookCreatorMarginLft();
    double bookCreatorMarginTop();
    double bookCreatorMarginBtm();
    int bookCreatorCols();
    int bookCreatorPageSize();
    bool eqKBypass();
    int getEqKLevel(int band);
    bool eqBBypass();
    int getEqBLevel(int band);
    int requestServerInterval();
    bool bmKCrossFade();
    bool requestRemoveOnRotAdd();
    bool requestDialogAutoShow();
    bool checkUpdates();
    int updatesBranch();
    [[nodiscard]] int theme() const;
    const QPoint durationPosition();
    bool dbDirectoryWatchEnabled();
    SfxEntryList getSfxEntries();
    void addSfxEntry(SfxEntry entry);
    void setSfxEntries(SfxEntryList entries);
    [[nodiscard]] int estimationSingerPad() const;
    void setEstimationSingerPad(int secs);
    [[nodiscard]] int estimationEmptySongLength() const;
    void setEstimationEmptySongLength(int secs);
    [[nodiscard]] bool estimationSkipEmptySingers() const;
    void setEstimationSkipEmptySingers(bool skip);
    [[nodiscard]] bool rotationDisplayPosition() const;
    void setRotationDisplayPosition(bool show);
    int currentRotationPosition();
    bool dbSkipValidation();
    bool dbLazyLoadDurations();
    int systemId();
    // The literal font as saved (family/size/style), with no auto-scale
    // adjustment applied - used to populate the font picker in Settings so
    // it shows what was actually chosen, not a recalculated size. See
    // cdgRemainFont() below for the version used for actual on-screen
    // rendering.
    QFont cdgRemainFontRaw();
    QFont cdgRemainFont();
    QColor cdgRemainTextColor();
    QColor cdgRemainBgColor();
    [[nodiscard]] bool rotationShowNextSong() const;
    void sync();
    bool previewEnabled();
    bool showMainWindowVideo();
    bool showMainWindowSoundClips();
    void setShowMplxControls(bool show);
    bool showMplxControls();
    void setShowMainWindowSoundClips(const bool &show);
    bool showMainWindowNowPlaying();
    void setShowMainWindowNowPlaying(const bool &show);
    int mainWindowVideoSize();
    void setMainWindowVideoSize(const PreviewSize &size);
    bool enforceAspectRatio();
    void setEnforceAspectRatio(const bool &enforce);
    QString auxTickerFile();
    QString uuid();
    uint slideShowInterval();
    int lastSingerAddPositionType();
    void saveShortcutKeySequence(const QString &name, const QKeySequence &sequence);
    QKeySequence loadShortcutKeySequence(const QString &name);
    bool cdgPrescalingEnabled();
    [[nodiscard]] bool rotationAltSortOrder() const;
    bool treatAllSingersAsRegs();

    // When enabled, tickerFont() and cdgRemainFont() below return a version
    // of the saved font resized to fit whatever screen the CDG/singer display
    // window is currently on, instead of the literal saved point size. This
    // lets the same install look right on a small HD monitor or a big 4K TV
    // without the user manually re-tuning font sizes per venue/screen.
    bool tickerTimerAutoScale();
    void setTickerTimerAutoScale(bool enabled);
    // Called by DlgCdg (main/GUI thread) whenever the CDG display window's
    // size changes, so tickerFont()/cdgRemainFont() have a current reference
    // height to scale against. Safe to call/read from any thread (plain
    // atomic int, no Qt GUI objects touched) since the ticker's rendering
    // runs on its own worker thread and reads tickerFont() from there.
    static void setCdgDisplayHeightPx(int heightPx);

signals:
    void treatAllSingersAsRegsChanged(bool enabled);
    void enforceAspectRatioChanged(const bool &enforce);
    void requestServerVenueChanged(int venueId);
    void mplxModeChanged(int mode);
    void karaokeAutoAdvanceChanged(bool enabled);
    void showQueueRemovalWarningChanged(bool enabled);
    void showSingerRemovalWarningChanged(bool enabled);
    void showSongInterruptionWarningChanged(bool enabled);
    void showSongStopPauseWarningChanged(bool enabled);
    void requestServerIntervalChanged(int interval);
    void requestServerEnabledChanged(bool enabled);
    void rotationDisplayPositionChanged(bool show);
    void rotationDurationSettingsModified();
    void rotationShowNextSongChanged(bool show);
    void remainOffsetChanged(int offsetR, int offsetB);
    void previewEnabledChanged(bool enabled);
    void videoOffsetChanged(int offsetMs);
    void lastSingerAddPositionTypeChanged(int type);
    void shortcutsChanged();


public slots:
    void setShowMainWindowVideo(bool show);
    void setTreatAllSingersAsRegs(bool enabled);
    void setRotationAltSortOrder(bool enabled);
    void setCdgPrescalingEnabled(bool enabled);
    void setSlideShowInterval(int secs);
    void setHardwareAccelEnabled(bool enabled);
    void setDbDoubleClickAddsSong(bool enabled);
    void setDurationPosition(QPoint pos);
    void resetDurationPosition();
    void setRemainRtOffset(int offset);
    void setRemainBtmOffset(int offset);
    void dbSetLazyLoadDurations(bool val);
    void dbSetSkipValidation(bool val);
    void setBmKCrossfade(bool enabled);
    void setShowCdgWindow(bool show);
    void setCdgWindowFullscreen(bool fullScreen);
    void setCdgOffsetTop(int pixels);
    void setCdgOffsetBottom(int pixels);
    void setCdgOffsetLeft(int pixels);
    void setCdgOffsetRight(int pixels);
    void setShowQueueRemovalWarning(bool show);
    void setShowSingerRemovalWarning(bool show);
    void setKaraokeAutoAdvance(bool enabled);
    void setShowSongInterruptionWarning(bool enabled);
    void setAlertBgColor(QColor color);
    void setAlertTxtColor(QColor color);
    void setIgnoreAposInSearch(bool ignore);
    void setShowSongPauseStopWarning(bool enabled);
    void setBookCreatorArtistFont(QFont font);
    void setBookCreatorTitleFont(QFont font);
    void setBookCreatorHeaderFont(QFont font);
    void setBookCreatorFooterFont(QFont font);
    void setBookCreatorHeaderText(QString text);
    void setBookCreatorFooterText(QString text);
    void setBookCreatorPageNumbering(bool show);
    void setBookCreatorSortCol(int col);
    void setBookCreatorMarginRt(double margin);
    void setBookCreatorMarginLft(double margin);
    void setBookCreatorMarginTop(double margin);
    void setBookCreatorMarginBtm(double margin);
    void setEqKBypass(bool bypass);
    void setEqKLevel(int band, int level);
    void setEqBBypass(bool bypass);
    void setEqBLevel(int band, int level);
    void setRequestServerInterval(int interval);
    void setTickerShowRotationInfo(bool show);
    void setRequestRemoveOnRotAdd(bool remove);
    void setRequestDialogAutoShow(bool enabled);
    void setCheckUpdates(bool enabled);
    void setUpdatesBranch(int index);
    void setTheme(int theme);
    void setBookCreatorCols(int cols);
    void setBookCreatorPageSize(int size);
    void setStoreDownloadDir(QString path);
    void setLogEnabled(bool enabled);
    void setLogVisible(bool visible);
    void setLogDir(QString path);
    void setYtDlpPath(QString path);
    void setDownloadPath(QString path);
    void setCurrentRotationPosition(int position);
    void dbSetDirectoryWatchEnabled(bool val);
    void setSystemId(int id);
    void setCdgRemainEnabled(bool enabled);
    void setCdgRemainFont(QFont font);
    void setCdgRemainTextColor(QColor color);
    void setCdgRemainBgColor(QColor color);
    void setRotationShowNextSong(bool show);
    void setProgressiveSearchEnabled(bool enabled);
    void setPreviewEnabled(bool enabled);
    void setVideoOffsetMs(int offset);
    void setLastSingerAddPositionType(int type);
};

#endif // SETTINGS_H
