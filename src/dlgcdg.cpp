/*
 * Copyright (c) 2013-2021 Thomas Isaac Lightburn
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

#include "dlgcdg.h"
#include "ui_dlgcdg.h"
#include <QGuiApplication>
#include <QSvgRenderer>
#include <QPainter>
#include <QDir>
#include <QFile>
#include <QImageReader>
#include <QScreen>
#include <QResizeEvent>


VideoDisplay *DlgCdg::getVideoDisplay()
{
    return ui->videoDisplayKar;
}

VideoDisplay *DlgCdg::getVideoDisplayBm()
{
    return ui->videoDisplayBm;
}

DlgCdg::DlgCdg(MediaBackend &KaraokeBackend, MediaBackend &BreakBackend, QWidget *parent, Qt::WindowFlags f) :
    QDialog(parent, f), ui(new Ui::DlgCdg), m_kmb(KaraokeBackend), m_bmb(BreakBackend)
{
    ui->setupUi(this);
    m_tWidget = std::make_unique<TransparentWidget>(this);
    m_tWidget->setObjectName("DurationTimer");
    m_tWidget->show();
    m_tWidget->move(m_settings.durationPosition());
    ui->videoDisplayKar->setFillOnPaint(false);
    ui->widgetAlert->setAutoFillBackground(true);
    ui->fsToggleWidget->hide();
    ui->widgetAlert->hide();
    ui->widgetAlert->setAttribute(Qt::WA_TransparentForMouseEvents);
    ui->widgetAlert->setMouseTracking(true);
    ui->scroll->setVisible(m_settings.tickerEnabled());
    ui->scroll->setTickerEnabled(m_settings.tickerEnabled());
    m_tWidget->setVisible(m_settings.cdgRemainEnabled());
    tickerFontChanged();
    ui->scroll->setSpeed(m_settings.tickerSpeed());
    remainFontChanged();
    QPalette palette = ui->scroll->palette();
    palette.setColor(ui->scroll->foregroundRole(), m_settings.tickerTextColor());
    ui->scroll->setPalette(palette);
    palette = this->palette();
    palette.setColor(QPalette::Window, m_settings.tickerBgColor());
    setPalette(palette);
    m_lastSize.setWidth(300);
    m_lastSize.setHeight(216);
    m_fullScreen = m_settings.cdgWindowFullscreen();
    m_tWidget->setTextColor(m_settings.cdgRemainTextColor());
    m_tWidget->setBackgroundColor(m_settings.cdgRemainBgColor());
    applyBackgroundImageMode();
    showAlert(false);
    alertFontChanged(m_settings.karaokeAAAlertFont());
    alertBgColorChanged(m_settings.alertBgColor());
    alertTxtColorChanged(m_settings.alertTxtColor());
    cdgOffsetsChanged();
    connect(ui->videoDisplayKar, &VideoDisplay::mouseMoveEvent, this, &DlgCdg::mouseMove);
    connect(ui->btnToggleFullscreen, &QPushButton::clicked, this, &DlgCdg::btnToggleFullscreenClicked);
    connect(&m_timerSlideShow, &QTimer::timeout, this, &DlgCdg::timerSlideShowTimeout);
    connect(&m_timer1s, &QTimer::timeout, this, &DlgCdg::timer1sTimeout);
    connect(&m_timerAlertCountdown, &QTimer::timeout, this, &DlgCdg::timerCountdownTimeout);
    connect(&m_timerButtonShow, &QTimer::timeout, [&] () { ui->fsToggleWidget->hide(); });
    m_timerButtonShow.setInterval(1000);
    m_timerAlertCountdown.setInterval(1000);
    m_timer1s.start(1000);
    if (m_settings.bgMode() == Settings::BG_MODE_SLIDESHOW)
        m_timerSlideShow.start(static_cast<int>(m_settings.slideShowInterval() * 1000));
    ui->videoDisplayBm->hide();
    if (!m_settings.showCdgWindow())
        hide();
    else
        show();
}

DlgCdg::~DlgCdg() = default;

void DlgCdg::setTickerText(const QString &text)
{
    ui->scroll->setText(text);
}

void DlgCdg::stopTicker()
{
    ui->scroll->stop();
}

void DlgCdg::tickerFontChanged()
{
    ui->scroll->setFont(m_settings.tickerFont());
    ui->scroll->setMinimumHeight(QFontMetrics(m_settings.tickerFont()).height());
    ui->scroll->setMaximumHeight(QFontMetrics(m_settings.tickerFont()).height());
    ui->scroll->refresh();
}

void DlgCdg::remainFontChanged()
{
    m_tWidget->setTextFont(m_settings.cdgRemainFont());
}

void DlgCdg::tickerTimerAutoScaleChanged()
{
    // Toggling the setting should take effect right away, not wait for the
    // window to next be resized.
    tickerFontChanged();
    remainFontChanged();
}

void DlgCdg::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
    // Qt can fire several resize events for a single logical resize (e.g.
    // during a fullscreen transition). Only react once the height has
    // actually settled on a new value, and only if auto-scale is even
    // turned on - otherwise this is a no-op, identical to before this
    // feature existed. Refreshing the ticker unconditionally on every
    // resize callback was spawning a new re-render faster than the ticker
    // thread could settle, which stopped it from scrolling.
    int newHeight = height();
    if (newHeight == m_lastAppliedDisplayHeight)
        return;
    m_lastAppliedDisplayHeight = newHeight;
    Settings::setCdgDisplayHeightPx(newHeight);
    if (!m_settings.tickerTimerAutoScale())
        return;
    tickerFontChanged();
    remainFontChanged();
}

void DlgCdg::tickerSpeedChanged()
{
    ui->scroll->setSpeed(m_settings.tickerSpeed());
}

void DlgCdg::tickerTextColorChanged()
{
    auto palette = ui->scroll->palette();
    palette.setColor(ui->scroll->foregroundRole(), m_settings.tickerTextColor());
    ui->scroll->setPalette(palette);
    ui->scroll->refresh();
}

void DlgCdg::tickerBgColorChanged()
{
    auto palette = this->palette();
    palette.setColor(QPalette::Window, m_settings.tickerBgColor());
    this->setPalette(palette);
    ui->scroll->refresh();
    //ui->scroll->refreshTickerSettings();
}

void DlgCdg::tickerEnableChanged()
{
    ui->scroll->setVisible(m_settings.tickerEnabled());
    ui->scroll->setTickerEnabled(m_settings.tickerEnabled());
    ui->scroll->setText(ui->scroll->getCurrentText(), true);
}

void DlgCdg::mouseDoubleClickEvent([[maybe_unused]]QMouseEvent *e)
{
    m_fullScreen = !m_fullScreen;
    if (m_fullScreen)
        showFullScreen();
    else
        showNormal();
    // The Fullscreen/Make Windowed button used to only get its text set
    // once, inside showEvent() - neither this double-click toggle nor the
    // button's own click handler below ever updated it afterward. That left
    // the label frozen after the very first fullscreen change, so from then
    // on it always described the OPPOSITE of what the window actually was -
    // this is what made the button look like it was working backwards.
    ui->btnToggleFullscreen->setText(m_fullScreen ? "Make Windowed" : "Make Fullscreen");
    cdgOffsetsChanged();
    m_settings.setCdgWindowFullscreen(m_fullScreen);
    // Only persist geometry while windowed. Saving it while fullscreen
    // records the fullscreen-sized rectangle as the window's "normal"
    // geometry, so the next time it's shown windowed it comes back
    // oversized - bigger than the monitor, with its edges off-screen and
    // no way to grab them to resize it back down.
    if (!m_fullScreen)
        m_settings.saveWindowState(this);
    // QDesktopWidget was removed in Qt 6 - QScreen is the modern
    // replacement. screens().indexOf(...) reproduces the same "index of
    // the screen this window is mostly on" value QDesktopWidget::screenNumber()
    // used to return, which is what Settings expects to store here.
    m_settings.setCdgWindowFullscreenMonitor(QGuiApplication::screens().indexOf(this->screen()));
}

QFileInfoList DlgCdg::getSlideShowImages()
{
    QFileInfoList images;
    QDir srcDir(m_settings.bgSlideShowDir());
    auto files = srcDir.entryInfoList(QDir::Files, QDir::Name | QDir::IgnoreCase);
    for (const auto & file : files)
    {
        if (QImageReader::imageFormat(file.absoluteFilePath()) != "")
            images << file;
    }
    return images;
}

void DlgCdg::showAlert(bool show)
{
    if ((show) && (m_settings.karaokeAAAlertEnabled()))
    {
        ui->videoDisplayKar->hide();
        ui->videoDisplayBm->hide();
        ui->widgetAlert->show();
    }
    else
    {
        ui->widgetAlert->hide();
        if (m_bmb.hasActiveVideo() && !m_kmb.hasActiveVideo()) {
            ui->videoDisplayBm->show();
            ui->videoDisplayKar->hide();
        }
        else {
            ui->videoDisplayBm->hide();
            ui->videoDisplayKar->show();
        }
    }
}

void DlgCdg::setNextSinger(const QString &name)
{
    ui->lblNextSinger->setText(name);
}

void DlgCdg::setNextSong(const QString &song)
{
    ui->lblNextSong->setText(song);
}

void DlgCdg::setCountdownSecs(int seconds)
{
    m_countdownPos = seconds;
    ui->lblSeconds->setText(QString::number(seconds) + tr(" seconds"));
    m_timerAlertCountdown.stop();
    m_timerAlertCountdown.start();
}

void DlgCdg::timerCountdownTimeout()
{
    if (m_countdownPos > 0)
        m_countdownPos--;
    ui->lblSeconds->setText(QString::number(m_countdownPos) + tr(" seconds"));
    ui->lblSeconds->repaint();
    ui->widgetAlert->repaint();
}

void DlgCdg::alertBgColorChanged(const QColor &color)
{
    auto palette = ui->widgetAlert->palette();
    palette.setColor(ui->widgetAlert->backgroundRole(), color);
    ui->widgetAlert->setPalette(palette);
}

void DlgCdg::alertTxtColorChanged(const QColor &color)
{
    auto palette = ui->widgetAlert->palette();
    palette.setColor(ui->widgetAlert->foregroundRole(), color);
    ui->widgetAlert->setPalette(palette);
}

void DlgCdg::applyBackgroundImageMode()
{
    if (m_settings.bgMode() == Settings::BgMode::BG_MODE_IMAGE && QFile::exists(m_settings.cdgDisplayBackgroundImage()))
    {
        m_timerSlideShow.stop();
        ui->videoDisplayKar->setBackground(QPixmap(m_settings.cdgDisplayBackgroundImage()));
    }
    else if (m_settings.bgMode() == Settings::BgMode::BG_MODE_SLIDESHOW && QDir(m_settings.bgSlideShowDir()).exists())
    {
        m_timerSlideShow.start();
        slideShowMoveNext();
    }
    else
    {
        m_timerSlideShow.stop();
        ui->videoDisplayKar->useDefaultBackground();
    }
}

void DlgCdg::timerSlideShowTimeout()
{
    if (!ui->videoDisplayKar->hasActiveVideo() && !ui->videoDisplayBm->hasActiveVideo())
    {
        slideShowMoveNext();
    }
}

void DlgCdg::slideShowMoveNext()
{
    m_curSlideshowPos++;
    auto images = getSlideShowImages();
    if (images.empty())
    {
        ui->videoDisplayKar->useDefaultBackground();
        return;
    }
    if (m_curSlideshowPos >= images.size())
        m_curSlideshowPos = 0;
    if (images.at(m_curSlideshowPos).fileName().endsWith("svg", Qt::CaseInsensitive))
    {
        QPixmap bgImage(QSize(1920,1080));
        QPainter painter(&bgImage);
        QSvgRenderer renderer(images.at(m_curSlideshowPos).absoluteFilePath());
        renderer.render(&painter);
        ui->videoDisplayKar->setBackground(bgImage);
    }
    else
        ui->videoDisplayKar->setBackground(images.at(m_curSlideshowPos).absoluteFilePath());
}

void DlgCdg::alertFontChanged(const QFont &font)
{
    ui->label->setFont(font);
    ui->label_2->setFont(font);
    ui->label_4->setFont(font);
    ui->lblNextSinger->setFont(font);
    ui->lblNextSong->setFont(font);
    ui->lblSeconds->setFont(font);
}

void DlgCdg::mouseMove([[maybe_unused]]QMouseEvent *event)
{
    if (m_fullScreen)
        ui->btnToggleFullscreen->setText(tr("Make Windowed"));
    else
        ui->btnToggleFullscreen->setText(tr("Make Fullscreen"));
    ui->fsToggleWidget->show();
    m_timerButtonShow.start();
}

void DlgCdg::timer1sTimeout()
{
    if (m_settings.cdgRemainEnabled())
    {
        if (m_kmb.state() == MediaBackend::PlayingState && !m_tWidget->isVisible())
        {
            m_tWidget->show();
        }
        else if (m_kmb.state() != MediaBackend::PlayingState && m_tWidget->isVisible())
        {
            m_tWidget->hide();
        }
        if (m_kmb.state() == MediaBackend::PlayingState)
        {
            m_tWidget->setString(" " + MediaBackend::msToMMSS(m_kmb.duration() - m_kmb.position()) + " ");
        }
    }
}


void DlgCdg::btnToggleFullscreenClicked()
{
    m_fullScreen = !m_fullScreen;
    if (m_fullScreen)
    {
        showFullScreen();
    }
    else
        showNormal();
    // See the matching comment in mouseDoubleClickEvent() above - this
    // button's label was never actually updated after a click, only by
    // showEvent() the next time the window was shown, so it went stale
    // and read backwards after the first press.
    ui->btnToggleFullscreen->setText(m_fullScreen ? "Make Windowed" : "Make Fullscreen");
    m_settings.setCdgWindowFullscreen(m_fullScreen);
    // See the matching comment in mouseDoubleClickEvent() above - saving
    // geometry while fullscreen is what was making this window come back
    // oversized (bigger than the monitor, unreachable edges) the next time
    // it was shown windowed.
    if (!m_fullScreen)
        m_settings.saveWindowState(this);
    // QDesktopWidget was removed in Qt 6 - QScreen is the modern
    // replacement. screens().indexOf(...) reproduces the same "index of
    // the screen this window is mostly on" value QDesktopWidget::screenNumber()
    // used to return, which is what Settings expects to store here.
    m_settings.setCdgWindowFullscreenMonitor(QGuiApplication::screens().indexOf(this->screen()));
    cdgOffsetsChanged();
}

void DlgCdg::cdgOffsetsChanged()
{
    if (isFullScreen())
        this->layout()->setContentsMargins(m_settings.cdgOffsetLeft(),m_settings.cdgOffsetTop(),m_settings.cdgOffsetRight(),m_settings.cdgOffsetBottom());
    else
        this->layout()->setContentsMargins(0,0,0,0);
}


void DlgCdg::closeEvent([[maybe_unused]]QCloseEvent *event)
{
    this->hide();
    m_settings.setShowCdgWindow(false);
    event->ignore();
    // hide() above delivers a hide event synchronously before this line
    // even runs, and that now emits visibilityChanged(false) itself (see
    // hideEvent() below) - so nothing further to do here.
}

void DlgCdg::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    m_settings.restoreWindowState(this);

    // Figure out whether we're about to (re-)enter fullscreen THIS show
    // cycle before doing anything geometry-related below. This used to run
    // after the geometry-repair block instead, which meant that block was
    // deciding whether to clamp the window's size based on isFullScreen() -
    // Qt's CURRENT state, before showFullScreen() below has even been
    // requested yet - rather than what the window is actually about to
    // become. If a saved "fullscreen" preference meant this window was
    // about to be sent fullscreen via the delayed call below, this code
    // would still see isFullScreen() == false at that moment and force the
    // window down to an 800x600 windowed rectangle right as it was also
    // being told to go fullscreen - leaving Qt's fullscreen flag set (so
    // the window drew with no title bar/border, since true fullscreen
    // windows never have one) while the actual on-screen rectangle was
    // whatever the clamp had just forced it to. That's what made the
    // window look small and border-less with no edges to grab, and made
    // the Fullscreen/Make Windowed button (which was still going by the
    // correct m_fullScreen/settings value) look like it was working
    // backwards - clicking "Make Windowed" while already stuck in this
    // half-fullscreen state was the first thing to actually call
    // showNormal() cleanly, which is why it looked like it flipped to
    // fullscreen-sized instead of shrinking.
    bool aboutToGoFullscreen = false;
    // Only restore the last-known fullscreen state once per hide->show
    // session (the flag is reset in hideEvent() below). Without this, a
    // later, legitimate showFullScreen()/showNormal() call from elsewhere -
    // like the "Fullscreen"/"Make Windowed" button - can re-deliver a show
    // event here and re-run this block, undoing whatever that button just
    // did (this is what broke the fullscreen button after the previous
    // fix's narrower re-entrancy guard let this second call through).
    if (!m_fullscreenStateRestoredThisShow)
    {
        m_fullscreenStateRestoredThisShow = true;
        // m_fullScreen needs to reflect the restored state right away, not
        // only once the delayed showFullScreen() below actually runs. It
        // was only being set inside code the user's own button/double-click
        // triggers, never here - so if the singer window opened into a
        // saved "fullscreen" state, m_fullScreen still read whatever it was
        // left at from construction (or an earlier session) until the timer
        // fired 100ms later. Clicking the Fullscreen/Make Windowed button
        // during that 100ms gap read/updated the stale value, and then the
        // delayed showFullScreen() below stomped over it without knowing a
        // click had happened - that's what made the first fullscreen click
        // appear to do nothing, and the next click do the opposite of what
        // its label said.
        m_fullScreen = m_settings.cdgWindowFullscreen();
        aboutToGoFullscreen = m_fullScreen;
        if (m_fullScreen)
        {
            this->showNormal();
            ui->btnToggleFullscreen->setText("Make Windowed");
            QTimer::singleShot(100, [&] () {
                // Only actually go fullscreen if nothing changed m_fullScreen
                // in the 100ms since we decided to restore it (e.g. the user
                // clicking the button in the meantime) - otherwise this would
                // undo their click.
                if (m_fullScreen)
                {
                    this->showFullScreen();
                    cdgOffsetsChanged();
                }
            });
        }
        else
            ui->btnToggleFullscreen->setText("Make Fullscreen");
    }

    // Repair any geometry that was already saved bad by an earlier version
    // of this code (before the fullscreen-save guards above existed) - if
    // the restored size/position doesn't fit on the screen it's opening on,
    // it means the window would be too big and/or partly off-screen with no
    // way to grab its edges to fix it. Fall back to a reasonable windowed
    // size centered on that screen instead. Skipped entirely if we're about
    // to be fullscreen this cycle - see the long comment above.
    if (!isFullScreen() && !aboutToGoFullscreen)
    {
        const QRect avail = this->screen() ? this->screen()->availableGeometry()
                                            : QGuiApplication::primaryScreen()->availableGeometry();
        const QRect geo = this->geometry();
        const bool tooBig = geo.width() > avail.width() || geo.height() > avail.height();
        const bool offScreen = !avail.intersects(geo);
        if (tooBig || offScreen)
        {
            QSize fallbackSize(qMin(avail.width(), 800), qMin(avail.height(), 600));
            QRect fallback(QPoint(0, 0), fallbackSize);
            fallback.moveCenter(avail.center());
            this->setGeometry(fallback);
        }
    }
    m_settings.setShowCdgWindow(true);
    emit visibilityChanged(true);
}

void DlgCdg::hideEvent(QHideEvent *event)
{
    // Same reasoning as the fullscreen-toggle handlers above: if this
    // window happens to get hidden (Escape, the X button, etc.) while it's
    // still fullscreen, saving geometry here would persist the fullscreen
    // size as the "normal" size and the window would come back oversized
    // next time.
    if (!isFullScreen())
        m_settings.saveWindowState(this);
    QWidget::hideEvent(event);
    // Reset so the next time this window is shown, showEvent() correctly
    // restores its last-known fullscreen state again.
    m_fullscreenStateRestoredThisShow = false;
    // Pressing Escape on this window calls QDialog::reject(), which hides
    // the window directly without ever going through closeEvent() above -
    // that's a documented Qt behavior, not a bug in this file. Emitting
    // here instead means the main window's toggle button gets told the
    // singer window closed no matter how it closed (Escape key, the X
    // button, or a call to hide() from elsewhere), instead of only when
    // closeEvent() specifically runs.
    emit visibilityChanged(false);
}

void DlgCdg::setSlideshowInterval(int secs) {
    m_timerSlideShow.setInterval(secs * 1000);
}


TransparentWidget::~TransparentWidget()
{
    m_settings.saveWindowState(this);
}

void TransparentWidget::mouseMoveEvent(QMouseEvent *event)
{
    // Dragging this widget used to move it live via move(), but that leaves
    // behind visible trail artifacts where it overlaps the karaoke video
    // display underneath - the video display is drawn by GStreamer directly
    // onto its own native window area rather than through Qt's normal
    // widget painting, so Qt has no way to know it needs to repaint the
    // spot this widget just vacated. Dragging is disabled entirely instead
    // of trying to repaint around a video overlay. The widget's position is
    // still restored from Settings on startup, and can be changed by
    // editing that saved position directly if it's ever needed.
    QWidget::mouseMoveEvent(event);
}

void TransparentWidget::moveEvent(QMoveEvent *event)
{
    QWidget::moveEvent(event);
    //settings.saveWindowState(this);
}

void TransparentWidget::mousePressEvent(QMouseEvent *event) {
    // Dragging is disabled - see mouseMoveEvent() above.
    QWidget::mousePressEvent(event);
}

void TransparentWidget::setString(const QString &string) const {
    m_label->setText(string);
}

void TransparentWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.fillRect (this->rect(), QColor(0, 0, 0, 0x20)); /* set transparent color*/
}

