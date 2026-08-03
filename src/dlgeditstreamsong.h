#ifndef DLGEDITSTREAMSONG_H
#define DLGEDITSTREAMSONG_H

#include <QDialog>
#include <QString>

namespace Ui {
class DlgEditStreamSong;
}

/**
 * @brief Edits the artist/title of a shared stream library entry. This edits
 * the library row itself, not a per-singer assignment - the change is visible
 * to every singer already using this entry, and gets pushed to the request
 * server so search results there stay in sync.
 */
class DlgEditStreamSong : public QDialog {
Q_OBJECT

public:
    explicit DlgEditStreamSong(const QString &artist, const QString &title, QWidget *parent = nullptr);
    ~DlgEditStreamSong() override;

    [[nodiscard]] QString artist() const;
    [[nodiscard]] QString title() const;

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

private:
    Ui::DlgEditStreamSong *ui;
};

#endif // DLGEDITSTREAMSONG_H
