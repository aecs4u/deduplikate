#ifndef DUPLICATEMODEL_H
#define DUPLICATEMODEL_H

#include <QAbstractItemModel>
#include <QList>
#include "duplicatefinder.h"

class DuplicateModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit DuplicateModel(QObject *parent = nullptr);

    void setResults(const QList<DuplicateFinder::DuplicateGroup> &results);
    void clear();

    void selectAll();
    void selectNone();
    void invertSelection();

    QList<QString> getSelectedFiles() const;

    // QAbstractItemModel interface
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

private:
    struct FileItem {
        QString path;
        QString fileName;
        QString directory;
        quint64 size;
        quint64 modifiedDate;
        QString hash;
        bool checked;
        int groupIndex;
    };

    QList<DuplicateFinder::DuplicateGroup> m_groups;
    QList<QList<FileItem>> m_items; // Items organized by group

    QString formatSize(quint64 size) const;
    QString formatDate(quint64 timestamp) const;
};

#endif // DUPLICATEMODEL_H
