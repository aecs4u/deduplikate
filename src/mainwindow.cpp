#include "mainwindow.h"
#include "duplicatefinder.h"
#include "duplicatemodel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QGroupBox>
#include <QFormLayout>
#include <QFileInfo>
#include <QUrl>
#include <QPushButton>
#include <KLocalizedString>
#include <KIO/DeleteJob>
#include <KIO/CopyJob>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_resultsModel(nullptr)
    , m_duplicateFinder(nullptr)
    , m_scanning(false)
    , m_currentTool(0)
{
    setupUi();
    setupMenuBar();

    m_duplicateFinder = new DuplicateFinder(this);

    connect(m_duplicateFinder, &DuplicateFinder::scanStarted,
            this, &MainWindow::onScanStarted);
    connect(m_duplicateFinder, &DuplicateFinder::scanProgress,
            this, &MainWindow::onScanProgress);
    connect(m_duplicateFinder, &DuplicateFinder::scanFinished,
            this, &MainWindow::onScanFinished);
    connect(m_duplicateFinder, &DuplicateFinder::resultsReady,
            this, &MainWindow::onResultsReady);

    setWindowTitle(i18n("Deduplikate - Duplicate File Finder"));
    resize(1200, 700);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUi()
{
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Main splitter (left | center | right)
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);

    createLeftPanel();
    createCenterPanel();
    createRightPanel();

    m_mainSplitter->addWidget(m_toolList);
    m_mainSplitter->addWidget(m_centerRightSplitter);
    m_mainSplitter->setStretchFactor(0, 1);
    m_mainSplitter->setStretchFactor(1, 5);

    mainLayout->addWidget(m_mainSplitter);

    createBottomPanel();
    mainLayout->addWidget(m_actionPanel);

    setCentralWidget(centralWidget);
}

void MainWindow::setupMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu(i18n("&File"));

    QAction *quitAction = fileMenu->addAction(i18n("&Quit"));
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QMainWindow::close);

    QMenu *helpMenu = menuBar()->addMenu(i18n("&Help"));

    QAction *aboutAction = helpMenu->addAction(i18n("&About"));
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, i18n("About Deduplikate"),
            i18n("Deduplikate 1.0.0\n\n"
                 "A KDE6 application for finding duplicate files\n"
                 "using the czkawka backend.\n\n"
                 "Built with Qt6 and KDE Frameworks 6"));
    });
}

void MainWindow::createLeftPanel()
{
    m_toolList = new QListWidget();
    m_toolList->setMaximumWidth(180);
    m_toolList->addItem(i18n("Duplicate Files"));
    m_toolList->addItem(i18n("Empty Folders"));
    m_toolList->addItem(i18n("Big Files"));
    m_toolList->addItem(i18n("Empty Files"));
    m_toolList->addItem(i18n("Temporary Files"));
    m_toolList->addItem(i18n("Similar Images"));
    m_toolList->addItem(i18n("Similar Videos"));
    m_toolList->addItem(i18n("Similar Music"));

    m_toolList->setCurrentRow(0);

    connect(m_toolList, &QListWidget::currentRowChanged,
            this, &MainWindow::onToolSelected);
}

void MainWindow::createCenterPanel()
{
    m_centerRightSplitter = new QSplitter(Qt::Horizontal);

    m_resultsView = new QTreeView();
    m_resultsModel = new DuplicateModel(this);
    m_resultsView->setModel(m_resultsModel);
    m_resultsView->setRootIsDecorated(true);
    m_resultsView->setAlternatingRowColors(true);
    m_resultsView->setSortingEnabled(true);

    m_centerRightSplitter->addWidget(m_resultsView);
}

