#include <QtTest/QtTest>
#include "duplicatemodel.h"
#include "duplicatefinder.h"

class TestDuplicateModel : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Tree structure tests
    void testEmptyModel();
    void testSingleGroupWithFiles();
    void testMultipleGroups();
    void testParentChildRelationships();
    void testInvalidIndices();

    // Selection logic tests
    void testSelectAll();
    void testSelectNone();
    void testInvertSelection();
    void testIndividualCheckboxToggle();
    void testGetSelectedFiles();
    void testSelectionPersistence();

    // Data display tests
    void testColumnCount();
    void testHeaderData();
    void testSizeFormatting();
    void testDateFormatting();
    void testDisplayRole();
    void testTooltipRole();

    // Model operations tests
    void testSetResults();
    void testClear();
    void testRowCountTopLevel();
    void testRowCountChildren();
    void testDataChangeSignals();

private:
    DuplicateModel *model;

    QList<DuplicateFinder::DuplicateGroup> createTestData(int groups, int filesPerGroup);
};

void TestDuplicateModel::initTestCase()
{
    // One-time initialization
}

void TestDuplicateModel::cleanupTestCase()
{
    // One-time cleanup
}

void TestDuplicateModel::init()
{
    // Create fresh model before each test
    model = new DuplicateModel();
}

void TestDuplicateModel::cleanup()
{
    // Clean up after each test
    delete model;
    model = nullptr;
}

QList<DuplicateFinder::DuplicateGroup> TestDuplicateModel::createTestData(int groups, int filesPerGroup)
{
    QList<DuplicateFinder::DuplicateGroup> result;

    for (int g = 0; g < groups; ++g) {
        DuplicateFinder::DuplicateGroup group;

        for (int f = 0; f < filesPerGroup; ++f) {
            DuplicateFinder::DuplicateEntry entry;
            entry.path = QStringLiteral("/tmp/test/group%1/file%2.txt").arg(g).arg(f);
            entry.size = 1024 * (g + 1);  // Different sizes for each group
            entry.modifiedDate = 1640000000 + (g * 1000) + f;
            entry.hash = QStringLiteral("hash%1").arg(g);
            group.entries.append(entry);
        }

        result.append(group);
    }

    return result;
}

// ==== Tree Structure Tests ====

void TestDuplicateModel::testEmptyModel()
{
    QCOMPARE(model->rowCount(), 0);
    QCOMPARE(model->columnCount(), 5);

    QModelIndex invalid = model->index(0, 0);
    QVERIFY(!invalid.isValid());
}

void TestDuplicateModel::testSingleGroupWithFiles()
{
    auto data = createTestData(1, 3);
    model->setResults(data);

    QCOMPARE(model->rowCount(), 1);

    QModelIndex groupIndex = model->index(0, 0);
    QVERIFY(groupIndex.isValid());
    QCOMPARE(model->rowCount(groupIndex), 3);

    // Check children
    for (int i = 0; i < 3; ++i) {
        QModelIndex childIndex = model->index(i, 0, groupIndex);
        QVERIFY(childIndex.isValid());
        QCOMPARE(model->rowCount(childIndex), 0); // Children have no children
    }
}

void TestDuplicateModel::testMultipleGroups()
{
    auto data = createTestData(5, 2);
    model->setResults(data);

    QCOMPARE(model->rowCount(), 5);

    for (int g = 0; g < 5; ++g) {
        QModelIndex groupIndex = model->index(g, 0);
        QVERIFY(groupIndex.isValid());
        QCOMPARE(model->rowCount(groupIndex), 2);
    }
}

void TestDuplicateModel::testParentChildRelationships()
{
    auto data = createTestData(2, 3);
    model->setResults(data);

    QModelIndex group0 = model->index(0, 0);
    QModelIndex child0 = model->index(0, 0, group0);

    QVERIFY(group0.isValid());
    QVERIFY(child0.isValid());

    // Test parent() function
    QModelIndex parentOfChild = model->parent(child0);
    QCOMPARE(parentOfChild.row(), group0.row());
    QCOMPARE(parentOfChild.column(), group0.column());

    // Group has no parent
    QModelIndex parentOfGroup = model->parent(group0);
    QVERIFY(!parentOfGroup.isValid());
}

void TestDuplicateModel::testInvalidIndices()
{
    auto data = createTestData(2, 3);
    model->setResults(data);

    // Out of range indices
    QModelIndex invalid1 = model->index(-1, 0);
    QVERIFY(!invalid1.isValid());

    QModelIndex invalid2 = model->index(10, 0);
    QVERIFY(!invalid2.isValid());

    QModelIndex invalid3 = model->index(0, 10);
    QVERIFY(!invalid3.isValid());

    // Out of range child
    QModelIndex group = model->index(0, 0);
    QModelIndex invalid4 = model->index(10, 0, group);
    QVERIFY(!invalid4.isValid());
}

