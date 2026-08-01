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

#include "okjtypes.h"
#include "src/models/tablemodelstreamsongs.h"
#include <QSqlQuery>
#include <QSqlError>
#include <utility>
#include <spdlog/spdlog.h>

std::ostream & operator<<(std::ostream& os, const okj::RotationSinger& s)
{
    return os
        << " RotationSinger(id: " << s.id
        << " position: " << s.position
        << " name: " << s.name.toStdString()
        << " regular: " << s.regular
        << " addTS: " << s.addTs.toString().toStdString()
        << " valid: " << s.valid
        << ")";
}


namespace okj {

    QString RotationSinger::nextSongPath() const {
        QSqlQuery query;
        query.prepare(
                "SELECT dbsongs.path FROM dbsongs,queuesongs WHERE queuesongs.singer = :singerid AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1");
        query.bindValue(":singerid", id);
        query.exec();
        if (auto lastError = query.lastError(); lastError.type() != QSqlError::NoError)
            m_logger->error("{} DB error! Error while querying the db on disk! Error: {}", loggingPrefix(),
                            lastError.text().toStdString());
        if (query.first())
            return query.value(0).toString();
        // No unplayed queue song - fall back to a saved stream entry.
        // The URL is returned as-is; callers resolve it via yt-dlp before play.
        return TableModelStreamSongs::nextUnplayedForSinger(name).url;
    }

    QString RotationSinger::nextSongArtist() const {
        QSqlQuery query;
        query.prepare(
                "SELECT dbsongs.artist FROM dbsongs,queuesongs WHERE queuesongs.singer = :singerid AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1");
        query.bindValue(":singerid", id);
        query.exec();
        if (auto lastError = query.lastError(); lastError.type() != QSqlError::NoError)
            m_logger->error("{} DB error! Error while querying the db on disk! Error: {}", loggingPrefix(),
                            lastError.text().toStdString());
        if (query.first())
            return query.value(0).toString();
        return TableModelStreamSongs::nextUnplayedForSinger(name).artist;
    }

    QString RotationSinger::nextSongTitle() const {
        QSqlQuery query;
        query.prepare(
                "SELECT dbsongs.title FROM dbsongs,queuesongs WHERE queuesongs.singer = :singerid AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1");
        query.bindValue(":singerid", id);
        query.exec();
        if (auto lastError = query.lastError(); lastError.type() != QSqlError::NoError)
            m_logger->error("{} DB error! Error while querying the db on disk! Error: {}", loggingPrefix(),
                            lastError.text().toStdString());
        if (query.first())
            return query.value(0).toString();
        return TableModelStreamSongs::nextUnplayedForSinger(name).title;
    }

    QString RotationSinger::nextSongArtistTitle() const {
        QSqlQuery query;
        query.prepare(
                "SELECT dbsongs.artist, dbsongs.title FROM dbsongs,queuesongs WHERE queuesongs.singer = :singerid AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1");
        query.bindValue(":singerid", id);
        query.exec();
        if (auto lastError = query.lastError(); lastError.type() != QSqlError::NoError)
            m_logger->error("{} DB error! Error while querying the db on disk! Error: {}", loggingPrefix(),
                            lastError.text().toStdString());
        if (query.first())
            return query.value(0).toString() + " - " + query.value(1).toString();
        if (auto stream = TableModelStreamSongs::nextUnplayedForSinger(name); stream.id != 0)
            return stream.artist + " - " + stream.title;
        return " - empty - ";
    }

    QString RotationSinger::nextSongSongId() const {
        QSqlQuery query;
        query.prepare(
                "SELECT dbsongs.discid FROM dbsongs,queuesongs WHERE queuesongs.singer = :singerid AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1");
        query.bindValue(":singerid", id);
        query.exec();
        if (auto lastError = query.lastError(); lastError.type() != QSqlError::NoError)
            m_logger->error("{} DB error! Error while querying the db on disk! Error: {}", loggingPrefix(),
                            lastError.text().toStdString());
        if (query.first())
            return query.value(0).toString();
        return {};
    }

