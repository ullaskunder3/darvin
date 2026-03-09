#include "mainwindow.h"

#include <QStandardPaths>
#include <QPlainTextEdit>
#include "terminalwidget.h"
#include <QVBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QDockWidget>
#include <QFileSystemModel>
#include <QTreeView>
#include <QDir>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QProcess>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    codeEditor = new QPlainTextEdit();

    QFont font = codeEditor->font();
    font.setFamily("Consolas");
    font.setPointSize(11);
    codeEditor->setFont(font);

    setCentralWidget(codeEditor);

    terminalDock = new QDockWidget("Output Console", this);
    terminalDock->setFeatures(QDockWidget::DockWidgetClosable);

    terminalWidget = new TerminalWidget(this);

    terminalDock->setWidget(terminalWidget);
    addDockWidget(Qt::BottomDockWidgetArea, terminalDock);

    terminalDock->hide();

    QMenu *fileMenu = menuBar()->addMenu("File");

    QAction *actionNew = fileMenu->addAction("New");

    QAction *actionOpen = fileMenu->addAction("Open");
    actionOpen->setShortcut(QKeySequence("F3"));

    QAction *actionSave = fileMenu->addAction("Save");
    actionSave->setShortcut(QKeySequence("F2"));

    fileMenu->addSeparator();

    QAction *actionExit = fileMenu->addAction("Exit");
    actionExit->setShortcut(QKeySequence("Alt+X"));

    connect(actionNew, &QAction::triggered, this, &MainWindow::newDocument);
    connect(actionOpen, &QAction::triggered, this, &MainWindow::openDocument);
    connect(actionSave, &QAction::triggered, this, &MainWindow::saveDocument);
    connect(actionExit, &QAction::triggered, this, &QWidget::close);

    QMenu *editMenu = menuBar()->addMenu("Edit");
    editMenu->addAction("Undo", codeEditor, &QPlainTextEdit::undo);
    editMenu->addAction("Redo", codeEditor, &QPlainTextEdit::redo);

    QMenu *compileMenu = menuBar()->addMenu("Compile");
    QAction *actionCompile = compileMenu->addAction("Compile");
    actionCompile->setShortcut(QKeySequence("Alt+F9"));
    connect(actionCompile, &QAction::triggered, this, &MainWindow::compileCode);

    QMenu *runMenu = menuBar()->addMenu("Run");
    QAction *actionRun = runMenu->addAction("Run Code");
    actionRun->setShortcut(QKeySequence("Ctrl+F9"));
    connect(actionRun, &QAction::triggered, this, &MainWindow::runCode);

    QAction *actionViewOutput = runMenu->addAction("View Output");
    actionViewOutput->setShortcut(QKeySequence("Alt+F5"));
    connect(actionViewOutput, &QAction::triggered, this, &MainWindow::viewOutput);

    QMenu *helpMenu = menuBar()->addMenu("Help");
    helpMenu->addAction("About Darvin");

    QDockWidget *fileDock = new QDockWidget("Project Explorer", this);
    fileDock->setFeatures(QDockWidget::NoDockWidgetFeatures);

    QString workspacePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/DarvinProjects";
    QDir dir(workspacePath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    fileModel = new QFileSystemModel(this);
    fileModel->setRootPath(workspacePath);

    fileTree = new QTreeView(fileDock);
    fileTree->setModel(fileModel);
    fileTree->setRootIndex(fileModel->index(workspacePath));
    fileTree->setHeaderHidden(true);

    connect(fileTree, &QTreeView::clicked, this, &MainWindow::openFileFromTree);

    for (int i = 1; i < fileModel->columnCount(); ++i) {
        fileTree->hideColumn(i);
    }

    fileDock->setWidget(fileTree);
    addDockWidget(Qt::LeftDockWidgetArea, fileDock);

    setWindowTitle("Darvin IDE - Untitled");
    resize(1000, 600);
}

void MainWindow::newDocument()
{
    currentFile.clear();
    codeEditor->setPlainText("#include <iostream>\n\nint main() {\n    std::cout << \"Hello, Darvin!\" << std::endl;\n    return 0;\n}\n");
    setWindowTitle("Darvin IDE - Untitled");
}

