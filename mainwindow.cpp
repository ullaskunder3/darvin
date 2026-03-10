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
#include <QProcessEnvironment>
#include <QFileInfo>
#include <QCoreApplication>

// ── Compiler auto-detection ─────────────────────────────────────────────────
// Returns the path to g++.exe to use for compilation.
// Priority:
//   1. compiler/bin/g++.exe  next to darvin.exe  (bundled — installed or portable)
//   2. C:/TDM-GCC-64/bin/g++.exe                 (developer's local machine)
//   3. plain "g++"                                (last resort — must be on PATH)
static QString findCompiler()
{
    // 1. Bundled compiler sitting next to darvin.exe
    QString appDir = QCoreApplication::applicationDirPath();
    QString bundled = appDir + "/compiler/bin/g++.exe";
    if (QFile::exists(bundled)) return bundled;

    // 2. Local TDM-GCC (developer machine)
    QString tdm = "C:/TDM-GCC-64/bin/g++.exe";
    if (QFile::exists(tdm)) return tdm;

    // 3. Fallback
    return "g++";
}

// Returns the root of whichever compiler we found (parent of bin/).
static QString compilerRoot()
{
    QString gpp = findCompiler();
    if (gpp == "g++") return QString();
    return QFileInfo(gpp).absolutePath() + "/..";   // bin/../  →  compiler root
}
// ───────────────────────────────────────────────────────────────────────────

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

    // ── File menu ──────────────────────────────────────────────────────────
    QMenu *fileMenu = menuBar()->addMenu("File");
    QAction *actionNew  = fileMenu->addAction("New");
    QAction *actionOpen = fileMenu->addAction("Open");
    actionOpen->setShortcut(QKeySequence("F3"));
    QAction *actionSave = fileMenu->addAction("Save");
    actionSave->setShortcut(QKeySequence("F2"));
    fileMenu->addSeparator();
    QAction *actionExit = fileMenu->addAction("Exit");
    actionExit->setShortcut(QKeySequence("Alt+X"));

    connect(actionNew,  &QAction::triggered, this, &MainWindow::newDocument);
    connect(actionOpen, &QAction::triggered, this, &MainWindow::openDocument);
    connect(actionSave, &QAction::triggered, this, &MainWindow::saveDocument);
    connect(actionExit, &QAction::triggered, this, &QWidget::close);

    // ── Edit menu ──────────────────────────────────────────────────────────
    QMenu *editMenu = menuBar()->addMenu("Edit");
    editMenu->addAction("Undo", codeEditor, &QPlainTextEdit::undo);
    editMenu->addAction("Redo", codeEditor, &QPlainTextEdit::redo);

    // ── Compile menu ───────────────────────────────────────────────────────
    QMenu *compileMenu = menuBar()->addMenu("Compile");
    QAction *actionDebug = compileMenu->addAction("Compile Debug");
    actionDebug->setShortcut(QKeySequence("Alt+F9"));
    connect(actionDebug, &QAction::triggered, this, [this]() { compileCode(false); });

    QAction *actionRelease = compileMenu->addAction("Build Release  (share with students)");
    actionRelease->setShortcut(QKeySequence("Shift+F9"));
    connect(actionRelease, &QAction::triggered, this, [this]() { compileCode(true); });

    // ── Run menu ───────────────────────────────────────────────────────────
    QMenu *runMenu = menuBar()->addMenu("Run");
    QAction *actionRun = runMenu->addAction("Run Code");
    actionRun->setShortcut(QKeySequence("Ctrl+F9"));
    connect(actionRun, &QAction::triggered, this, &MainWindow::runCode);

    QAction *actionViewOutput = runMenu->addAction("View Output");
    actionViewOutput->setShortcut(QKeySequence("Alt+F5"));
    connect(actionViewOutput, &QAction::triggered, this, &MainWindow::viewOutput);

    // ── Help menu ──────────────────────────────────────────────────────────
    menuBar()->addMenu("Help")->addAction("About Darvin");

    // ── Project Explorer ───────────────────────────────────────────────────
    QDockWidget *fileDock = new QDockWidget("Project Explorer", this);
    fileDock->setFeatures(QDockWidget::NoDockWidgetFeatures);

    QString workspacePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/DarvinProjects";
    QDir().mkpath(workspacePath);

    fileModel = new QFileSystemModel(this);
    fileModel->setRootPath(workspacePath);

    fileTree = new QTreeView(fileDock);
    fileTree->setModel(fileModel);
    fileTree->setRootIndex(fileModel->index(workspacePath));
    fileTree->setHeaderHidden(true);
    for (int i = 1; i < fileModel->columnCount(); ++i)
        fileTree->hideColumn(i);

    connect(fileTree, &QTreeView::clicked, this, &MainWindow::openFileFromTree);
    fileDock->setWidget(fileTree);
    addDockWidget(Qt::LeftDockWidgetArea, fileDock);

    setWindowTitle("Darvin IDE - Untitled");
    resize(1000, 600);
}

// ── Document actions ────────────────────────────────────────────────────────