    int RotationSinger::nextSongDurationSecs() const {
        QSqlQuery query;
        query.prepare(
                "SELECT dbsongs.duration FROM dbsongs,queuesongs WHERE queuesongs.singer = :singerid AND queuesongs.played = 0 AND dbsongs.songid = queuesongs.song ORDER BY position LIMIT 1");
        query.bindValue(":singerid", id);
        query.exec();
        if (auto lastError = query.lastError(); lastError.type() != QSqlError::NoError)
            m_logger->error("{} DB error! Error while querying the db on disk! Error: {}", loggingPrefix(),
                            lastError.text().toStdString());
        if (query.first())
            return (query.value(0).toInt() / 1000) + m_settings->estimationSingerPad();
        // Fall back to a stream entry. Its duration is fetched via yt-dlp when
        // the entry is added, so rotation estimates stay meaningful. If we
        // never got one, treat it like an empty singer rather than zero.
        if (auto stream = TableModelStreamSongs::nextUnplayedForSinger(name); stream.id != 0) {
            if (stream.duration > 0)
                return stream.duration + m_settings->estimationSingerPad();
            return m_settings->estimationEmptySongLength() + m_settings->estimationSingerPad();
        }
        if (!m_settings->estimationSkipEmptySingers())
            return m_settings->estimationEmptySongLength() + m_settings->estimationSingerPad();
        return 0;
    }

    int RotationSinger::nextSongKeyChg() const {
        QSqlQuery query;
        query.prepare(
                "SELECT keychg FROM queuesongs WHERE singer = :singerid AND played = 0 ORDER BY position LIMIT 1");
        query.bindValue(":singerid", id);
        query.exec();
        if (auto lastError = query.lastError(); lastError.type() != QSqlError::NoError)
            m_logger->error("{} DB error! Error while querying the db on disk! Error: {}", loggingPrefix(),
                            lastError.text().toStdString());
        if (query.first())
            return query.value(0).toInt();
        return 0;
    }

    int RotationSinger::nextSongQueueId() const {
        QSqlQuery query;
        query.prepare(
                "SELECT qsongid FROM queuesongs WHERE singer = :singerid AND played = 0 ORDER BY position LIMIT 1");
        query.bindValue(":singerid", id);
        query.exec();
        if (auto lastError = query.lastError(); lastError.type() != QSqlError::NoError)
            m_logger->error("{} DB error! Error while querying the db on disk! Error: {}", loggingPrefix(),
                            lastError.text().toStdString());
        if (query.first())
            return query.value(0).toInt();
        return -1;
    }

    int RotationSinger::numSongsSung() const {
        QSqlQuery query;
        query.prepare("SELECT COUNT(qsongid) FROM queuesongs WHERE singer = :singerid AND played = true");
        query.bindValue(":singerid", id);
        query.exec();
        if (auto lastError = query.lastError(); lastError.type() != QSqlError::NoError)
            m_logger->error("{} DB error! Error while querying the db on disk! Error: {}", loggingPrefix(),
                            lastError.text().toStdString());
        if (query.first())
            return query.value(0).toInt();
        return -1;
    }

    int RotationSinger::numSongsUnsung() const {
        QSqlQuery query;
        query.prepare("SELECT COUNT(qsongid) FROM queuesongs WHERE singer = :singerid AND played = false");
        query.bindValue(":singerid", id);
        query.exec();
        if (auto lastError = query.lastError(); lastError.type() != QSqlError::NoError)
            m_logger->error("{} DB error! Error while querying the db on disk! Error: {}", loggingPrefix(),
                            lastError.text().toStdString());
        // Unplayed stream entries count too - otherwise a singer whose only
        // pending song is a stream link looks like they have nothing queued.
        int streamUnsung = TableModelStreamSongs::unplayedCountForSinger(name);
        if (query.first())
            return query.value(0).toInt() + streamUnsung;
        return (streamUnsung > 0) ? streamUnsung : -1;
    }

    RotationSinger::RotationSinger() {
        m_logger = spdlog::get("logger");
        m_settings = std::make_shared<Settings>();
    }

    RotationSinger::RotationSinger(int id, QString name, int position, bool regular, QDateTime addTs, bool valid)
            : id(id), name(std::move(name)), position(position), regular(regular), addTs(std::move(addTs)),
              valid(valid) {
        m_logger = spdlog::get("logger");
        m_settings = std::make_shared<Settings>();
    }

    RotationSinger::RotationSinger(const RotationSinger &r1)
            : id(r1.id), name(r1.name), position(r1.position), regular(r1.regular), addTs(r1.addTs), valid(r1.valid) {
        m_logger = spdlog::get("logger");
        m_settings = std::make_shared<Settings>();
    }


}
