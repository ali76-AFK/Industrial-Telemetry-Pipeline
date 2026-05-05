#include "mainwindow.h"

#include <QAbstractItemView>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

#include <exception>
#include <vector>

#include "ingestion.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    buildUi();

    AppConfig config;
    setUiFromConfig(config);

    appendLog("Application started.");
    updateActionStates();
}

MainWindow::~MainWindow()
{
    disconnectDatabaseIfNeeded();
}

void MainWindow::buildUi()
{
    centralWidget_ = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(centralWidget_);
    auto *formLayout = new QFormLayout();

    dsnEdit_ = new QLineEdit(this);
    usernameEdit_ = new QLineEdit(this);
    passwordEdit_ = new QLineEdit(this);
    csvPathEdit_ = new QLineEdit(this);
    rejectLogEdit_ = new QLineEdit(this);

    passwordEdit_->setEchoMode(QLineEdit::Password);

    auto *csvRowWidget = new QWidget(this);
    auto *csvRowLayout = new QHBoxLayout(csvRowWidget);
    csvRowLayout->setContentsMargins(0, 0, 0, 0); // Important for form alignment
    browseCsvButton_ = new QPushButton("Browse...", csvRowWidget);
    csvRowLayout->addWidget(csvPathEdit_);
    csvRowLayout->addWidget(browseCsvButton_);

    auto *rejectRowWidget = new QWidget(this);
    auto *rejectRowLayout = new QHBoxLayout(rejectRowWidget);
    rejectRowLayout->setContentsMargins(0, 0, 0, 0);
    browseRejectLogButton_ = new QPushButton("Browse...", rejectRowWidget);
    rejectRowLayout->addWidget(rejectLogEdit_);
    rejectRowLayout->addWidget(browseRejectLogButton_);

    formLayout->addRow("DSN:", dsnEdit_);
    formLayout->addRow("Username:", usernameEdit_);
    formLayout->addRow("Password:", passwordEdit_);
    formLayout->addRow("CSV path:", csvRowWidget);
    formLayout->addRow("Reject log:", rejectRowWidget);

    auto *buttonLayout = new QHBoxLayout();
    connectButton_ = new QPushButton("Connect", this);
    disconnectButton_ = new QPushButton("Disconnect", this);
    refreshButton_ = new QPushButton("Refresh", this);
    resetIngestButton_ = new QPushButton("Reset + Ingest", this);
    ingestOnlyButton_ = new QPushButton("Ingest Only", this);

    buttonLayout->addWidget(connectButton_);
    buttonLayout->addWidget(disconnectButton_);
    buttonLayout->addWidget(refreshButton_);
    buttonLayout->addWidget(resetIngestButton_);
    buttonLayout->addWidget(ingestOnlyButton_);
    buttonLayout->addStretch();

    statusLabel_ = new QLabel("Ready.", this);

    table_ = new QTableWidget(this);
    table_->setColumnCount(7);
    table_->setHorizontalHeaderLabels({
        "sample_id",
        "device_id",
        "metric_name",
        "metric_value",
        "event_time_utc",
        "ingest_time_utc",
        "source_type"
    });
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setSortingEnabled(false);
    table_->horizontalHeader()->setStretchLastSection(false);
    table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);

    logBox_ = new QPlainTextEdit(this);
    logBox_->setReadOnly(true);
    logBox_->setPlaceholderText("Operational log...");

    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(statusLabel_);
    mainLayout->addWidget(table_, 3);
    mainLayout->addWidget(logBox_, 1);

    setCentralWidget(centralWidget_);
    resize(1200, 700);
    setWindowTitle("Industrial Telemetry Pipeline");

    connect(connectButton_, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(disconnectButton_, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(refreshButton_, &QPushButton::clicked, this, &MainWindow::onRefreshClicked);
    connect(resetIngestButton_, &QPushButton::clicked, this, &MainWindow::onResetAndIngestClicked);
    connect(ingestOnlyButton_, &QPushButton::clicked, this, &MainWindow::onIngestOnlyClicked);
    connect(browseCsvButton_, &QPushButton::clicked, this, &MainWindow::onBrowseCsvClicked);
    connect(browseRejectLogButton_, &QPushButton::clicked, this, &MainWindow::onBrowseRejectLogClicked);
}

AppConfig MainWindow::readConfigFromUi() const
{
    AppConfig config;
    config.dsn = dsnEdit_->text().trimmed().toStdString();
    config.username = usernameEdit_->text().trimmed().toStdString();
    config.password = passwordEdit_->text().toStdString();
    config.default_csv_path = csvPathEdit_->text().trimmed().toStdString();
    config.reject_log_path = rejectLogEdit_->text().trimmed().toStdString();
    return config;
}

void MainWindow::setUiFromConfig(const AppConfig &config)
{
    dsnEdit_->setText(QString::fromStdString(config.dsn));
    usernameEdit_->setText(QString::fromStdString(config.username));
    passwordEdit_->setText(QString::fromStdString(config.password));
    csvPathEdit_->setText(QString::fromStdString(config.default_csv_path));
    rejectLogEdit_->setText(QString::fromStdString(config.reject_log_path));
}

void MainWindow::setStatusMessage(const QString &message)
{
    statusLabel_->setText(message);
}

void MainWindow::appendLog(const QString &message)
{
    logBox_->appendPlainText(message);
}

void MainWindow::updateActionStates()
{
    connectButton_->setEnabled(!connected_);
    disconnectButton_->setEnabled(connected_);
    refreshButton_->setEnabled(connected_);
    resetIngestButton_->setEnabled(connected_);
    ingestOnlyButton_->setEnabled(connected_);
}

QString MainWindow::currentCsvPath() const
{
    return csvPathEdit_->text().trimmed();
}

QString MainWindow::currentRejectLogPath() const
{
    return rejectLogEdit_->text().trimmed();
}

bool MainWindow::validateIngestInputs(QString &errorMessage) const
{
    const QString csvPath = currentCsvPath();
    const QString rejectLogPath = currentRejectLogPath();

    if (csvPath.isEmpty()) {
        errorMessage = "CSV path is required.";
        return false;
    }

    QFileInfo csvInfo(csvPath);
    if (!csvInfo.exists() || !csvInfo.isFile()) {
        errorMessage = "CSV file not found: " + csvPath;
        return false;
    }

    if (!csvInfo.isReadable()) {
        errorMessage = "CSV file is not readable: " + csvPath;
        return false;
    }

    if (rejectLogPath.isEmpty()) {
        errorMessage = "Reject log path is required.";
        return false;
    }

    QFileInfo rejectInfo(rejectLogPath);
    QDir rejectDir = rejectInfo.dir();
    if (!rejectDir.exists()) {
        errorMessage = "Reject log directory does not exist: " + rejectDir.absolutePath();
        return false;
    }

    return true;
}

bool MainWindow::ensureConnected()
{
    if (connected_) {
        return true;
    }

    try {
        AppConfig config = readConfigFromUi();
        connect_database(db_, config);
        connected_ = true;
        setStatusMessage("Connected.");
        appendLog("Connected to database: " + QString::fromStdString(config.dsn));
        updateActionStates();
        refreshTable();
        return true;
    } catch (const std::exception &ex) {
        QMessageBox::critical(
            this,
            "Connection Error",
            QString("Failed to connect to the database:\n%1").arg(ex.what())
        );
        setStatusMessage("Connection failed.");
        appendLog("Connection failed: " + QString::fromUtf8(ex.what()));
        updateActionStates();
        return false;
    } catch (...) {
        QMessageBox::critical(
            this,
            "Connection Error",
            "Failed to connect to the database. Check DSN, username, password, and ODBC setup."
        );
        setStatusMessage("Connection failed.");
        appendLog("Connection failed with unknown error.");
        updateActionStates();
        return false;
    }
}

void MainWindow::disconnectDatabaseIfNeeded()
{
    if (connected_) {
        cleanup(db_);
        connected_ = false;
        updateActionStates();
    }
}

void MainWindow::refreshTable()
{
    const std::vector<DatabaseRow> rows = fetch_inserted_rows(db_);
    populateTable(rows);
    appendLog(QString("Loaded %1 row(s) into table.").arg(rows.size()));
}

void MainWindow::populateTable(const std::vector<DatabaseRow> &rows)
{
    table_->setSortingEnabled(false);
    table_->setRowCount(0);
    table_->setRowCount(static_cast<int>(rows.size()));

    for (int row = 0; row < static_cast<int>(rows.size()); ++row) {
        const auto &r = rows[row];

        auto *idItem = new QTableWidgetItem();
        idItem->setData(Qt::DisplayRole, r.sample_id);
        table_->setItem(row, 0, idItem);

        table_->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(r.device_id)));
        table_->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(r.metric_name)));

        auto *valueItem = new QTableWidgetItem();
        valueItem->setData(Qt::DisplayRole, r.metric_value);
        table_->setItem(row, 3, valueItem);

        table_->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(r.event_time_utc)));
        table_->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(r.ingest_time_utc)));
        table_->setItem(row, 6, new QTableWidgetItem(QString::fromStdString(r.source_type)));
    }

    table_->resizeRowsToContents();
    table_->resizeColumnsToContents();
    table_->setSortingEnabled(true);
}

