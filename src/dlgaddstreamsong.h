#ifndef DLGADDSTREAMSONG_H
#define DLGADDSTREAMSONG_H

#include <QDialog>
#include <QString>
#include "settings.h"

namespace Ui {
class DlgAddStreamSong;
}

/**
 * @brief Collects Artist / Title / URL for a stream entry.
 *
 * If a yt-dlp path is configured, the "Look up" button fetches the video's
 * title and duration. Duration matters because the rotation's time estimates
 * need it up front - we deliberately don't resolve the playable stream URL
 * here, since those are signed and expire within hours.
 */
class DlgAddStreamSong : public QDialog {
Q_OBJECT

public:
    explicit DlgAddStreamSong(QString singerName, QWidget *parent = nullptr);
    ~DlgAddStreamSong() override;

    [[nodiscard]] QString artist() const;
    [[nodiscard]] QString title() const;
    [[nodiscard]] QString url() const;
    [[nodiscard]] int duration() const { return m_duration; }

private slots:
    void on_btnLookup_clicked();
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

private:
    Ui::DlgAddStreamSong *ui;
    Settings m_settings;
    QString m_singerName;
    int m_duration{0};
};

#endif // DLGADDSTREAMSONG_H
