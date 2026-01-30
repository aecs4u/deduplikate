#ifndef DUPLICATEFINDER_H
#define DUPLICATEFINDER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QThread>

struct CzkawkaDuplicateFinder;

class DuplicateFinder : public QObject
{
    Q_OBJECT

public:
    struct ScanParameters {
        int checkMethod;          // 0=Hash, 1=Name, 2=Size, 3=SizeName
        int hashType;             // 0=Blake3, 1=Crc32, 2=Xxh3
        bool recursive;
        bool ignoreHardLinks;
        bool useCache;
        quint64 minSize;
        quint64 maxSize;
        QStringList includePaths;
        QStringList excludePaths;
    };

    struct DuplicateEntry {
        QString path;
        quint64 size;
        quint64 modifiedDate;
        QString hash;
    };

    struct DuplicateGroup {
        QList<DuplicateEntry> entries;
    };

    explicit DuplicateFinder(QObject *parent = nullptr);
    ~DuplicateFinder();

    void startScan(const ScanParameters &params);
    void stopScan();

    QList<DuplicateGroup> getResults() const;
    int getGroupCount() const;
    quint64 getWastedSpace() const;

Q_SIGNALS:
    void scanStarted();
    void scanProgress(int current, int total);
    void scanFinished(bool success);
    void resultsReady(int groupCount, quint64 wastedSpace);

private:
    class ScanThread;
    ScanThread *m_scanThread;
    QList<DuplicateGroup> m_results;
    int m_groupCount;
    quint64 m_wastedSpace;
};

// Worker thread for scanning
class DuplicateFinder::ScanThread : public QThread
{
    Q_OBJECT

public:
    ScanThread(const DuplicateFinder::ScanParameters &params, QObject *parent = nullptr);
    ~ScanThread();

    void stop();
    QList<DuplicateFinder::DuplicateGroup> getResults() const;
    int getGroupCount() const;
    quint64 getWastedSpace() const;

Q_SIGNALS:
    void progress(int current, int total);

protected:
    void run() override;

private:
    DuplicateFinder::ScanParameters m_params;
    CzkawkaDuplicateFinder *m_finder;
    QList<DuplicateFinder::DuplicateGroup> m_results;
    int m_groupCount;
    quint64 m_wastedSpace;
    bool m_shouldStop;
};

#endif // DUPLICATEFINDER_H
