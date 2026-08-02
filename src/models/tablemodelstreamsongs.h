#ifndef TABLEMODELSTREAMSONGS_H
#define TABLEMODELSTREAMSONGS_H

#include <QAbstractTableModel>
#include <QString>
#include <optional>
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
 * Backed by two tables: streamLibrary (one row per unique song, shared across
 * every singer who wants it) and streamSongs (a thin per-singer assignment
 * pointing at a library row). This model presents the joined, flattened view
 * via okj::StreamSong - callers working with a specific singer's entries
 * don't need to know the library/assignment split exists underneath.
 *
 * Assignments are keyed to historySingers (by name), so a regular's saved
 * links come back on future nights. The played flag is cleared when the
 * rotation is cleared, so saved entries are ready to sing again next time.
 * Library rows are never deleted when an assignment is removed, so the
 * catalog only grows over time.
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

    // Case-insensitive exact match on artist+title against the shared
    // library, used to offer reusing an existing entry instead of creating a
    // duplicate. Returns nullopt if nothing matches.
    static std::optional<okj::StreamLibraryEntry> findLibraryMatch(const QString &artist, const QString &title);

    // Attaches an existing library entry to a singer. Returns the new
    // assignment's id, or -1 on failure.
    int attachExistingToSinger(const QString &singerName, int libraryId);

    // Creates a brand new library entry and attaches it to a singer in one
    // step. Returns the new assignment's id, or -1 on failure.
    int addNewSongForSinger(const QString &singerName, const QString &artist, const QString &title,
                             const QString &url, int duration);

    // Removes a singer's assignment only - the library entry (and any other
    // singer's assignment to it) is left alone.
    void deleteSong(int streamSongId);
    void setPlayed(int streamSongId, bool played = true);
    // Clears the played flag on every assignment - called when the rotation
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
    static int nextPositionForSinger(int historySingerId);
};

#endif // TABLEMODELSTREAMSONGS_H