void MainWindow::createRightPanel()
{
    m_settingsPanel = new QWidget();
    QVBoxLayout *settingsLayout = new QVBoxLayout(m_settingsPanel);

    QGroupBox *methodGroup = new QGroupBox(i18n("Detection Method"));
    QFormLayout *methodLayout = new QFormLayout(methodGroup);

    m_checkMethodCombo = new QComboBox();
    m_checkMethodCombo->addItem(i18n("Hash (Most Accurate)"), 0);
    m_checkMethodCombo->addItem(i18n("Name"), 1);
    m_checkMethodCombo->addItem(i18n("Size"), 2);
    m_checkMethodCombo->addItem(i18n("Size + Name"), 3);
    methodLayout->addRow(i18n("Method:"), m_checkMethodCombo);

    m_hashTypeCombo = new QComboBox();
    m_hashTypeCombo->addItem(i18n("Blake3 (Recommended)"), 0);
    m_hashTypeCombo->addItem(i18n("CRC32"), 1);
    m_hashTypeCombo->addItem(i18n("XXH3"), 2);
    methodLayout->addRow(i18n("Hash Type:"), m_hashTypeCombo);

    settingsLayout->addWidget(methodGroup);

    QGroupBox *optionsGroup = new QGroupBox(i18n("Options"));
    QVBoxLayout *optionsLayout = new QVBoxLayout(optionsGroup);

    m_recursiveCheck = new QCheckBox(i18n("Recursive search"));
    m_recursiveCheck->setChecked(true);
    optionsLayout->addWidget(m_recursiveCheck);

    m_ignoreHardLinksCheck = new QCheckBox(i18n("Ignore hard links"));
    m_ignoreHardLinksCheck->setChecked(true);
    optionsLayout->addWidget(m_ignoreHardLinksCheck);

    m_useCacheCheck = new QCheckBox(i18n("Use cache (faster)"));
    m_useCacheCheck->setChecked(true);
    optionsLayout->addWidget(m_useCacheCheck);

    QHBoxLayout *minSizeLayout = new QHBoxLayout();
    minSizeLayout->addWidget(new QLabel(i18n("Min size (KB):")));
    m_minSizeSpin = new QSpinBox();
    m_minSizeSpin->setRange(0, 1000000);
    m_minSizeSpin->setValue(1);
    minSizeLayout->addWidget(m_minSizeSpin);
    optionsLayout->addLayout(minSizeLayout);

    QHBoxLayout *maxSizeLayout = new QHBoxLayout();
    maxSizeLayout->addWidget(new QLabel(i18n("Max size (MB, 0=unlimited):")));
    m_maxSizeSpin = new QSpinBox();
    m_maxSizeSpin->setRange(0, 1000000);
    m_maxSizeSpin->setValue(0);
    maxSizeLayout->addWidget(m_maxSizeSpin);
    optionsLayout->addLayout(maxSizeLayout);

    settingsLayout->addWidget(optionsGroup);

    QGroupBox *pathsGroup = new QGroupBox(i18n("Directories"));
    QVBoxLayout *pathsLayout = new QVBoxLayout(pathsGroup);

    pathsLayout->addWidget(new QLabel(i18n("Include:")));
    m_includePathsList = new QListWidget();
    m_includePathsList->setMaximumHeight(100);
    pathsLayout->addWidget(m_includePathsList);

    QHBoxLayout *includeButtonsLayout = new QHBoxLayout();
    m_addIncludePathBtn = new QPushButton(i18n("Add"));
    m_removeIncludePathBtn = new QPushButton(i18n("Remove"));
    includeButtonsLayout->addWidget(m_addIncludePathBtn);
    includeButtonsLayout->addWidget(m_removeIncludePathBtn);
    pathsLayout->addLayout(includeButtonsLayout);

    pathsLayout->addWidget(new QLabel(i18n("Exclude:")));
    m_excludePathsList = new QListWidget();
    m_excludePathsList->setMaximumHeight(100);
    pathsLayout->addWidget(m_excludePathsList);

    QHBoxLayout *excludeButtonsLayout = new QHBoxLayout();
    m_addExcludePathBtn = new QPushButton(i18n("Add"));
    m_removeExcludePathBtn = new QPushButton(i18n("Remove"));
    excludeButtonsLayout->addWidget(m_addExcludePathBtn);
    excludeButtonsLayout->addWidget(m_removeExcludePathBtn);
    pathsLayout->addLayout(excludeButtonsLayout);

    settingsLayout->addWidget(pathsGroup);
    settingsLayout->addStretch();

    m_settingsPanel->setMaximumWidth(350);
    m_centerRightSplitter->addWidget(m_settingsPanel);

    connect(m_addIncludePathBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, i18n("Select Directory"));
        if (!dir.isEmpty()) {
            m_includePathsList->addItem(dir);
        }
    });

    connect(m_removeIncludePathBtn, &QPushButton::clicked, this, [this]() {
        qDeleteAll(m_includePathsList->selectedItems());
    });

    connect(m_addExcludePathBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, i18n("Select Directory"));
        if (!dir.isEmpty()) {
            m_excludePathsList->addItem(dir);
        }
    });

    connect(m_removeExcludePathBtn, &QPushButton::clicked, this, [this]() {
        qDeleteAll(m_excludePathsList->selectedItems());
    });
}

