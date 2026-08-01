#include "dlgaddstreamsong.h"
#include "ui_dlgaddstreamsong.h"
#include <QApplication>
#include <QMessageBox>
#include <QProcess>
#include <QFileInfo>

DlgAddStreamSong::DlgAddStreamSong(QString singerName, QWidget *parent) :
        QDialog(parent),
        ui(new Ui::DlgAddStreamSong),
        m_singerName(std::move(singerName)) {
    ui->setupUi(this);
    setWindowTitle("Add stream song for " + m_singerName);
    ui->lblLookupResult->setText("");
    // The lookup is optional - without yt-dlp configured the KJ can still add
    // an entry manually, they just won't get a duration for rotation timing.
    if (m_settings.ytDlpPath().trimmed().isEmpty()) {
        ui->btnLookup->setEnabled(false);
        ui->btnLookup->setToolTip("Set a yt-dlp path in Settings -> External to enable lookups");
    }
}

DlgAddStreamSong::~DlgAddStreamSong() {
    delete ui;
}

QString DlgAddStreamSong::artist() const {
    return ui->lineEditArtist->text().trimmed();
}

QString DlgAddStreamSong::title() const {
    return ui->lineEditTitle->text().trimmed();
}

QString DlgAddStreamSong::url() const {
    return ui->lineEditUrl->text().trimmed();
}

void DlgAddStreamSong::on_btnLookup_clicked() {
    QString url = ui->lineEditUrl->text().trimmed();
    if (url.isEmpty()) {
        ui->lblLookupResult->setText("Enter a URL first.");
        return;
    }
    QString ytDlpPath = m_settings.ytDlpPath();
    if (ytDlpPath.trimmed().isEmpty() || !QFileInfo::exists(ytDlpPath)) {
        ui->lblLookupResult->setText("yt-dlp path is not set or doesn't exist.");
        return;
    }

    ui->lblLookupResult->setText("Looking up...");
    ui->btnLookup->setEnabled(false);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents();

    // Metadata only - fast enough to do synchronously, and it avoids pulling
    // the whole async resolver in for what is a one-off dialog action.
    QProcess process;
    process.start(ytDlpPath, QStringList() << "--no-playlist" << "--print" << "%(title)s"
                                           << "--print" << "%(duration)s" << url);
    bool finished = process.waitForFinished(30000);
    QApplication::restoreOverrideCursor();
    ui->btnLookup->setEnabled(true);

    if (!finished) {
        process.kill();
        ui->lblLookupResult->setText("Lookup timed out.");
        return;
    }
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        QString err = QString::fromUtf8(process.readAllStandardError()).trimmed();
        ui->lblLookupResult->setText(err.isEmpty() ? "Lookup failed." : "Lookup failed: " + err.left(120));
        return;
    }

    QStringList lines = QString::fromUtf8(process.readAllStandardOutput())
            .split('\n', Qt::SkipEmptyParts);
    if (lines.isEmpty()) {
        ui->lblLookupResult->setText("yt-dlp returned nothing.");
        return;
    }
    QString videoTitle = lines.at(0).trimmed();
    m_duration = (lines.size() > 1) ? static_cast<int>(lines.at(1).trimmed().toDouble()) : 0;

    // Only prefill the title if the KJ hasn't typed one - never clobber input.
    if (ui->lineEditTitle->text().trimmed().isEmpty())
        ui->lineEditTitle->setText(videoTitle);

    if (m_duration > 0) {
        int mins = m_duration / 60;
        int secs = m_duration % 60;
        ui->lblLookupResult->setText(QString("Found (%1:%2)").arg(mins).arg(secs, 2, 10, QChar('0')));
    } else {
        ui->lblLookupResult->setText("Found, but no duration reported.");
    }
}

void DlgAddStreamSong::on_buttonBox_accepted() {
    if (url().isEmpty()) {
        QMessageBox::warning(this, "URL required", "A URL is required to add a stream song.");
        return;
    }
    if (title().isEmpty()) {
        QMessageBox::warning(this, "Title required", "A title is required so the song can be identified in the rotation.");
        return;
    }
    accept();
}

void DlgAddStreamSong::on_buttonBox_rejected() {
    reject();
}
