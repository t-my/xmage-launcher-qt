#include "zipextractthread.h"
#include <QDir>
#include <QSaveFile>
#include <zip.h>

#define EXTRACT_BUFFER_SIZE 4096

ZipExtractThread::ZipExtractThread(const QString &zipPath, const QString &destPath)
{
    this->zipPath = zipPath;
    this->destPath = destPath;
}

void ZipExtractThread::run()
{
    int error = 0;
    zip_t *zip = zip_open(zipPath.toLocal8Bit(), ZIP_RDONLY, &error);
    if (error != 0)
    {
        zip_discard(zip);
        emit extractFailed("Error opening zip file");
        return;
    }

    char buf[EXTRACT_BUFFER_SIZE];
    QDir().mkpath(destPath);

    emit log("Extracting...");
    zip_int64_t numEntries = zip_get_num_entries(zip, 0);

    for (zip_int64_t i = 0; i < numEntries; i++)
    {
        emit progress(i, numEntries);

        zip_stat_t stat;
        if (zip_stat_index(zip, i, 0, &stat) != 0)
        {
            emit log("Error getting info on file at index " + QString::number(i));
            continue;
        }

        QString outPath = destPath + "/" + stat.name;

        // Handle directory entries
        if (stat.name[strlen(stat.name) - 1] == '/')
        {
            QDir().mkpath(outPath);
            continue;
        }

        // Ensure parent directory exists
        QFileInfo fileInfo(outPath);
        QDir().mkpath(fileInfo.absolutePath());

        QSaveFile out(outPath);
        if (!out.open(QIODevice::WriteOnly))
        {
            emit log("Error creating file " + QString(stat.name));
            continue;
        }

        zip_file_t *in = zip_fopen_index(zip, i, 0);
        if (in == NULL)
        {
            emit log("Failed to open " + QString(stat.name));
            continue;
        }

        int len = zip_fread(in, buf, EXTRACT_BUFFER_SIZE);
        bool writeError = false;
        while (len > 0)
        {
            if (out.write(buf, len) == -1)
            {
                emit log("Error writing to file " + QString(stat.name));
                writeError = true;
                break;
            }
            len = zip_fread(in, buf, EXTRACT_BUFFER_SIZE);
        }

        if (len == -1)
        {
            emit log("Error reading from file " + QString(stat.name));
        }
        else if (!writeError)
        {
            out.commit();
        }

        zip_fclose(in);
    }

    zip_discard(zip);
    emit log("Extraction complete");
    emit extractComplete(destPath);
}