void MainWindow::createBottomPanel()
{
    m_actionPanel = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_actionPanel);

    QHBoxLayout *buttonsLayout = new QHBoxLayout();

    m_scanButton = new QPushButton(QIcon::fromTheme("system-search"), i18n("Scan"));
    m_stopButton = new QPushButton(QIcon::fromTheme("process-stop"), i18n("Stop"));
    m_stopButton->setEnabled(false);
    m_deleteButton = new QPushButton(QIcon::fromTheme("edit-delete"), i18n("Delete"));
    m_deleteButton->setEnabled(false);
    m_moveButton = new QPushButton(QIcon::fromTheme("folder-move"), i18n("Move"));
    m_moveButton->setEnabled(false);
    m_hardlinkButton = new QPushButton(QIcon::fromTheme("link"), i18n("Hardlink"));
    m_hardlinkButton->setEnabled(false);
    m_symlinkButton = new QPushButton(QIcon::fromTheme("emblem-symbolic-link"), i18n("Symlink"));
    m_symlinkButton->setEnabled(false);
    m_selectAllButton = new QPushButton(i18n("Select All"));
    m_selectNoneButton = new QPushButton(i18n("Select None"));
    m_invertSelectionButton = new QPushButton(i18n("Invert Selection"));

    buttonsLayout->addWidget(m_scanButton);
    buttonsLayout->addWidget(m_stopButton);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(m_selectAllButton);
    buttonsLayout->addWidget(m_selectNoneButton);
    buttonsLayout->addWidget(m_invertSelectionButton);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(m_deleteButton);
    buttonsLayout->addWidget(m_moveButton);
    buttonsLayout->addWidget(m_hardlinkButton);
    buttonsLayout->addWidget(m_symlinkButton);

    layout->addLayout(buttonsLayout);

    QHBoxLayout *statusLayout = new QHBoxLayout();
    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    m_statusLabel = new QLabel(i18n("Ready"));
    m_resultsLabel = new QLabel();

    statusLayout->addWidget(m_statusLabel);
    statusLayout->addWidget(m_progressBar);
    statusLayout->addStretch();
    statusLayout->addWidget(m_resultsLabel);

    layout->addLayout(statusLayout);

    m_actionPanel->setMaximumHeight(100);

    connect(m_scanButton, &QPushButton::clicked, this, &MainWindow::onScanClicked);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    connect(m_deleteButton, &QPushButton::clicked, this, &MainWindow::onDeleteClicked);
    connect(m_moveButton, &QPushButton::clicked, this, &MainWindow::onMoveClicked);
    connect(m_hardlinkButton, &QPushButton::clicked, this, &MainWindow::onHardlinkClicked);
    connect(m_symlinkButton, &QPushButton::clicked, this, &MainWindow::onSymlinkClicked);
    connect(m_selectAllButton, &QPushButton::clicked, this, &MainWindow::onSelectAllClicked);
    connect(m_selectNoneButton, &QPushButton::clicked, this, &MainWindow::onSelectNoneClicked);
    connect(m_invertSelectionButton, &QPushButton::clicked, this, &MainWindow::onInvertSelectionClicked);
}

void MainWindow::onToolSelected(int index)
{
    m_currentTool = index;
    m_statusLabel->setText(i18n("Tool changed to: %1", m_toolList->item(index)->text()));

    // For now, only duplicate files is implemented
    bool isDuplicateTool = (index == 0);
    m_settingsPanel->setEnabled(isDuplicateTool);
    m_scanButton->setEnabled(isDuplicateTool);
}

