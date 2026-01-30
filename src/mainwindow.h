#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QListWidget>
#include <QTreeView>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QProgressBar>
#include <QLabel>
#include <QGroupBox>

class DuplicateFinder;
class DuplicateModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private Q_SLOTS:
    void onToolSelected(int index);
    void onScanClicked();
    void onStopClicked();
    void onDeleteClicked();
    void onMoveClicked();
    void onHardlinkClicked();
    void onSymlinkClicked();
    void onSelectAllClicked();
    void onSelectNoneClicked();
    void onInvertSelectionClicked();

    void onScanStarted();
    void onScanProgress(int current, int total);
    void onScanFinished(bool success);
    void onResultsReady(int groupCount, quint64 wastedSpace);

private:
    void setupUi();
    void setupMenuBar();
    void createLeftPanel();
    void createCenterPanel();
    void createRightPanel();
    void createBottomPanel();
    void updateUiState(bool scanning);

    // UI Components
    QSplitter *m_mainSplitter;
    QSplitter *m_centerRightSplitter;

    // Left panel - Tool selection
    QListWidget *m_toolList;

    // Center panel - Results
    QTreeView *m_resultsView;
    DuplicateModel *m_resultsModel;

    // Right panel - Settings
    QWidget *m_settingsPanel;
    QComboBox *m_checkMethodCombo;
    QComboBox *m_hashTypeCombo;
    QCheckBox *m_recursiveCheck;
    QCheckBox *m_ignoreHardLinksCheck;
    QCheckBox *m_useCacheCheck;
    QSpinBox *m_minSizeSpin;
    QSpinBox *m_maxSizeSpin;
    QListWidget *m_includePathsList;
    QListWidget *m_excludePathsList;
    QPushButton *m_addIncludePathBtn;
    QPushButton *m_addExcludePathBtn;
    QPushButton *m_removeIncludePathBtn;
    QPushButton *m_removeExcludePathBtn;

    // Bottom panel - Actions
    QWidget *m_actionPanel;
    QPushButton *m_scanButton;
    QPushButton *m_stopButton;
    QPushButton *m_deleteButton;
    QPushButton *m_moveButton;
    QPushButton *m_hardlinkButton;
    QPushButton *m_symlinkButton;
    QPushButton *m_selectAllButton;
    QPushButton *m_selectNoneButton;
    QPushButton *m_invertSelectionButton;
    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;
    QLabel *m_resultsLabel;

    // Business logic
    DuplicateFinder *m_duplicateFinder;

    // State
    bool m_scanning;
    int m_currentTool;
};

#endif // MAINWINDOW_H
