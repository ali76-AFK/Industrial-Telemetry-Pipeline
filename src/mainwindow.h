#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <vector>

#include "config.h"
#include "db.h"
#include "model.h"

class QLineEdit;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QTableWidget;
class QWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectClicked();
    void onResetAndIngestClicked();
    void onIngestOnlyClicked();
    void onRefreshClicked();
    void onDisconnectClicked();
    void onBrowseCsvClicked();
    void onBrowseRejectLogClicked();

private:
    void buildUi();
    void appendLog(const QString &message);
    void setStatusMessage(const QString &message);
    void updateActionStates();
    bool ensureConnected();
    void disconnectDatabaseIfNeeded();
    void refreshTable();
    void populateTable(const std::vector<DatabaseRow> &rows);
    bool validateIngestInputs(QString &errorMessage) const;
    QString currentCsvPath() const;
    QString currentRejectLogPath() const;
    AppConfig readConfigFromUi() const;
    void setUiFromConfig(const AppConfig &config);

private:
    QWidget *centralWidget_ = nullptr;
    QLineEdit *dsnEdit_ = nullptr;
    QLineEdit *usernameEdit_ = nullptr;
    QLineEdit *passwordEdit_ = nullptr;
    QLineEdit *csvPathEdit_ = nullptr;
    QLineEdit *rejectLogEdit_ = nullptr;

    QPushButton *connectButton_ = nullptr;
    QPushButton *disconnectButton_ = nullptr;
    QPushButton *refreshButton_ = nullptr;
    QPushButton *resetIngestButton_ = nullptr;
    QPushButton *ingestOnlyButton_ = nullptr;
    QPushButton *browseCsvButton_ = nullptr;
    QPushButton *browseRejectLogButton_ = nullptr;

    QLabel *statusLabel_ = nullptr;
    QTableWidget *table_ = nullptr;
    QPlainTextEdit *logBox_ = nullptr;

    OdbcContext db_{};
    bool connected_ = false;
};

#endif // MAINWINDOW_H
