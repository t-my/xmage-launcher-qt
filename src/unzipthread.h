#ifndef UNZIPTHREAD_H
#define UNZIPTHREAD_H

#include <QDir>
#include <QFileInfo>
#include <QSaveFile>
#include <QString>
#include <QThread>
#include <zip.h>

#define UNZIP_BUFFER_SIZE 1024

class UnzipThread : public QThread
{
    Q_OBJECT
public:
    UnzipThread(QString fileName, QString destPath, bool stripRoot = true);
    void run() override;

private:
    QString fileName;
    QString destPath;
    bool stripRoot;

signals:
    void log(QString message);
    void progress(qint64 complete, qint64 total);
    void unzip_complete(QString installLocation);
    void unzip_fail(QString errorMessage);
};

#endif // UNZIPTHREAD_H
