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
    cdgOffsetsChanged();
    m_settings.setCdgWindowFullscreen(m_fullScreen);
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
    m_settings.setCdgWindowFullscreen(m_fullScreen);
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
    m_settings.setShowCdgWindow(true);
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
        if (m_settings.cdgWindowFullscreen())
        {
            this->showNormal();
            QTimer::singleShot(100, [&] () {
                ui->btnToggleFullscreen->setText("Make Windowed");
                this->showFullScreen();
                cdgOffsetsChanged();
            });
        }
        else
            ui->btnToggleFullscreen->setText("Make Fullscreen");
    }
    emit visibilityChanged(true);
}

void DlgCdg::hideEvent(QHideEvent *event)
{
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
