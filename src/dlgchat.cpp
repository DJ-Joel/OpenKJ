#include "dlgchat.h"
#include "ui_dlgchat.h"
#include <QDateTime>
#include <QMessageBox>

DlgChat::DlgChat(OKJSongbookAPI &songbookApi, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgChat),
    m_songbookApi(songbookApi)
{
    ui->setupUi(this);
    connect(&m_songbookApi, &OKJSongbookAPI::chatMessagesChanged, this, &DlgChat::messagesUpdated);
    updateReplyTargetUi();
}

DlgChat::~DlgChat()
{
    delete ui;
}

void DlgChat::messagesUpdated(const OkjsChatMessages &messages)
{
    m_messages = messages;
    rebuildList();
}

void DlgChat::rebuildList()
{
    int previouslySelected = selectedMessageId();
    ui->listMessages->clear();
    for (const auto &msg : m_messages)
    {
        QString time = QDateTime::fromSecsSinceEpoch(msg.time).toString("hh:mm");
        QString who = msg.fromKj ? QString("KJ -> %1").arg(msg.username) : msg.username;
        QString text = msg.messageText;
        if (msg.hidden)
            text = "[hidden] " + text;
        auto *item = new QListWidgetItem(QString("[%1] %2: %3").arg(time, who, text));
        item->setData(Qt::UserRole, msg.messageId);
        item->setData(Qt::UserRole + 1, msg.singerId);
        item->setData(Qt::UserRole + 2, msg.username);
        item->setData(Qt::UserRole + 3, msg.muted);
        item->setData(Qt::UserRole + 4, msg.hidden);
        if (msg.hidden)
            item->setForeground(Qt::gray);
        else if (msg.fromKj)
            item->setForeground(Qt::darkGreen);
        ui->listMessages->addItem(item);
        if (msg.messageId == previouslySelected)
            ui->listMessages->setCurrentItem(item);
    }
    ui->listMessages->scrollToBottom();
}

int DlgChat::selectedMessageId() const
{
    auto *item = ui->listMessages->currentItem();
    if (!item)
        return 0;
    return item->data(Qt::UserRole).toInt();
}

void DlgChat::on_listMessages_itemSelectionChanged()
{
    auto *item = ui->listMessages->currentItem();
    if (!item)
    {
        m_replyTargetSingerId = 0;
        m_replyTargetName.clear();
    }
    else
    {
        m_replyTargetSingerId = item->data(Qt::UserRole + 1).toInt();
        m_replyTargetName = item->data(Qt::UserRole + 2).toString();
        bool muted = item->data(Qt::UserRole + 3).toBool();
        bool hidden = item->data(Qt::UserRole + 4).toBool();
        ui->btnMute->setText(muted ? tr("Unmute singer") : tr("Mute singer"));
        ui->btnHide->setText(hidden ? tr("Unhide message") : tr("Hide message"));
    }
    updateReplyTargetUi();
}

void DlgChat::updateReplyTargetUi()
{
    bool haveTarget = m_replyTargetSingerId > 0;
    ui->btnSend->setEnabled(haveTarget);
    ui->btnHide->setEnabled(haveTarget);
    ui->btnMute->setEnabled(haveTarget);
    if (haveTarget)
        ui->lblReplyTarget->setText(tr("Replying to: %1").arg(m_replyTargetName));
    else
        ui->lblReplyTarget->setText(tr("Select a message to reply to it"));
}

void DlgChat::on_btnSend_clicked()
{
    if (m_replyTargetSingerId <= 0)
        return;
    QString text = ui->lineEditReply->text().trimmed();
    if (text.isEmpty())
        return;
    m_songbookApi.sendChatReply(m_replyTargetSingerId, text);
    ui->lineEditReply->clear();
}

void DlgChat::on_btnHide_clicked()
{
    auto *item = ui->listMessages->currentItem();
    if (!item)
        return;
    int messageId = item->data(Qt::UserRole).toInt();
    bool currentlyHidden = item->data(Qt::UserRole + 4).toBool();
    m_songbookApi.setChatMessageHidden(messageId, !currentlyHidden);
}

void DlgChat::on_btnMute_clicked()
{
    auto *item = ui->listMessages->currentItem();
    if (!item)
        return;
    int singerId = item->data(Qt::UserRole + 1).toInt();
    QString username = item->data(Qt::UserRole + 2).toString();
    bool currentlyMuted = item->data(Qt::UserRole + 3).toBool();
    if (!currentlyMuted)
    {
        auto result = QMessageBox::question(this, tr("Mute singer"),
                            tr("Mute %1? They will still be able to read this conversation, "
                               "but won't be able to send any more messages.").arg(username),
                            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (result != QMessageBox::Yes)
            return;
    }
    m_songbookApi.setSingerMuted(singerId, !currentlyMuted);
}

void DlgChat::on_btnClear_clicked()
{
    auto result = QMessageBox::question(this, tr("Clear chat"),
                        tr("Delete all chat messages? This can't be undone.\n\n"
                           "Muted singers will stay muted."),
                        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (result != QMessageBox::Yes)
        return;
    m_songbookApi.clearChat();
}

void DlgChat::on_btnClose_clicked()
{
    close();
}
