#ifndef DLGCHAT_H
#define DLGCHAT_H

#include <QDialog>
#include <QListWidgetItem>
#include "okjsongbookapi.h"
#include "settings.h"

namespace Ui {
class DlgChat;
}

/**
 * @brief The KJ's view of singer chat. Shows every singer's messages as one
 * flat chronological stream (singers only ever see their own thread), with
 * controls to reply to whoever is selected, hide individual messages, and
 * mute a singer.
 */
class DlgChat : public QDialog
{
    Q_OBJECT

public:
    explicit DlgChat(OKJSongbookAPI &songbookApi, QWidget *parent = nullptr);
    ~DlgChat() override;

public slots:
    void messagesUpdated(const OkjsChatMessages &messages);

private slots:
    void on_btnSend_clicked();
    void on_btnHide_clicked();
    void on_btnMute_clicked();
    void on_btnClear_clicked();
    void on_btnClose_clicked();
    void on_listMessages_itemSelectionChanged();

private:
    Ui::DlgChat *ui;
    OKJSongbookAPI &m_songbookApi;
    Settings m_settings;
    OkjsChatMessages m_messages;
    // Who a reply will be sent to - set by selecting any message from them.
    int m_replyTargetSingerId{0};
    QString m_replyTargetName;

    void rebuildList();
    void updateReplyTargetUi();
    [[nodiscard]] int selectedMessageId() const;
};

#endif // DLGCHAT_H
