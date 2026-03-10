#include "terminalwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QProcess>
#include <QScrollBar>
#include <QTextCursor>

TerminalWidget::TerminalWidget(QWidget *parent)
    : QWidget(parent)
{
    process = new QProcess(this);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Top Bar
    QHBoxLayout *topLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("▶ Console Output", this);
    stopButton = new QPushButton("[■ Stop]", this);
    stopButton->setEnabled(false);
    
    topLayout->addWidget(titleLabel);
    topLayout->addStretch();
    topLayout->addWidget(stopButton);

    mainLayout->addLayout(topLayout);

    // Console text edit
    consoleTextEdit = new QTextEdit(this);
    consoleTextEdit->setReadOnly(true);
    consoleTextEdit->setStyleSheet("background-color: #1e1e1e; color: white; font-family: Consolas; font-size: 11pt;");
    mainLayout->addWidget(consoleTextEdit);

    // Input area
    QHBoxLayout *inputLayout = new QHBoxLayout();
    QLabel *promptLabel = new QLabel("› ", this);
    promptLabel->setStyleSheet("color: #4CAF50; font-weight: bold; font-family: Consolas; font-size: 11pt;");
    
    inputLine = new QLineEdit(this);
    inputLine->setStyleSheet("background-color: #2b2b2b; color: #4CAF50; border: 1px solid #4CAF50; font-family: Consolas; font-size: 11pt;");
    inputLine->setEnabled(false);

    inputLayout->addWidget(promptLabel);
    inputLayout->addWidget(inputLine);

    mainLayout->addLayout(inputLayout);

    // Connections
    connect(process, &QProcess::readyReadStandardOutput, this, &TerminalWidget::onReadyReadStandardOutput);
    connect(process, &QProcess::readyReadStandardError, this, &TerminalWidget::onReadyReadStandardError);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TerminalWidget::onProcessFinished);
    
    connect(inputLine, &QLineEdit::returnPressed, this, &TerminalWidget::onInputReturned);
    connect(stopButton, &QPushButton::clicked, this, &TerminalWidget::onStopClicked);
}

TerminalWidget::~TerminalWidget()
{
    if (process->state() != QProcess::NotRunning) {
        process->kill();
        process->waitForFinished();
    }
}

void TerminalWidget::setWorkingDirectory(const QString &dir)
{
    process->setWorkingDirectory(dir);
}

void TerminalWidget::setProcessEnvironment(const QProcessEnvironment &env)
{
    process->setProcessEnvironment(env);
}

void TerminalWidget::stopProcess()
{
    if (process->state() != QProcess::NotRunning) {
        process->kill();
        process->waitForFinished();
    }
}

void TerminalWidget::startProcess(const QString &program, const QStringList &arguments)
{
    inputLine->setEnabled(true);
    inputLine->clear();
    inputLine->setFocus();
    
    stopButton->setEnabled(true);
    
    process->start(program, arguments);
}

void TerminalWidget::appendHtml(const QString &html)
{
    consoleTextEdit->moveCursor(QTextCursor::End);
    consoleTextEdit->insertHtml(html);
    consoleTextEdit->moveCursor(QTextCursor::End);
    consoleTextEdit->verticalScrollBar()->setValue(consoleTextEdit->verticalScrollBar()->maximum());
}

void TerminalWidget::appendOutput(const QString &text, const QString &color)
{
    // Append html
    QString html = QString("<span style='color:%1;'>%2</span>").arg(color, text.toHtmlEscaped().replace("\n", "<br>"));
    appendHtml(html);
}

void TerminalWidget::clear()
{
    consoleTextEdit->clear();
}

void TerminalWidget::onReadyReadStandardOutput()
{
    QByteArray data = process->readAllStandardOutput();
    appendOutput(QString::fromLocal8Bit(data), "white");
}

void TerminalWidget::onReadyReadStandardError()
{
    QByteArray data = process->readAllStandardError();
    appendOutput(QString::fromLocal8Bit(data), "#FFA500"); // Orange
}

void TerminalWidget::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    inputLine->setEnabled(false);
    stopButton->setEnabled(false);
    
    if (exitStatus == QProcess::CrashExit) {
        appendOutput("\n\n[Process Crashed]\n", "red");
    } else {
        QString color = (exitCode == 0) ? "#4CAF50" : "red"; // Green if 0, else Red
        appendOutput(QString("\n\n[Program exited with code %1]\n").arg(exitCode), color);
    }
    emit processFinished(exitCode);
}

void TerminalWidget::onInputReturned()
{
    QString text = inputLine->text();
    inputLine->clear();
    
    if (process->state() == QProcess::Running) {
        // Echo the input
        appendOutput(text + "\n", "#4CAF50");
        
        QByteArray data = text.toLocal8Bit();
        data.append('\n');
        process->write(data);
    }
}

void TerminalWidget::onStopClicked()
{
    if (process->state() != QProcess::NotRunning) {
        process->kill();
    }
}