// ==== Selection Logic Tests ====

void TestDuplicateModel::testSelectAll()
{
    auto data = createTestData(2, 3);
    model->setResults(data);

    model->selectAll();

    QStringList selected = model->getSelectedFiles();
    QCOMPARE(selected.size(), 6); // 2 groups * 3 files = 6 files
}

void TestDuplicateModel::testSelectNone()
{
    auto data = createTestData(2, 3);
    model->setResults(data);

    model->selectAll();
    QCOMPARE(model->getSelectedFiles().size(), 6);

    model->selectNone();
    QCOMPARE(model->getSelectedFiles().size(), 0);
}

void TestDuplicateModel::testInvertSelection()
{
    auto data = createTestData(2, 3);
    model->setResults(data);

    // Initially nothing selected
    QCOMPARE(model->getSelectedFiles().size(), 0);

    // Invert should select all
    model->invertSelection();
    QCOMPARE(model->getSelectedFiles().size(), 6);

    // Invert again should select none
    model->invertSelection();
    QCOMPARE(model->getSelectedFiles().size(), 0);
}

void TestDuplicateModel::testIndividualCheckboxToggle()
{
    auto data = createTestData(1, 3);
    model->setResults(data);

    QModelIndex group = model->index(0, 0);
    QModelIndex file0 = model->index(0, 0, group);

    // Initially unchecked
    QVariant checkState = model->data(file0, Qt::CheckStateRole);
    QCOMPARE(checkState.toInt(), static_cast<int>(Qt::Unchecked));

    // Toggle to checked
    bool result = model->setData(file0, Qt::Checked, Qt::CheckStateRole);
    QVERIFY(result);

    checkState = model->data(file0, Qt::CheckStateRole);
    QCOMPARE(checkState.toInt(), static_cast<int>(Qt::Checked));

    // Should be in selected files
    QStringList selected = model->getSelectedFiles();
    QCOMPARE(selected.size(), 1);
}

void TestDuplicateModel::testGetSelectedFiles()
{
    auto data = createTestData(2, 3);
    model->setResults(data);

    // Select files in first group
    QModelIndex group0 = model->index(0, 0);
    for (int i = 0; i < 3; ++i) {
        QModelIndex fileIndex = model->index(i, 0, group0);
        model->setData(fileIndex, Qt::Checked, Qt::CheckStateRole);
    }

    QStringList selected = model->getSelectedFiles();
    QCOMPARE(selected.size(), 3);

    // Check paths are correct
    for (const QString &path : selected) {
        QVERIFY(path.contains(QLatin1String("/tmp/test/group0/")));
    }
}

void TestDuplicateModel::testSelectionPersistence()
{
    auto data = createTestData(1, 3);
    model->setResults(data);

    // Select first file
    QModelIndex group = model->index(0, 0);
    QModelIndex file0 = model->index(0, 0, group);
    model->setData(file0, Qt::Checked, Qt::CheckStateRole);

    QCOMPARE(model->getSelectedFiles().size(), 1);

    // Selection should persist when queried again
    QCOMPARE(model->getSelectedFiles().size(), 1);

    // Clear model should clear selection
    model->clear();
    QCOMPARE(model->getSelectedFiles().size(), 0);
}

// ==== Data Display Tests ====

void TestDuplicateModel::testColumnCount()
{
    QCOMPARE(model->columnCount(), 5);

    auto data = createTestData(2, 3);
    model->setResults(data);

    // Should still be 5 columns
    QCOMPARE(model->columnCount(), 5);

    QModelIndex group = model->index(0, 0);
    QCOMPARE(model->columnCount(group), 5);
}

void TestDuplicateModel::testHeaderData()
{
    QVariant header0 = model->headerData(0, Qt::Horizontal, Qt::DisplayRole);
    QCOMPARE(header0.toString(), QString());  // Checkbox column (empty string)

    QVariant header1 = model->headerData(1, Qt::Horizontal, Qt::DisplayRole);
    QVERIFY(header1.toString().contains(QLatin1String("Name")) || header1.toString().contains(QLatin1String("File")));

    QVariant header2 = model->headerData(2, Qt::Horizontal, Qt::DisplayRole);
    QVERIFY(header2.toString().contains(QLatin1String("Size")));

    QVariant header3 = model->headerData(3, Qt::Horizontal, Qt::DisplayRole);
    QVERIFY(header3.toString().contains(QLatin1String("Modified")) || header3.toString().contains(QLatin1String("Date")));

    QVariant header4 = model->headerData(4, Qt::Horizontal, Qt::DisplayRole);
    QVERIFY(header4.toString().contains(QLatin1String("Directory")) || header4.toString().contains(QLatin1String("Path")));
}