void MainWindow::onConnectClicked()
{
    if (ensureConnected()) {
        appendLog("Refresh after connect complete.");
    }
}

void MainWindow::onDisconnectClicked()
{
    if (!connected_) {
        return;
    }

    cleanup(db_);
    connected_ = false;

    table_->setSortingEnabled(false);
    table_->clearContents();
    table_->setRowCount(0);
    table_->setSortingEnabled(true);

    setStatusMessage("Disconnected.");
    appendLog("Disconnected from database.");
    updateActionStates();
}

void MainWindow::onRefreshClicked()
{
    if (!ensureConnected()) {
        return;
    }

    try {
        refreshTable();
        appendLog("Refresh complete.");
    } catch (const std::exception &ex) {
        QMessageBox::critical(
            this,
            "Refresh Error",
            QString("Failed to load rows:\n%1").arg(ex.what())
        );
        appendLog("Refresh failed: " + QString::fromUtf8(ex.what()));
    } catch (...) {
        QMessageBox::critical(this, "Refresh Error", "Failed to load rows.");
        appendLog("Refresh failed with unknown error.");
    }
}

void MainWindow::onResetAndIngestClicked()
{
    if (!ensureConnected()) {
        return;
    }

    QString errorMessage;
    if (!validateIngestInputs(errorMessage)) {
        QMessageBox::warning(this, "Validation Error", errorMessage);
        appendLog("Validation error: " + errorMessage);
        return;
    }

    try {
        AppConfig config = readConfigFromUi();
        appendLog("Starting reset + ingest...");
        appendLog("CSV path: " + currentCsvPath());
        appendLog("Reject log: " + currentRejectLogPath());

        reset_test_table(db_);
        appendLog("Reset dbo.telemetry_samples and reseeded identity.");

        prepare_insert_statement(db_);
        IngestionSummary summary = ingest_csv(db_, config.default_csv_path, config.reject_log_path);

        refreshTable();

        const QString msg = QString("Reset + ingest complete. Inserted: %1, Skipped: %2")
                                .arg(summary.inserted)
                                .arg(summary.skipped);
        setStatusMessage(msg);
        appendLog(msg);
    } catch (const std::exception &ex) {
        QMessageBox::critical(
            this,
            "Ingestion Error",
            QString("Reset + ingest failed:\n%1").arg(ex.what())
        );
        setStatusMessage("Reset + ingest failed.");
        appendLog("Reset + ingest failed: " + QString::fromUtf8(ex.what()));
    } catch (...) {
        QMessageBox::critical(
            this,
            "Ingestion Error",
            "Reset + ingest failed. Check file paths, DB state, and CSV contents."
        );
        setStatusMessage("Reset + ingest failed.");
        appendLog("Reset + ingest failed with unknown error.");
    }
}