void MainWindow::onScanClicked()
{
    if (m_includePathsList->count() == 0) {
        QMessageBox::warning(this, i18n("No Directories"),
            i18n("Please add at least one directory to scan."));
        return;
    }

    DuplicateFinder::ScanParameters params;
    params.checkMethod = m_checkMethodCombo->currentData().toInt();
    params.hashType = m_hashTypeCombo->currentData().toInt();
    params.recursive = m_recursiveCheck->isChecked();
    params.ignoreHardLinks = m_ignoreHardLinksCheck->isChecked();
    params.useCache = m_useCacheCheck->isChecked();
    params.minSize = static_cast<quint64>(m_minSizeSpin->value()) * 1024;
    params.maxSize = m_maxSizeSpin->value() > 0
        ? static_cast<quint64>(m_maxSizeSpin->value()) * 1024 * 1024
        : 0;

    for (int i = 0; i < m_includePathsList->count(); ++i) {
        params.includePaths.append(m_includePathsList->item(i)->text());
    }

    for (int i = 0; i < m_excludePathsList->count(); ++i) {
        params.excludePaths.append(m_excludePathsList->item(i)->text());
    }

    m_resultsModel->clear();
    m_duplicateFinder->startScan(params);
}

void MainWindow::onStopClicked()
{
    m_duplicateFinder->stopScan();
}

void MainWindow::onDeleteClicked()
{
    QList<QString> selectedFiles = m_resultsModel->getSelectedFiles();

    if (selectedFiles.isEmpty()) {
        QMessageBox::information(this, i18n("No Selection"),
            i18n("Please select files to delete."));
        return;
    }

    // Calculate total size
    quint64 totalSize = 0;
    for (const QString &filePath : selectedFiles) {
        QFileInfo info(filePath);
        if (info.exists()) {
            totalSize += info.size();
        }
    }

    QString sizeStr;
    if (totalSize > 1024 * 1024 * 1024) {
        sizeStr = QString::number(totalSize / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
    } else if (totalSize > 1024 * 1024) {
        sizeStr = QString::number(totalSize / (1024.0 * 1024.0), 'f', 2) + " MB";
    } else if (totalSize > 1024) {
        sizeStr = QString::number(totalSize / 1024.0, 'f', 2) + " KB";
    } else {
        sizeStr = QString::number(totalSize) + " bytes";
    }

    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setWindowTitle(i18n("Confirm Deletion"));
    msgBox.setText(i18n("Delete %1 selected files (%2)?", selectedFiles.count(), sizeStr));
    msgBox.setInformativeText(i18n("This action cannot be undone if you choose permanent deletion."));

    QPushButton *trashButton = msgBox.addButton(i18n("Move to Trash"), QMessageBox::AcceptRole);
    QPushButton *deleteButton = msgBox.addButton(i18n("Delete Permanently"), QMessageBox::DestructiveRole);
    QPushButton *cancelButton = msgBox.addButton(QMessageBox::Cancel);

    msgBox.setDefaultButton(trashButton);
    msgBox.exec();

    if (msgBox.clickedButton() == cancelButton) {
        return;
    }

    bool moveToTrash = (msgBox.clickedButton() == trashButton);
    int successCount = 0;
    int failCount = 0;
    QStringList failedFiles;

    m_statusLabel->setText(i18n("Deleting files..."));
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, selectedFiles.count());

    for (int i = 0; i < selectedFiles.count(); ++i) {
        const QString &filePath = selectedFiles[i];
        m_progressBar->setValue(i);
        qApp->processEvents();

        QFileInfo info(filePath);
        if (!info.exists()) {
            continue;
        }

        bool success = false;
        if (moveToTrash) {
            // Use KIO to move to trash
            QUrl fileUrl = QUrl::fromLocalFile(filePath);
            KIO::DeleteJob *job = KIO::trash(fileUrl, KIO::HideProgressInfo);
            job->exec();
            success = (job->error() == 0);
        } else {
            // Permanent deletion
            QFile file(filePath);
            success = file.remove();
        }

        if (success) {
            successCount++;
        } else {
            failCount++;
            failedFiles.append(filePath);
        }
    }

    m_progressBar->setVisible(false);

    // Refresh results after deletion
    if (successCount > 0) {
        // Trigger a new scan with same parameters, or just remove from model
        // For now, just clear the results and ask user to rescan
        m_statusLabel->setText(i18n("Deleted %1 files, %2 failed", successCount, failCount));

        if (failCount > 0) {
            QString message = i18n("Failed to delete %1 files:\n", failCount);
            for (int i = 0; i < qMin(5, failedFiles.count()); ++i) {
                message += failedFiles[i] + "\n";
            }
            if (failedFiles.count() > 5) {
                message += i18n("... and %1 more", failedFiles.count() - 5);
            }
            QMessageBox::warning(this, i18n("Deletion Errors"), message);
        } else {
            QMessageBox::information(this, i18n("Success"),
                i18n("Successfully deleted %1 files.", successCount));
        }

        // Clear results and suggest rescan
        m_resultsModel->clear();
        m_resultsLabel->clear();
        m_deleteButton->setEnabled(false);
        m_moveButton->setEnabled(false);
        m_hardlinkButton->setEnabled(false);
        m_symlinkButton->setEnabled(false);
    }
}

