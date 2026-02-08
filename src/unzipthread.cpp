#include "unzipthread.h"

UnzipThread::UnzipThread(QString fileName, QString destPath, bool stripRoot)
{
    this->fileName = fileName;
    this->destPath = destPath;
    this->stripRoot = stripRoot;
}

void UnzipThread::run()
{
    int error = 0;
    zip_t *zip = zip_open(fileName.toLocal8Bit(), ZIP_RDONLY, &error);
    if (error != 0)
    {
        zip_discard(zip);
        emit unzip_fail("Unzip: Error opening zip file");
        return;
    }
    char buf[UNZIP_BUFFER_SIZE];

    // Clean up old files before extracting
    emit log("Removing old XMage files...");
    QDir(destPath + "/mage-client/lib").removeRecursively();
    QDir(destPath + "/mage-client/db").removeRecursively();
    QDir(destPath + "/mage-server/lib").removeRecursively();
    QDir(destPath + "/mage-server/db").removeRecursively();

    emit log("Unzipping file " + fileName);
    zip_int64_t numEntries = zip_get_num_entries(zip, 0);

    // Determine if zip has a root folder we need to skip
    // Check all entries to find common root directory prefix
    QString rootFolder;
    if (stripRoot && numEntries > 0)
    {
        QString commonPrefix;
        bool hasCommonRoot = true;

        for (zip_int64_t i = 0; i < numEntries && hasCommonRoot; i++)
        {
            zip_stat_t stat;
            if (zip_stat_index(zip, i, 0, &stat) != 0)
            {
                continue;
            }
            QString entryName(stat.name);
            int slashPos = entryName.indexOf('/');
            if (slashPos <= 0)
            {
                // Entry at root level or invalid - no common root
                hasCommonRoot = false;
                break;
            }
            QString prefix = entryName.left(slashPos + 1);
            if (commonPrefix.isEmpty())
            {
                commonPrefix = prefix;
            }
            else if (prefix != commonPrefix)
            {
                hasCommonRoot = false;
            }
        }

        if (hasCommonRoot && !commonPrefix.isEmpty())
        {
            rootFolder = commonPrefix;
            emit log("Stripping root folder: " + rootFolder);
        }
    }
    emit log("Extracting to: " + destPath);

    for (zip_int64_t i = 0; i < numEntries; i++)
    {
        emit progress(i, numEntries);
        zip_stat_t stat;
        if (zip_stat_index(zip, i, 0, &stat) != 0)
        {
            emit log("Unzip: Error getting info on file at index " + QString::number(i));
            continue;
        }

        QString entryName(stat.name);
        // Strip root folder if present
        if (!rootFolder.isEmpty() && entryName.startsWith(rootFolder))
        {
            entryName = entryName.mid(rootFolder.length());
            if (entryName.isEmpty())
            {
                continue; // Skip the root folder entry itself
            }
        }

        QString outPath = destPath + '/' + entryName;

        if (stat.name[strlen(stat.name) - 1] == '/')
        {
            QDir().mkpath(outPath);
            continue;
        }

        // Ensure parent directory exists
        QFileInfo(outPath).dir().mkpath(".");

        QSaveFile out(outPath);
        if (!out.open(QIODevice::WriteOnly))
        {
            emit log("Unzip: Error creating file " + entryName);
            continue;
        }
        zip_file_t *in = zip_fopen_index(zip, i, 0);
        if (in == NULL)
        {
            emit log("Unzip: Failed to open " + entryName);
            continue;
        }
        int len = zip_fread(in, buf, UNZIP_BUFFER_SIZE);
        bool writeError = false;
        while (len > 0)
        {
            if (out.write(buf, len) == -1)
            {
                emit log("Unzip: Error writing to file " + entryName);
                writeError = true;
                break;
            }
            len = zip_fread(in, buf, UNZIP_BUFFER_SIZE);
        }
        if (len == -1)
        {
            emit log("Unzip: Error reading from file " + entryName);
        }
        else if (!writeError)
        {
            out.commit();
        }
        if (zip_fclose(in) != 0)
        {
            emit log("Unzip: Error closing file " + entryName);
        }
    }
    zip_discard(zip);
    emit log("Unzip complete");
    emit unzip_complete(destPath);
}