void MainWindow::onIngestOnlyClicked()
{
    if (!ensureConnected()) {
        return;
    }

    QString errorMessage;
    if (!validateIngestInputs(errorMessage)) {
        QMessageBox::warning(this, "Validation Error", errorMessage);
        appendLog("Validation error: " + errorMessage);
        return;
    }

    try {
        AppConfig config = readConfigFromUi();
        appendLog("Starting ingest...");
        appendLog("CSV path: " + currentCsvPath());
        appendLog("Reject log: " + currentRejectLogPath());

        prepare_insert_statement(db_);
        IngestionSummary summary = ingest_csv(db_, config.default_csv_path, config.reject_log_path);

        refreshTable();

        const QString msg = QString("Ingest complete. Inserted: %1, Skipped: %2")
                                .arg(summary.inserted)
                                .arg(summary.skipped);
        setStatusMessage(msg);
        appendLog(msg);
    } catch (const std::exception &ex) {
        QMessageBox::critical(
            this,
            "Ingestion Error",
            QString("Ingest failed:\n%1").arg(ex.what())
        );
        setStatusMessage("Ingest failed.");
        appendLog("Ingest failed: " + QString::fromUtf8(ex.what()));
    } catch (...) {
        QMessageBox::critical(
            this,
            "Ingestion Error",
            "Ingest failed. Check file paths, DB state, and CSV contents."
        );
        setStatusMessage("Ingest failed.");
        appendLog("Ingest failed with unknown error.");
    }
}

void MainWindow::onBrowseCsvClicked()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        "Select CSV file",
        currentCsvPath(),
        "CSV Files (*.csv);;All Files (*)"
    );

    if (!path.isEmpty()) {
        csvPathEdit_->setText(path);
        appendLog("CSV path selected: " + path);
    }
}

void MainWindow::onBrowseRejectLogClicked()
{
    const QString path = QFileDialog::getSaveFileName(
        this,
        "Select reject log file",
        currentRejectLogPath(),
        "Log Files (*.log);;All Files (*)"
    );

    if (!path.isEmpty()) {
        rejectLogEdit_->setText(path);
        appendLog("Reject log selected: " + path);
    }
}