void MainWindow::onMoveClicked()
{
    QList<QString> selectedFiles = m_resultsModel->getSelectedFiles();

    if (selectedFiles.isEmpty()) {
        QMessageBox::information(this, i18n("No Selection"),
            i18n("Please select files to move."));
        return;
    }

    QString targetDir = QFileDialog::getExistingDirectory(this,
        i18n("Select Destination Directory"),
        QString(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (targetDir.isEmpty()) {
        return;
    }

    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setWindowTitle(i18n("Confirm Move"));
    msgBox.setText(i18n("Move %1 selected files to %2?", selectedFiles.count(), targetDir));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Yes);

    if (msgBox.exec() != QMessageBox::Yes) {
        return;
    }

    int successCount = 0;
    int failCount = 0;
    QStringList failedFiles;

    m_statusLabel->setText(i18n("Moving files..."));
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, selectedFiles.count());

    for (int i = 0; i < selectedFiles.count(); ++i) {
        const QString &filePath = selectedFiles[i];
        m_progressBar->setValue(i);
        qApp->processEvents();

        QFileInfo info(filePath);
        if (!info.exists()) {
            continue;
        }

        QUrl sourceUrl = QUrl::fromLocalFile(filePath);
        QUrl destUrl = QUrl::fromLocalFile(targetDir + "/" + info.fileName());

        KIO::CopyJob *job = KIO::move(sourceUrl, destUrl, KIO::HideProgressInfo);
        job->exec();

        if (job->error() == 0) {
            successCount++;
        } else {
            failCount++;
            failedFiles.append(filePath);
        }
    }

    m_progressBar->setVisible(false);
    m_statusLabel->setText(i18n("Moved %1 files, %2 failed", successCount, failCount));

    if (failCount > 0) {
        QString message = i18n("Failed to move %1 files:\n", failCount);
        for (int i = 0; i < qMin(5, failedFiles.count()); ++i) {
            message += failedFiles[i] + "\n";
        }
        if (failedFiles.count() > 5) {
            message += i18n("... and %1 more", failedFiles.count() - 5);
        }
        QMessageBox::warning(this, i18n("Move Errors"), message);
    } else {
        QMessageBox::information(this, i18n("Success"),
            i18n("Successfully moved %1 files.", successCount));
    }

    if (successCount > 0) {
        m_resultsModel->clear();
        m_resultsLabel->clear();
        m_deleteButton->setEnabled(false);
        m_moveButton->setEnabled(false);
        m_hardlinkButton->setEnabled(false);
        m_symlinkButton->setEnabled(false);
    }
}

