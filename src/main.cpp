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
#include <algorithm>
#include <QApplication>
#include "mainwindow.h"
#include <QStyleFactory>
#include <QSplashScreen>
#include <QStringList>
#include <QMessageBox>
#include "settings.h"
#include "idledetect.h"
#include "runguard/runguard.h"
#include "okjversion.h"
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/async_logger.h>
#include <spdlog/async.h>


Settings settings;
IdleDetect *filter;

//todo: This should be me moved into main after migration to spdlog is complete
//      It's currently only global for use by the QDebug callback
std::shared_ptr<spdlog::async_logger> logger;


void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    bool loggingEnabled = settings.logEnabled();
    std::string logMsg = msg.toStdString();
    if (context.function) {
        logMsg.append(" [");
        logMsg.append(context.function);
        logMsg.append("]");
    }
    switch (type) {
        case QtDebugMsg:
            if (!loggingEnabled)
                return;
            logger->debug(logMsg);
            break;
        case QtInfoMsg:
            logger->info(logMsg);
            break;
        case QtWarningMsg:
            logger->warn(logMsg);
            break;
        case QtCriticalMsg:
            logger->critical(logMsg);
            break;
        case QtFatalMsg:
            logger->critical(logMsg);
          //  abort();
    }
}

int main(int argc, char *argv[]) {
    QString logDir = settings.logDir();
    QDir dir;
    QString logFilePath;
    QString filename = "openkj-debug-" + QDateTime::currentDateTime().toString("yyyy-MM-dd") + ".log";
    dir.mkpath(logDir);
    logFilePath = logDir + QDir::separator() + filename;

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath.toStdString(), false);
    spdlog::init_thread_pool(8192, 2);
    std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
    logger = std::make_shared<spdlog::async_logger>("logger", sinks.begin(), sinks.end(), spdlog::thread_pool(),
                                                    spdlog::async_overflow_policy::block);
    spdlog::register_logger(logger);
    logger->set_level(spdlog::level::trace);
    spdlog::flush_every(std::chrono::seconds(1));
    logger->flush_on(spdlog::level::warn);

    auto consoleLogLevel = settings.getConsoleLogLevel();
    auto fileLogLevel = settings.getFileLogLevel();

    switch (consoleLogLevel) {
        case Settings::LOG_LEVEL_CRITICAL:
            console_sink->set_level(spdlog::level::critical);
            break;
        case Settings::LOG_LEVEL_ERROR:
            console_sink->set_level(spdlog::level::err);
            break;
        case Settings::LOG_LEVEL_WARNING:
            console_sink->set_level(spdlog::level::warn);
            break;
        case Settings::LOG_LEVEL_INFO:
            console_sink->set_level(spdlog::level::info);
            break;
        case Settings::LOG_LEVEL_DEBUG:
            console_sink->set_level(spdlog::level::debug);
            break;
        case Settings::LOG_LEVEL_TRACE:
            console_sink->set_level(spdlog::level::trace);
            break;
        default:
            console_sink->set_level(spdlog::level::off);
    }
    switch (fileLogLevel) {
        case Settings::LOG_LEVEL_CRITICAL:
            file_sink->set_level(spdlog::level::critical);
            break;
        case Settings::LOG_LEVEL_ERROR:
            file_sink->set_level(spdlog::level::err);
            break;
        case Settings::LOG_LEVEL_WARNING:
            file_sink->set_level(spdlog::level::warn);
            break;
        case Settings::LOG_LEVEL_INFO:
            file_sink->set_level(spdlog::level::info);
            break;
        case Settings::LOG_LEVEL_DEBUG:
            file_sink->set_level(spdlog::level::debug);
            break;
        case Settings::LOG_LEVEL_TRACE:
            file_sink->set_level(spdlog::level::trace);
            break;
        default:
            file_sink->set_level(spdlog::level::off);
    }
    if (file_sink->level() > console_sink->level())
        logger->set_level(file_sink->level());
    else
        logger->set_level(console_sink->level());

    logger->set_level(spdlog::level::trace);
    console_sink->set_pattern("[%^%l%$] %v");
    file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

    logger->info("OpenKJ version {} starting up", OKJ_VERSION_STRING);


    //QLoggingCategory::setFilterRules("*.debug=true");
    qInstallMessageHandler(myMessageOutput);
    QApplication a(argc, argv);

#ifdef MAC_OVERRIDE_GST
    // This points GStreamer paths to the framework contained in the app bundle.  Not needed on brew installs.
    QString appDir = QCoreApplication::applicationDirPath();
    qInfo() << "Application dir: " << appDir;
    appDir.remove(appDir.length() - 5, 5);
    qputenv("GST_PLUGIN_SYSTEM_PATH", QString(appDir + "Frameworks/GStreamer.framework/Versions/Current/lib/gstreamer-1.0").toLocal8Bit());
    qputenv("GST_PLUGIN_SCANNER", QString(appDir + "Frameworks/GStreamer.framework/Versions/Current/libexec/gstreamer-1.0/gst-plugin-scanner").toLocal8Bit());
    qputenv("GTK_PATH", QString(appDir + "Frameworks/GStreamer.framework/Versions/Current/").toLocal8Bit());
    qputenv("GIO_EXTRA_MODULES", QString(appDir + "Frameworks/GStreamer.framework/Versions/Current/lib/gio/modules").toLocal8Bit());
    qWarning() << "MacOS detected, changed GST env vars to point to the bundled framework";
    qWarning() << qgetenv("GST_PLUGIN_SYSTEM_PATH") << endl << qgetenv("GST_PLUGIN_SCANNER") << endl << qgetenv("GTK_PATH") << endl << qgetenv("GIO_EXTRA_MODULES") << endl;
