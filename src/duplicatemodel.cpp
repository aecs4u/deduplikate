#include "duplicatemodel.h"
#include <QFileInfo>
#include <QDateTime>
#include <QIcon>
#include <QFont>

DuplicateModel::DuplicateModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

void DuplicateModel::setResults(const QList<DuplicateFinder::DuplicateGroup> &results)
{
    beginResetModel();

    m_groups = results;
    m_items.clear();

    for (int groupIdx = 0; groupIdx < m_groups.size(); ++groupIdx) {
        const auto &group = m_groups[groupIdx];
        QList<FileItem> groupItems;

        for (const auto &entry : group.entries) {
            QFileInfo fileInfo(entry.path);

            FileItem item;
            item.path = entry.path;
            item.fileName = fileInfo.fileName();
            item.directory = fileInfo.absolutePath();
            item.size = entry.size;
            item.modifiedDate = entry.modifiedDate;
            item.hash = entry.hash;
            item.checked = false;
            item.groupIndex = groupIdx;

            groupItems.append(item);
        }

        m_items.append(groupItems);
    }

    endResetModel();
}

void DuplicateModel::clear()
{
    beginResetModel();
    m_groups.clear();
    m_items.clear();
    endResetModel();
}

void DuplicateModel::selectAll()
{
    for (auto &groupItems : m_items) {
        for (auto &item : groupItems) {
            item.checked = true;
        }
    }
    Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, 0));
}

void DuplicateModel::selectNone()
{
    for (auto &groupItems : m_items) {
        for (auto &item : groupItems) {
            item.checked = false;
        }
    }
    Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, 0));
}

void DuplicateModel::invertSelection()
{
    for (auto &groupItems : m_items) {
        for (auto &item : groupItems) {
            item.checked = !item.checked;
        }
    }
    Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, 0));
}

QList<QString> DuplicateModel::getSelectedFiles() const
{
    QList<QString> selectedFiles;
    for (const auto &groupItems : m_items) {
        for (const auto &item : groupItems) {
            if (item.checked) {
                selectedFiles.append(item.path);
            }
        }
    }
    return selectedFiles;
}

QModelIndex DuplicateModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    if (!parent.isValid()) {
        // Top-level item (group header)
        if (row >= 0 && row < m_items.size()) {
            return createIndex(row, column, quintptr(row) << 32);
        }
    } else {
        // Child item (file in group)
        int groupIdx = parent.row();
        if (groupIdx >= 0 && groupIdx < m_items.size()) {
            if (row >= 0 && row < m_items[groupIdx].size()) {
                return createIndex(row, column, (quintptr(groupIdx) << 32) | (row + 1));
            }
        }
    }

    return QModelIndex();
}

QModelIndex DuplicateModel::parent(const QModelIndex &child) const
{
    if (!child.isValid()) {
        return QModelIndex();
    }

    quintptr id = child.internalId();
    int childRow = id & 0xFFFFFFFF;

    if (childRow == 0) {
        // Top-level item has no parent
        return QModelIndex();
    } else {
        // Child item, return group header as parent
        int groupIdx = id >> 32;
        return createIndex(groupIdx, 0, quintptr(groupIdx) << 32);
    }
}

int DuplicateModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        // Root level: return number of groups
        return m_items.size();
    } else {
        quintptr id = parent.internalId();
        int childRow = id & 0xFFFFFFFF;

        if (childRow == 0) {
            // Group header: return number of files in this group
            int groupIdx = id >> 32;
            if (groupIdx >= 0 && groupIdx < m_items.size()) {
                return m_items[groupIdx].size();
            }
        }
    }

    return 0;
}

int DuplicateModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 5; // Checkbox, Name, Size, Modified, Path
}

QVariant DuplicateModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    quintptr id = index.internalId();
    int childRow = id & 0xFFFFFFFF;

    if (childRow == 0) {
        // Group header
        int groupIdx = id >> 32;
        if (groupIdx >= 0 && groupIdx < m_items.size()) {
            if (role == Qt::DisplayRole && index.column() == 1) {
                return QStringLiteral("Group %1 (%2 files)")
                    .arg(groupIdx + 1)
                    .arg(m_items[groupIdx].size());
            } else if (role == Qt::FontRole) {
                QFont font;
                font.setBold(true);
                return font;
            } else if (role == Qt::BackgroundRole) {
                return QColor(230, 230, 230);
            }
        }
    } else {
        // File item
        int groupIdx = id >> 32;
        int fileIdx = childRow - 1;

        if (groupIdx >= 0 && groupIdx < m_items.size() &&
            fileIdx >= 0 && fileIdx < m_items[groupIdx].size()) {

            const FileItem &item = m_items[groupIdx][fileIdx];

            if (role == Qt::DisplayRole) {
                switch (index.column()) {
                case 0: return QString(); // Checkbox column
                case 1: return item.fileName;
                case 2: return formatSize(item.size);
                case 3: return formatDate(item.modifiedDate);
                case 4: return item.directory;
                default: return QVariant();
                }
            } else if (role == Qt::CheckStateRole && index.column() == 0) {
                return item.checked ? Qt::Checked : Qt::Unchecked;
            } else if (role == Qt::ToolTipRole) {
                return item.path;
            }
        }
    }

    return QVariant();
}

QVariant DuplicateModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case 0: return QString();
        case 1: return tr("Name");
        case 2: return tr("Size");
        case 3: return tr("Modified");
        case 4: return tr("Directory");
        default: return QVariant();
        }
    }

    return QVariant();
}

Qt::ItemFlags DuplicateModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    quintptr id = index.internalId();
    int childRow = id & 0xFFFFFFFF;

    if (childRow == 0) {
        // Group header
        return Qt::ItemIsEnabled;
    } else {
        // File item
        Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        if (index.column() == 0) {
            flags |= Qt::ItemIsUserCheckable;
        }
        return flags;
    }
}

bool DuplicateModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || role != Qt::CheckStateRole || index.column() != 0) {
        return false;
    }

    quintptr id = index.internalId();
    int childRow = id & 0xFFFFFFFF;

    if (childRow > 0) {
        int groupIdx = id >> 32;
        int fileIdx = childRow - 1;

        if (groupIdx >= 0 && groupIdx < m_items.size() &&
            fileIdx >= 0 && fileIdx < m_items[groupIdx].size()) {

            m_items[groupIdx][fileIdx].checked = (value.toInt() == Qt::Checked);
            Q_EMIT dataChanged(index, index);
            return true;
        }
    }

    return false;
}

QString DuplicateModel::formatSize(quint64 size) const
{
    if (size > 1024 * 1024 * 1024) {
        return QString::number(size / (1024.0 * 1024.0 * 1024.0), 'f', 2) + QLatin1String(" GB");
    } else if (size > 1024 * 1024) {
        return QString::number(size / (1024.0 * 1024.0), 'f', 2) + QLatin1String(" MB");
    } else if (size > 1024) {
        return QString::number(size / 1024.0, 'f', 2) + QLatin1String(" KB");
    } else {
        return QString::number(size) + QLatin1String(" bytes");
    }
}

QString DuplicateModel::formatDate(quint64 timestamp) const
{
    QDateTime dateTime = QDateTime::fromSecsSinceEpoch(timestamp);
    return dateTime.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
}