void MainWindow::openDocument()
{
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/DarvinProjects";
    QString fileName = QFileDialog::getOpenFileName(this, "Open File", defaultPath, "C++ Files (*.cpp *.h);;All Files (*.*)");

    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, "Error", "Cannot open file: " + file.errorString());
        return;
    }

    currentFile = fileName;
    QTextStream in(&file);
    codeEditor->setPlainText(in.readAll());
    file.close();

    setWindowTitle("Darvin IDE - " + QFileInfo(currentFile).fileName());
}

void MainWindow::saveDocument()
{
    QString fileName;
    if (currentFile.isEmpty()) {
        QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/DarvinProjects/main.cpp";
        fileName = QFileDialog::getSaveFileName(this, "Save File", defaultPath, "C++ Files (*.cpp *.h);;All Files (*.*)");
        if (fileName.isEmpty()) return;
        currentFile = fileName;
    } else {
        fileName = currentFile;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, "Error", "Cannot save file: " + file.errorString());
        return;
    }

    QTextStream out(&file);
    out << codeEditor->toPlainText();
    file.close();

    setWindowTitle("Darvin IDE - " + QFileInfo(currentFile).fileName());
}

void MainWindow::openFileFromTree(const QModelIndex &index)
{
    QString path = fileModel->filePath(index);
    QFileInfo fileInfo(path);

    if (fileInfo.isDir()) return;

    if (fileInfo.suffix().toLower() == "exe") {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::warning(this,
                                     "Unsupported File Format",
                                     "This file is a compiled executable (binary) and cannot be displayed correctly in the text editor.\n\nDo you want to open it anyway?",
                                     QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::No) {
            return;
        }
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, "Error", "Cannot open file: " + file.errorString());
        return;
    }

    currentFile = path;
    QTextStream in(&file);
    codeEditor->setPlainText(in.readAll());
    file.close();

    setWindowTitle("Darvin IDE - " + fileInfo.fileName());
}

bool MainWindow::compileCode()
{
    saveDocument();
    if (currentFile.isEmpty()) return false;

    terminalDock->show();
    terminalDock->setWindowTitle("Output Console - ⚙️ Compiling...");

    terminalWidget->clear();
    terminalWidget->appendOutput("Compiling " + QFileInfo(currentFile).fileName() + "...\n", "white");

    QFileInfo fileInfo(currentFile);
    QString exePath = fileInfo.absolutePath() + "/" + fileInfo.baseName() + ".exe";

    QProcess compiler;
    QStringList compileArgs;
    compileArgs << currentFile << "-o" << exePath;

    compiler.start("g++", compileArgs);
    compiler.waitForFinished();

    QString compileErrors = compiler.readAllStandardError();
    if (!compileErrors.isEmpty()) {
        terminalDock->setWindowTitle("Output Console - 🔴 ERRORS FOUND");
        terminalWidget->appendHtml("<span style='color:red;'><b>Compilation Failed:</b><br></span>" + compileErrors.toHtmlEscaped().replace("\n", "<br>"));
        return false;
    }

    terminalDock->setWindowTitle("Output Console - 🟢 SUCCESS");
    terminalWidget->appendHtml("<span style='color:green;'><b>Success: 0 Errors.</b></span><br>--------------------------------------------------<br>");
    return true;
}

void MainWindow::runCode()
{
    if (!compileCode()) return;

    QFileInfo fileInfo(currentFile);
    QString exePath = QDir::toNativeSeparators(fileInfo.absolutePath() + "/" + fileInfo.baseName() + ".exe");

    terminalDock->show();
    terminalDock->setWindowTitle("Output Console - ▶️ Running");

    terminalWidget->appendOutput(QString("Running: %1\n─────────────────────────────────\n").arg(exePath), "#888888");
    terminalWidget->startProcess(exePath, QStringList());
}

void MainWindow::viewOutput()
{
    terminalDock->show();
}