void MainWindow::onHardlinkClicked()
{
    QList<QString> selectedFiles = m_resultsModel->getSelectedFiles();

    if (selectedFiles.isEmpty()) {
        QMessageBox::information(this, i18n("No Selection"),
            i18n("Please select files to hardlink."));
        return;
    }

    // For hardlinking, we need to keep one original and link the others to it
    // Group by duplicate groups and keep the first file as original
    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setWindowTitle(i18n("Hardlink Duplicates"));
    msgBox.setText(i18n("Replace %1 selected duplicates with hardlinks to save space?",
                        selectedFiles.count()));
    msgBox.setInformativeText(i18n("This will replace duplicate files with hardlinks to the same inode.\n"
                                   "The first file in each group will be kept as the original."));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Yes);

    if (msgBox.exec() != QMessageBox::Yes) {
        return;
    }

    int successCount = 0;
    int failCount = 0;
    QStringList failedFiles;

    m_statusLabel->setText(i18n("Creating hardlinks..."));
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, selectedFiles.count());

    // Get all groups and process each group separately
    auto results = m_duplicateFinder->getResults();
    for (const auto &group : results) {
        if (group.entries.isEmpty()) continue;

        // Find the first file in this group that's not selected, or use first selected
        QString originalFile;
        QList<QString> filesToLink;

        for (const auto &entry : group.entries) {
            if (selectedFiles.contains(entry.path)) {
                filesToLink.append(entry.path);
            } else if (originalFile.isEmpty()) {
                originalFile = entry.path;
            }
        }

        // If all files in group are selected, use the first one as original
        if (originalFile.isEmpty() && !filesToLink.isEmpty()) {
            originalFile = filesToLink.takeFirst();
        }

        if (originalFile.isEmpty() || filesToLink.isEmpty()) {
            continue;
        }

        // Create hardlinks
        for (const QString &filePath : filesToLink) {
            m_progressBar->setValue(successCount + failCount);
            qApp->processEvents();

            QFileInfo info(filePath);
            if (!info.exists()) {
                continue;
            }

            // Remove the file first, then create hardlink
            if (QFile::remove(filePath)) {
                if (QFile::link(originalFile, filePath)) {
                    successCount++;
                } else {
                    failCount++;
                    failedFiles.append(filePath + " (link failed)");
                }
            } else {
                failCount++;
                failedFiles.append(filePath + " (remove failed)");
            }
        }
    }

    m_progressBar->setVisible(false);
    m_statusLabel->setText(i18n("Created %1 hardlinks, %2 failed", successCount, failCount));

    if (failCount > 0) {
        QString message = i18n("Failed to create hardlinks for %1 files:\n", failCount);
        for (int i = 0; i < qMin(5, failedFiles.count()); ++i) {
            message += failedFiles[i] + "\n";
        }
        if (failedFiles.count() > 5) {
            message += i18n("... and %1 more", failedFiles.count() - 5);
        }
        QMessageBox::warning(this, i18n("Hardlink Errors"), message);
    } else {
        QMessageBox::information(this, i18n("Success"),
            i18n("Successfully created %1 hardlinks.", successCount));
    }

    if (successCount > 0) {
        m_resultsModel->clear();
        m_resultsLabel->clear();
        m_deleteButton->setEnabled(false);
        m_moveButton->setEnabled(false);
        m_hardlinkButton->setEnabled(false);
        m_symlinkButton->setEnabled(false);
    }
}