void MainWindow::newDocument()
{
    currentFile.clear();
    codeEditor->setPlainText(
        "#include <iostream>\n\nint main() {\n"
        "    std::cout << \"Hello, Darvin!\" << std::endl;\n"
        "    return 0;\n}\n");
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
        auto reply = QMessageBox::warning(this,
            "Unsupported File Format",
            "This is a compiled executable. Open anyway?",
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No) return;
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

// ── Core compile logic ──────────────────────────────────────────────────────
bool MainWindow::compileCode(bool releaseMode)
{
    saveDocument();
    if (currentFile.isEmpty()) return false;

    terminalDock->show();
    terminalDock->setWindowTitle("Output Console - ⚙️ Compiling...");
    terminalWidget->stopProcess();
    terminalWidget->clear();

    QFileInfo fileInfo(currentFile);
    QString   code      = codeEditor->toPlainText();
    bool      usesSDL3  = code.contains("#include <SDL3");

    // ── Resolve compiler and SDL3 paths ────────────────────────────────────
    QString compiler = findCompiler();
    QString cRoot    = QDir::cleanPath(compilerRoot());   // compiler root folder

    // SDL3 is merged into the compiler tree during CI bundling:
    //   compiler/include/SDL3/   ← headers
    //   compiler/lib/libSDL3.a   ← import lib
    //   (SDL3.dll sits next to darvin.exe)
    // So if the bundled compiler has SDL3 headers, no extra -I/-L is needed.
    // We detect this and fall back to the old explicit paths for dev machines.
    bool sdl3Bundled = !cRoot.isEmpty() &&
                       QFile::exists(cRoot + "/include/SDL3/SDL.h");

    // ── Build argument list ────────────────────────────────────────────────
    QStringList args;
    args << fileInfo.fileName();                          // Source file

    if (usesSDL3) {
        if (!sdl3Bundled) {
            // Dev machine: SDL3 at the old explicit paths
            args << "-IC:/SDL3/x86_64-w64-mingw32/include"
                 << "-LC:/SDL3/x86_64-w64-mingw32/lib";
        }
        // If bundled, compiler already knows include/ and lib/ — no -I/-L needed
        args << "-lSDL3";
    }

    if (releaseMode) {
        args << "-mwindows"
             << "-static-libgcc"
             << "-static-libstdc++";
    }

    args << "-o" << fileInfo.baseName() + ".exe";
    // ──────────────────────────────────────────────────────────────────────

    QString modeLabel = releaseMode ? "[RELEASE]" : "[DEBUG]";
    terminalWidget->appendOutput(modeLabel + " Compiling " + fileInfo.fileName() + "...\n", "white");
    terminalWidget->appendOutput(compiler + " " + args.join(" ") + "\n", "#888888");

    QProcess proc;
    proc.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
    proc.setWorkingDirectory(fileInfo.absolutePath());
    proc.start(compiler, args);
    proc.waitForFinished();

    QString errors = proc.readAllStandardError();
    if (!errors.isEmpty()) {
        terminalDock->setWindowTitle("Output Console - 🔴 ERRORS FOUND");
        terminalWidget->appendHtml(
            "<span style='color:red;'><b>Compilation Failed:</b><br></span>" +
            errors.toHtmlEscaped().replace("\n", "<br>"));
        return false;
    }

    terminalDock->setWindowTitle("Output Console - 🟢 SUCCESS");
    terminalWidget->appendHtml("<span style='color:green;'><b>Success: 0 Errors.</b></span><br>");

    // ── Release: copy SDL3.dll next to the .exe if not already there ───────
    if (releaseMode && usesSDL3) {
        QString dllDest = fileInfo.absolutePath() + "/SDL3.dll";
        if (!QFile::exists(dllDest)) {
            // Try bundled location first, then dev machine location
            QString appDir  = QCoreApplication::applicationDirPath();
            QString dllSrc  = appDir + "/SDL3.dll";
            if (!QFile::exists(dllSrc))
                dllSrc = "C:/TDM-GCC-64/bin/SDL3.dll";

            if (QFile::copy(dllSrc, dllDest)) {
                terminalWidget->appendOutput("✔ Copied SDL3.dll → " + dllDest + "\n", "#4CAF50");
            } else {
                terminalWidget->appendOutput(
                    "⚠ Could not copy SDL3.dll automatically.\n"
                    "  Copy SDL3.dll next to your .exe before sharing.\n", "#FFA500");
            }
        }

        terminalWidget->appendHtml(
            "<br><span style='color:#4CAF50;'>"
            "<b>✅ Release build ready!</b><br>"
            "Share with students:<br>"
            "&nbsp;&nbsp;📄 " + fileInfo.baseName() + ".exe<br>"
            "&nbsp;&nbsp;📄 SDL3.dll"
            "</span><br>");
    }

    terminalWidget->appendOutput("──────────────────────────────────\n", "#444444");
    return true;
}

// ── Run / view ──────────────────────────────────────────────────────────────

void MainWindow::runCode()
{
    if (!compileCode(false)) return;

    QFileInfo fileInfo(currentFile);
    QString exePath = QDir::toNativeSeparators(
        fileInfo.absolutePath() + "/" + fileInfo.baseName() + ".exe");

    terminalDock->show();
    terminalDock->setWindowTitle("Output Console - ▶️ Running");

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString currentPath = env.value("PATH");
    QString appDir = QDir::toNativeSeparators(QCoreApplication::applicationDirPath());
    QString compilerBin = QDir::toNativeSeparators(QFileInfo(findCompiler()).absolutePath());
    env.insert("PATH", appDir + ";" + compilerBin + ";" + currentPath);
    terminalWidget->setProcessEnvironment(env);

    terminalWidget->setWorkingDirectory(fileInfo.absolutePath());
    terminalWidget->appendOutput(
        QString("Running: %1\n─────────────────────────────────\n").arg(exePath), "#888888");
    terminalWidget->startProcess(exePath, QStringList());
}

void MainWindow::viewOutput()
{
    terminalDock->show();
}