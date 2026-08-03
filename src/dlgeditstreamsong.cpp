#include "dlgeditstreamsong.h"
#include "ui_dlgeditstreamsong.h"
#include <QMessageBox>

DlgEditStreamSong::DlgEditStreamSong(const QString &artist, const QString &title, QWidget *parent) :
        QDialog(parent),
        ui(new Ui::DlgEditStreamSong) {
    ui->setupUi(this);
    ui->lineEditArtist->setText(artist);
    ui->lineEditTitle->setText(title);
}

DlgEditStreamSong::~DlgEditStreamSong() {
    delete ui;
}

QString DlgEditStreamSong::artist() const {
    return ui->lineEditArtist->text().trimmed();
}

QString DlgEditStreamSong::title() const {
    return ui->lineEditTitle->text().trimmed();
}

void DlgEditStreamSong::on_buttonBox_accepted() {
    if (artist().isEmpty() && title().isEmpty()) {
        QMessageBox::warning(this, "Artist or title required",
                              "Enter at least an artist or a title so the song can be identified.");
        return;
    }
    accept();
}

void DlgEditStreamSong::on_buttonBox_rejected() {
    reject();
}
