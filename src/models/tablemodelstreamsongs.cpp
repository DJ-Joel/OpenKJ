#include "tablemodelstreamsongs.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QColor>

TableModelStreamSongs::TableModelStreamSongs(QObject *parent) : QAbstractTableModel(parent) {
    m_logger = spdlog::get("logger");
}

int TableModelStreamSongs::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return static_cast<int>(m_songs.size());
}

int TableModelStreamSongs::columnCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return 4;
}

QVariant TableModelStreamSongs::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
        case COL_ID:
            return "ID";
        case COL_ARTIST:
            return "Artist";
        case COL_TITLE:
            return "Title";
        case COL_URL:
            return "URL";
        default:
            return {};
    }
}

QVariant TableModelStreamSongs::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= static_cast<int>(m_songs.size()))
        return {};
    const auto &song = m_songs.at(index.row());
    if (role == Qt::UserRole)
        return QVariant::fromValue(song);
    if (role == Qt::ForegroundRole && song.played)
        return QColor(Qt::gray);
    if (role != Qt::DisplayRole)
        return {};
    switch (index.column()) {
        case COL_ID:
            return song.id;
        case COL_ARTIST:
            return song.artist;
        case COL_TITLE:
            return song.title;
        case COL_URL:
            return song.url;
        default:
            return {};
    }
}

int TableModelStreamSongs::getHistorySingerId(const QString &name) {
    QSqlQuery query;
    query.prepare("SELECT id FROM historySingers WHERE name = :name LIMIT 1");
    query.bindValue(":name", name);
    query.exec();
    if (query.next())
        return query.value(0).toInt();
    return -1;
}

int TableModelStreamSongs::addHistorySinger(const QString &name) {
    QSqlQuery query;
    query.prepare("INSERT INTO historySingers (name) VALUES( :name )");
    query.bindValue(":name", name);
    query.exec();
    return query.lastInsertId().toInt();
}

void TableModelStreamSongs::loadSinger(const QString &singerName) {
    m_currentSingerName = singerName;
    m_currentHistorySingerId = getHistorySingerId(singerName);
    refresh();
}

void TableModelStreamSongs::refresh() {
    emit layoutAboutToBeChanged();
    m_songs.clear();
    if (m_currentHistorySingerId != -1) {
        QSqlQuery query;
        query.prepare("SELECT id, historySinger, artist, title, url, duration, played, position "
                      "FROM streamSongs WHERE historySinger = :hs ORDER BY position");
        query.bindValue(":hs", m_currentHistorySingerId);
        query.exec();
        if (auto error = query.lastError(); error.type() != QSqlError::NoError)
            m_logger->error("{} DB error: {}", m_loggingPrefix, error.text().toStdString());
        while (query.next()) {
            okj::StreamSong song;
            song.id = query.value(0).toInt();
            song.historySinger = query.value(1).toInt();
            song.artist = query.value(2).toString();
            song.title = query.value(3).toString();
            song.url = query.value(4).toString();
            song.duration = query.value(5).toInt();
            song.played = query.value(6).toBool();
            song.position = query.value(7).toInt();
            m_songs.push_back(song);
        }
    }
    emit layoutChanged();
}

int TableModelStreamSongs::addSong(const QString &singerName, const QString &artist, const QString &title,
                                   const QString &url, int duration) {
    int historySingerId = getHistorySingerId(singerName);
    if (historySingerId == -1)
        historySingerId = addHistorySinger(singerName);
    if (historySingerId == -1) {
        m_logger->error("{} Unable to resolve or create a history singer for '{}'", m_loggingPrefix,
                        singerName.toStdString());
        return -1;
    }

    int position = 0;
    QSqlQuery posQuery;
    posQuery.prepare("SELECT COALESCE(MAX(position), -1) + 1 FROM streamSongs WHERE historySinger = :hs");
    posQuery.bindValue(":hs", historySingerId);
    posQuery.exec();
    if (posQuery.next())
        position = posQuery.value(0).toInt();

    QSqlQuery query;
    query.prepare("INSERT INTO streamSongs (historySinger, artist, title, url, duration, played, position) "
                  "VALUES (:hs, :artist, :title, :url, :duration, 0, :position)");
    query.bindValue(":hs", historySingerId);
    query.bindValue(":artist", artist);
    query.bindValue(":title", title);
    query.bindValue(":url", url);
    query.bindValue(":duration", duration);
    query.bindValue(":position", position);
    query.exec();
    if (auto error = query.lastError(); error.type() != QSqlError::NoError) {
        m_logger->error("{} DB error adding stream song: {}", m_loggingPrefix, error.text().toStdString());
        return -1;
    }
    int newId = query.lastInsertId().toInt();
    if (singerName == m_currentSingerName) {
        m_currentHistorySingerId = historySingerId;
        refresh();
    }
    return newId;
}

void TableModelStreamSongs::deleteSong(int streamSongId) {
    QSqlQuery query;
    query.prepare("DELETE FROM streamSongs WHERE id = :id");
    query.bindValue(":id", streamSongId);
    query.exec();
    if (auto error = query.lastError(); error.type() != QSqlError::NoError)
        m_logger->error("{} DB error deleting stream song: {}", m_loggingPrefix, error.text().toStdString());
    refresh();
}

void TableModelStreamSongs::setPlayed(int streamSongId, bool played) {
    QSqlQuery query;
    query.prepare("UPDATE streamSongs SET played = :played WHERE id = :id");
    query.bindValue(":played", played ? 1 : 0);
    query.bindValue(":id", streamSongId);
    query.exec();
    refresh();
}

void TableModelStreamSongs::clearAllPlayed() {
    QSqlQuery query;
    query.exec("UPDATE streamSongs SET played = 0");
}

okj::StreamSong TableModelStreamSongs::getSong(int streamSongId) const {
    for (const auto &song : m_songs) {
        if (song.id == streamSongId)
            return song;
    }
    return {};
}

okj::StreamSong TableModelStreamSongs::nextUnplayedForSinger(const QString &singerName) {
    okj::StreamSong song;
    QSqlQuery query;
    query.prepare("SELECT s.id, s.historySinger, s.artist, s.title, s.url, s.duration, s.played, s.position "
                  "FROM streamSongs s INNER JOIN historySingers h ON h.id = s.historySinger "
                  "WHERE h.name = :name AND s.played = 0 ORDER BY s.position LIMIT 1");
    query.bindValue(":name", singerName);
    query.exec();
    if (query.next()) {
        song.id = query.value(0).toInt();
        song.historySinger = query.value(1).toInt();
        song.artist = query.value(2).toString();
        song.title = query.value(3).toString();
        song.url = query.value(4).toString();
        song.duration = query.value(5).toInt();
        song.played = query.value(6).toBool();
        song.position = query.value(7).toInt();
    }
    return song;
}

int TableModelStreamSongs::unplayedCountForSinger(const QString &singerName) {
    QSqlQuery query;
    query.prepare("SELECT COUNT(s.id) FROM streamSongs s "
                  "INNER JOIN historySingers h ON h.id = s.historySinger "
                  "WHERE h.name = :name AND s.played = 0");
    query.bindValue(":name", singerName);
    query.exec();
    if (query.next())
        return query.value(0).toInt();
    return 0;
}
