#ifndef TERMINALWIDGET_H
#define TERMINALWIDGET_H

#include <QWidget>
#include <QProcess>
#include <QProcessEnvironment>
#include <QString>

class QTextEdit;
class QLineEdit;
class QPushButton;

class TerminalWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TerminalWidget(QWidget *parent = nullptr);
    ~TerminalWidget();

    void setWorkingDirectory(const QString &dir);
    void setProcessEnvironment(const QProcessEnvironment &env);
    void startProcess(const QString &program, const QStringList &arguments);
    void stopProcess();
    void appendHtml(const QString &html);
    void appendOutput(const QString &text, const QString &color = "white");
    void clear();

signals:
    void processFinished(int exitCode);

private slots:
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onInputReturned();
    void onStopClicked();

private:
    QProcess *process;
    QTextEdit *consoleTextEdit;
    QLineEdit *inputLine;
    QPushButton *stopButton;
};

#endif // TERMINALWIDGET_H