void TestDuplicateModel::testSizeFormatting()
{
    auto data = createTestData(1, 1);
    model->setResults(data);

    QModelIndex group = model->index(0, 0);
    QModelIndex file = model->index(0, 0, group);

    QVariant sizeData = model->data(model->index(0, 2, group), Qt::DisplayRole);
    QString sizeStr = sizeData.toString();

    // Should contain KB or MB
    QVERIFY(sizeStr.contains(QLatin1String("KB")) || sizeStr.contains(QLatin1String("MB")) || sizeStr.contains(QLatin1String("bytes")));
}

void TestDuplicateModel::testDateFormatting()
{
    auto data = createTestData(1, 1);
    model->setResults(data);

    QModelIndex group = model->index(0, 0);
    QModelIndex file = model->index(0, 0, group);

    QVariant dateData = model->data(model->index(0, 3, group), Qt::DisplayRole);
    QString dateStr = dateData.toString();

    // Should be a formatted date string (not just a number)
    QVERIFY(!dateStr.isEmpty());
    QVERIFY(dateStr.length() > 5);
}

void TestDuplicateModel::testDisplayRole()
{
    auto data = createTestData(1, 1);
    model->setResults(data);

    QModelIndex group = model->index(0, 0);
    QModelIndex file = model->index(0, 0, group);

    // Test file name (column 1)
    QVariant nameData = model->data(model->index(0, 1, group), Qt::DisplayRole);
    QVERIFY(nameData.toString().contains(QLatin1String("file0.txt")));

    // Test directory (column 4)
    QVariant dirData = model->data(model->index(0, 4, group), Qt::DisplayRole);
    QVERIFY(dirData.toString().contains(QLatin1String("/tmp/test/group0")));
}

void TestDuplicateModel::testTooltipRole()
{
    auto data = createTestData(1, 1);
    model->setResults(data);

    QModelIndex group = model->index(0, 0);
    QModelIndex file = model->index(0, 0, group);

    // Tooltip should show full path
    QVariant tooltip = model->data(model->index(0, 1, group), Qt::ToolTipRole);
    if (!tooltip.isNull()) {
        QVERIFY(tooltip.toString().contains(QLatin1String("/tmp/test/group0/file0.txt")));
    }
}

// ==== Model Operations Tests ====

void TestDuplicateModel::testSetResults()
{
    auto data = createTestData(3, 4);
    model->setResults(data);

    QCOMPARE(model->rowCount(), 3);

    for (int i = 0; i < 3; ++i) {
        QModelIndex group = model->index(i, 0);
        QCOMPARE(model->rowCount(group), 4);
    }

    // Set new results should replace old
    auto newData = createTestData(2, 2);
    model->setResults(newData);

    QCOMPARE(model->rowCount(), 2);
    QModelIndex group = model->index(0, 0);
    QCOMPARE(model->rowCount(group), 2);
}

void TestDuplicateModel::testClear()
{
    auto data = createTestData(5, 3);
    model->setResults(data);

    QCOMPARE(model->rowCount(), 5);

    model->clear();

    QCOMPARE(model->rowCount(), 0);
    QCOMPARE(model->getSelectedFiles().size(), 0);
}

void TestDuplicateModel::testRowCountTopLevel()
{
    QCOMPARE(model->rowCount(), 0);

    model->setResults(createTestData(0, 0));
    QCOMPARE(model->rowCount(), 0);

    model->setResults(createTestData(1, 0));
    QCOMPARE(model->rowCount(), 1);

    model->setResults(createTestData(10, 1));
    QCOMPARE(model->rowCount(), 10);
}

void TestDuplicateModel::testRowCountChildren()
{
    auto data = createTestData(3, 5);
    model->setResults(data);

    for (int i = 0; i < 3; ++i) {
        QModelIndex group = model->index(i, 0);
        QCOMPARE(model->rowCount(group), 5);
    }
}

void TestDuplicateModel::testDataChangeSignals()
{
    auto data = createTestData(1, 2);
    model->setResults(data);

    QSignalSpy spy(model, &QAbstractItemModel::dataChanged);

    QModelIndex group = model->index(0, 0);
    QModelIndex file0 = model->index(0, 0, group);

    model->setData(file0, Qt::Checked, Qt::CheckStateRole);

    // Should emit dataChanged signal
    QVERIFY(spy.count() > 0);
}

QTEST_MAIN(TestDuplicateModel)
#include "test_duplicatemodel.moc"
