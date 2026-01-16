#ifndef ZIPEXTRACTTHREAD_H
#define ZIPEXTRACTTHREAD_H

#include <QString>
#include <QThread>

class ZipExtractThread : public QThread
{
    Q_OBJECT
public:
    ZipExtractThread(const QString &zipPath, const QString &destPath);
    void run() override;

private:
    QString zipPath;
    QString destPath;

signals:
    void log(QString message);
    void progress(qint64 complete, qint64 total);
    void extractComplete(QString extractedPath);
    void extractFailed(QString errorMessage);
};

#endif // ZIPEXTRACTTHREAD_H