TransparentWidget::TransparentWidget(QWidget *parent)
        : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint);
    auto layout = new QHBoxLayout(this);
    setLayout(layout);
    // QLayout::setMargin() was removed in Qt 6; setContentsMargins() below
    // already sets all margins to 0, so the old call is simply dropped.
    layout->setSpacing(0);
    layout->setContentsMargins(0,0,0,0);
    setContentsMargins(0,0,0,0);
    m_label = std::make_unique<QLabel>(this);
    layout->addWidget(m_label.get());
    m_label->setMargin(0);
    m_label->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    m_label->setText("00:00");
    m_label->setAutoFillBackground(true);
    m_label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
}

void TransparentWidget::setTextColor(const QColor &color) const {
    auto palette = m_label->palette();
    palette.setColor(QPalette::WindowText, color);
    m_label->setPalette(palette);
}

void TransparentWidget::setBackgroundColor(const QColor &color) const {
    auto palette = m_label->palette();
    palette.setColor(QPalette::Window, color);
    m_label->setPalette(palette);
}

void TransparentWidget::setTextFont(const QFont &font) {
    m_label->setFont(font);
    m_label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
#if (QT_VERSION >= QT_VERSION_CHECK(5,11,0))
    setFixedSize(QFontMetrics(font).horizontalAdvance("_____"), QFontMetrics(font).height());
    m_label->setFixedSize(QFontMetrics(font).horizontalAdvance("_____"), QFontMetrics(font).height());
#else
    setFixedSize(QFontMetrics(font).width("_____"),QFontMetrics(font).tightBoundingRect("0123456789:").height());
    m_label->setFixedSize(QFontMetrics(font).width("_____"),QFontMetrics(font).tightBoundingRect("0123456789:").height());
#endif
}

void TransparentWidget::resetPosition() {
    move(0,0);
}