void MainWindow::onSymlinkClicked()
{
    QList<QString> selectedFiles = m_resultsModel->getSelectedFiles();

    if (selectedFiles.isEmpty()) {
        QMessageBox::information(this, i18n("No Selection"),
            i18n("Please select files to symlink."));
        return;
    }

    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setWindowTitle(i18n("Symlink Duplicates"));
    msgBox.setText(i18n("Replace %1 selected duplicates with symbolic links?",
                        selectedFiles.count()));
    msgBox.setInformativeText(i18n("This will replace duplicate files with symbolic links.\n"
                                   "The first file in each group will be kept as the target."));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Yes);

    if (msgBox.exec() != QMessageBox::Yes) {
        return;
    }

    int successCount = 0;
    int failCount = 0;
    QStringList failedFiles;

    m_statusLabel->setText(i18n("Creating symbolic links..."));
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, selectedFiles.count());

    // Get all groups and process each group separately
    auto results = m_duplicateFinder->getResults();
    for (const auto &group : results) {
        if (group.entries.isEmpty()) continue;

        QString originalFile;
        QList<QString> filesToLink;

        for (const auto &entry : group.entries) {
            if (selectedFiles.contains(entry.path)) {
                filesToLink.append(entry.path);
            } else if (originalFile.isEmpty()) {
                originalFile = entry.path;
            }
        }

        if (originalFile.isEmpty() && !filesToLink.isEmpty()) {
            originalFile = filesToLink.takeFirst();
        }

        if (originalFile.isEmpty() || filesToLink.isEmpty()) {
            continue;
        }

        for (const QString &filePath : filesToLink) {
            m_progressBar->setValue(successCount + failCount);
            qApp->processEvents();

            QFileInfo info(filePath);
            if (!info.exists()) {
                continue;
            }

            if (QFile::remove(filePath)) {
                if (QFile::link(originalFile, filePath)) {
                    successCount++;
                } else {
                    failCount++;
                    failedFiles.append(filePath + " (symlink failed)");
                }
            } else {
                failCount++;
                failedFiles.append(filePath + " (remove failed)");
            }
        }
    }

    m_progressBar->setVisible(false);
    m_statusLabel->setText(i18n("Created %1 symbolic links, %2 failed", successCount, failCount));

    if (failCount > 0) {
        QString message = i18n("Failed to create symbolic links for %1 files:\n", failCount);
        for (int i = 0; i < qMin(5, failedFiles.count()); ++i) {
            message += failedFiles[i] + "\n";
        }
        if (failedFiles.count() > 5) {
            message += i18n("... and %1 more", failedFiles.count() - 5);
        }
        QMessageBox::warning(this, i18n("Symlink Errors"), message);
    } else {
        QMessageBox::information(this, i18n("Success"),
            i18n("Successfully created %1 symbolic links.", successCount));
    }

    if (successCount > 0) {
        m_resultsModel->clear();
        m_resultsLabel->clear();
        m_deleteButton->setEnabled(false);
        m_moveButton->setEnabled(false);
        m_hardlinkButton->setEnabled(false);
        m_symlinkButton->setEnabled(false);
    }
}

void MainWindow::onSelectAllClicked()
{
    m_resultsModel->selectAll();
}

void MainWindow::onSelectNoneClicked()
{
    m_resultsModel->selectNone();
}

void MainWindow::onInvertSelectionClicked()
{
    m_resultsModel->invertSelection();
}

void MainWindow::onScanStarted()
{
    updateUiState(true);
    m_statusLabel->setText(i18n("Scanning..."));
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, 0); // Indeterminate
}

void MainWindow::onScanProgress(int current, int total)
{
    if (total > 0) {
        m_progressBar->setRange(0, total);
        m_progressBar->setValue(current);
    }
}

void MainWindow::onScanFinished(bool success)
{
    updateUiState(false);
    m_progressBar->setVisible(false);

    if (success) {
        m_statusLabel->setText(i18n("Scan completed"));
    } else {
        m_statusLabel->setText(i18n("Scan stopped or failed"));
    }
}

void MainWindow::onResultsReady(int groupCount, quint64 wastedSpace)
{
    // Populate the model with results
    m_resultsModel->setResults(m_duplicateFinder->getResults());

    QString wastedSpaceStr;
    if (wastedSpace > 1024 * 1024 * 1024) {
        wastedSpaceStr = QString::number(wastedSpace / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
    } else if (wastedSpace > 1024 * 1024) {
        wastedSpaceStr = QString::number(wastedSpace / (1024.0 * 1024.0), 'f', 2) + " MB";
    } else if (wastedSpace > 1024) {
        wastedSpaceStr = QString::number(wastedSpace / 1024.0, 'f', 2) + " KB";
    } else {
        wastedSpaceStr = QString::number(wastedSpace) + " bytes";
    }

    m_resultsLabel->setText(i18n("Found %1 duplicate groups, wasted space: %2",
                                  groupCount, wastedSpaceStr));

    bool hasResults = (groupCount > 0);
    m_deleteButton->setEnabled(hasResults);
    m_moveButton->setEnabled(hasResults);
    m_hardlinkButton->setEnabled(hasResults);
    m_symlinkButton->setEnabled(hasResults);

    // Expand all groups to show files
    m_resultsView->expandAll();
}

void MainWindow::updateUiState(bool scanning)
{
    m_scanning = scanning;
    m_scanButton->setEnabled(!scanning);
    m_stopButton->setEnabled(scanning);
    m_settingsPanel->setEnabled(!scanning);
    m_toolList->setEnabled(!scanning);
}
