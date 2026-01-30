#include "duplicatefinder.h"
#include "czkawka_bridge/czkawka_bridge.h"
#include <QDebug>

DuplicateFinder::DuplicateFinder(QObject *parent)
    : QObject(parent)
    , m_scanThread(nullptr)
    , m_groupCount(0)
    , m_wastedSpace(0)
{
}

DuplicateFinder::~DuplicateFinder()
{
    if (m_scanThread) {
        m_scanThread->stop();
        m_scanThread->wait();
        delete m_scanThread;
    }
}

void DuplicateFinder::startScan(const ScanParameters &params)
{
    if (m_scanThread && m_scanThread->isRunning()) {
        qWarning() << "Scan already in progress";
        return;
    }

    if (m_scanThread) {
        delete m_scanThread;
    }

    m_scanThread = new ScanThread(params, this);

    connect(m_scanThread, &ScanThread::progress, this, &DuplicateFinder::scanProgress);
    connect(m_scanThread, &ScanThread::finished, this, [this]() {
        m_results = m_scanThread->getResults();
        m_groupCount = m_scanThread->getGroupCount();
        m_wastedSpace = m_scanThread->getWastedSpace();

        Q_EMIT resultsReady(m_groupCount, m_wastedSpace);
        Q_EMIT scanFinished(true);
    });

    Q_EMIT scanStarted();
    m_scanThread->start();
}

void DuplicateFinder::stopScan()
{
    if (m_scanThread && m_scanThread->isRunning()) {
        m_scanThread->stop();
    }
}

QList<DuplicateFinder::DuplicateGroup> DuplicateFinder::getResults() const
{
    return m_results;
}

int DuplicateFinder::getGroupCount() const
{
    return m_groupCount;
}

quint64 DuplicateFinder::getWastedSpace() const
{
    return m_wastedSpace;
}

// ScanThread implementation

DuplicateFinder::ScanThread::ScanThread(const DuplicateFinder::ScanParameters &params, QObject *parent)
    : QThread(parent)
    , m_params(params)
    , m_finder(nullptr)
    , m_groupCount(0)
    , m_wastedSpace(0)
    , m_shouldStop(false)
{
}

DuplicateFinder::ScanThread::~ScanThread()
{
    if (m_finder) {
        czkawka_duplicate_finder_free(m_finder);
    }
}

void DuplicateFinder::ScanThread::stop()
{
    m_shouldStop = true;
    if (m_finder) {
        czkawka_duplicate_finder_stop(m_finder);
    }
}

QList<DuplicateFinder::DuplicateGroup> DuplicateFinder::ScanThread::getResults() const
{
    return m_results;
}

int DuplicateFinder::ScanThread::getGroupCount() const
{
    return m_groupCount;
}

quint64 DuplicateFinder::ScanThread::getWastedSpace() const
{
    return m_wastedSpace;
}

void DuplicateFinder::ScanThread::run()
{
    // Create finder
    m_finder = czkawka_duplicate_finder_new(
        static_cast<CCheckingMethod>(m_params.checkMethod),
        static_cast<CHashType>(m_params.hashType),
        m_params.ignoreHardLinks,
        m_params.useCache
    );

    if (!m_finder) {
        qWarning() << "Failed to create duplicate finder";
        return;
    }

    // Configure finder
    czkawka_duplicate_finder_set_recursive(m_finder, m_params.recursive);
    czkawka_duplicate_finder_set_min_size(m_finder, m_params.minSize);
    if (m_params.maxSize > 0) {
        czkawka_duplicate_finder_set_max_size(m_finder, m_params.maxSize);
    }

    // Add directories
    for (const QString &path : m_params.includePaths) {
        if (m_shouldStop) return;
        czkawka_duplicate_finder_add_directory(m_finder, path.toUtf8().constData());
    }

    for (const QString &path : m_params.excludePaths) {
        if (m_shouldStop) return;
        czkawka_duplicate_finder_add_excluded_directory(m_finder, path.toUtf8().constData());
    }

    // Start scan
    qDebug() << "Starting duplicate scan...";
    bool success = czkawka_duplicate_finder_search(m_finder);

    if (!success || m_shouldStop) {
        qWarning() << "Scan failed or was stopped";
        return;
    }

    // Get results
    m_groupCount = czkawka_duplicate_finder_get_group_count(m_finder);
    m_wastedSpace = czkawka_duplicate_finder_get_wasted_space(m_finder);

    qDebug() << "Found" << m_groupCount << "duplicate groups";
    qDebug() << "Wasted space:" << m_wastedSpace << "bytes";

    // Fetch all groups
    for (int i = 0; i < m_groupCount; ++i) {
        if (m_shouldStop) return;

        const CDuplicateEntry *entries = nullptr;
        size_t count = 0;

        if (czkawka_duplicate_finder_get_group(m_finder, i, &entries, &count)) {
            DuplicateGroup group;

            for (size_t j = 0; j < count; ++j) {
                DuplicateEntry entry;
                entry.path = QString::fromUtf8(entries[j].path);
                entry.size = entries[j].size;
                entry.modifiedDate = entries[j].modified_date;
                entry.hash = QString::fromUtf8(entries[j].hash);

                group.entries.append(entry);
            }

            m_results.append(group);

            // Free the entries
            czkawka_duplicate_entries_free(const_cast<CDuplicateEntry*>(entries), count);
        }

        Q_EMIT progress(i + 1, m_groupCount);
    }

    qDebug() << "Scan completed, processed" << m_results.size() << "groups";
}