#endif

#ifdef Q_OS_WIN
    // GStreamer's built-in Windows plugin auto-detection relies on a specific
    // bin/../lib/gstreamer-1.0 folder layout relative to wherever it loaded
    // gstreamer-1.0.dll from, which our flat, windeployqt-style deployment
    // doesn't provide. Point it explicitly at a "gstreamer-1.0" folder placed
    // directly alongside the executable instead, so plugin loading doesn't
    // depend on that auto-detection working out.
    QString winAppDir = QCoreApplication::applicationDirPath();
    QString gstPluginDir = winAppDir + "/gstreamer-1.0";
    qputenv("GST_PLUGIN_PATH", gstPluginDir.toLocal8Bit());
    qInfo() << "Windows detected, set GST_PLUGIN_PATH to" << gstPluginDir;

    // souphttpsrc (used for http/https network streams, e.g. the Stream
    // button) needs a GIO TLS backend module to perform the TLS handshake
    // for https:// URLs. Without it, GStreamer fails with "Secure
    // connection setup failed. / TLS/SSL support not available; install
    // glib-networking". Point GIO at a bundled copy of that module rather
    // than relying on it being found automatically.
    QString gioModuleDir = winAppDir + "/gio-modules";
    qputenv("GIO_EXTRA_MODULES", gioModuleDir.toLocal8Bit());
    qInfo() << "Windows detected, set GIO_EXTRA_MODULES to" << gioModuleDir;
#endif


    a.installEventFilter(filter);
    // Only set a default if the user/environment hasn't already set one -
    // e.g. someone deliberately running with GST_DEBUG=*:7 set externally
    // to debug an issue should still get that, not have it overridden.
    // Level 3 (FIXME) was needlessly noisy and added real overhead to every
    // pipeline operation on every run; level 1 (ERROR only) is enough for
    // normal use while still surfacing genuine problems in the log.
    if (qEnvironmentVariableIsEmpty("GST_DEBUG"))
        qputenv("GST_DEBUG", "*:1");
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    if (settings.theme() == 1) {
        QPalette palette;
        QApplication::setStyle(QStyleFactory::create("Fusion"));
        palette.setColor(QPalette::Window, QColor(53, 53, 53));
        palette.setColor(QPalette::WindowText, Qt::white);
        palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(127, 127, 127));
        palette.setColor(QPalette::Base, QColor(42, 42, 42));
        palette.setColor(QPalette::AlternateBase, QColor(66, 66, 66));
        palette.setColor(QPalette::ToolTipBase, Qt::white);
        palette.setColor(QPalette::ToolTipText, QColor(53, 53, 53));
        palette.setColor(QPalette::Text, Qt::white);
        palette.setColor(QPalette::Disabled, QPalette::Text, QColor(127, 127, 127));
        palette.setColor(QPalette::Dark, QColor(35, 35, 35));
        palette.setColor(QPalette::Shadow, QColor(20, 20, 20));
        palette.setColor(QPalette::Button, QColor(53, 53, 53));
        palette.setColor(QPalette::ButtonText, Qt::white);
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(127, 127, 127));
        palette.setColor(QPalette::BrightText, Qt::red);
        palette.setColor(QPalette::Link, QColor(42, 130, 218));
        palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80, 80, 80));
        palette.setColor(QPalette::HighlightedText, Qt::white);
        palette.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(127, 127, 127));
        QApplication::setPalette(palette);
    } else if (settings.theme() == 2) {
        QApplication::setStyle(QStyleFactory::create("Fusion"));
    }
//    else
//    {
//
//    }
    QApplication::setFont(settings.applicationFont(), "QWidget");
    QApplication::setFont(settings.applicationFont(), "QMenu");
    QApplication::setFont(settings.applicationFont(), "QAction");

    // RunGuard seems to be broken by flatpak, ignore it if running in a flatpak sandbox
    logger->info("App dir path {}", QCoreApplication::applicationDirPath().toStdString());
    if (QCoreApplication::applicationDirPath() == "/app/bin") {
        logger->info("RunGuard disabled due to flatpak sandbox");
    } else {
        logger->debug("Checking for other instances");
        RunGuard guard("SharedMemorySingleInstanceProtectorOpenKJ");
        if (!guard.tryToRun()) {
            QMessageBox msgBox;
            msgBox.setText("OpenKJ is already running!");
            msgBox.setInformativeText(
                    "In order to protect the database, you can only run one instance of OpenKJ at a time.\nExiting now.");
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.exec();
            return 1;
        }
    }
    if (!settings.lastStartupOk()) {
        QMessageBox msgBox;
        msgBox.setText("OpenKJ appears to have failed to startup on the last run.");
        msgBox.setInformativeText("Would you like to attempt to recover by loading safe settings?");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        if (msgBox.exec() == QMessageBox::Yes)
            settings.setSafeStartupMode(true);
        else
            settings.setSafeStartupMode(false);
    }
#ifdef Q_OS_DARWIN
    if (settings.lastRunVersion() != OKJ_VERSION_STRING)
        settings.setSafeStartupMode(true);
#endif
    settings.setLastRunVersion(OKJ_VERSION_STRING);
    settings.setStartupOk(false);
    MainWindow w;
    w.show();
    return QApplication::exec();
}
