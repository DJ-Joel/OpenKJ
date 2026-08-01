#ifndef TABLEMODELSTREAMSONGS_H
#define TABLEMODELSTREAMSONGS_H

#include <QAbstractTableModel>
#include <QString>
#include <vector>
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/async_logger.h>
#include <spdlog/fmt/ostr.h>
#include "src/okjtypes.h"

/**
 * @brief Per-singer stream entries (e.g. YouTube links) for songs that aren't
 * in the karaoke library. These can't live in queueSongs because that table
 * joins against dbsongs and every row must map to a real karaoke file.
 *
 * Entries are keyed to historySingers (by name), so a regular's saved links
 * come back on future nights. The played flag is cleared when the rotation is
 * cleared, so saved entries are ready to sing again next time.
 */
class TableModelStreamSongs : public QAbstractTableModel {
Q_OBJECT

public:
    enum { COL_ID = 0, COL_ARTIST, COL_TITLE, COL_URL };

    explicit TableModelStreamSongs(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    // Loads by rotation singer name - resolves to the historySingers row.
    void loadSinger(const QString &singerName);
    void refresh();
    [[nodiscard]] QString currentSingerName() const { return m_currentSingerName; }

    // Returns the new row's id, or -1 on failure.
    int addSong(const QString &singerName, const QString &artist, const QString &title, const QString &url,
                int duration);
    void deleteSong(int streamSongId);
    void setPlayed(int streamSongId, bool played = true);
    // Clears the played flag on every stream song - called when the rotation
    // is cleared, so saved entries are available again.
    static void clearAllPlayed();

    [[nodiscard]] okj::StreamSong getSong(int streamSongId) const;

    // Next-unplayed lookups used by the rotation "next song" integration.
    // These are static/by-name because rotation singers are matched to stream
    // entries via the singer's name, not the rotation id.
    static okj::StreamSong nextUnplayedForSinger(const QString &singerName);
    static int unplayedCountForSinger(const QString &singerName);

private:
    std::vector<okj::StreamSong> m_songs;
    QString m_currentSingerName;
    int m_currentHistorySingerId{-1};
    std::string m_loggingPrefix{"[StreamSongsModel]"};
    std::shared_ptr<spdlog::logger> m_logger;

    static int getHistorySingerId(const QString &name);
    static int addHistorySinger(const QString &name);
};

#endif // TABLEMODELSTREAMSONGS_H
