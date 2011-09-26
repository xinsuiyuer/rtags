#ifndef DAEMON_H
#define DAEMON_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QThreadPool>
#include <clang-c/Index.h>
#include "Utils.h"
#include "GccArguments.h"
#include "Path.h"
#include "Location.h"

struct Node;
class RBuild : public QObject
{
    Q_OBJECT;
public:
    RBuild(QObject* parent = 0);
    bool addMakefile(Path makefile);
    void recurseDir(const Path &path);
    void setDatabaseFile(const Path &path);
    Path databaseFile() const;
private slots:
    void maybeDone();
    void onMakeFinished(int statusCode);
    void onMakeOutput();
    void onMakeError(QProcess::ProcessError error);
    void onClangRunnableFinished();
private:
    struct MakefileData {
        Path path, directory;
        QByteArray buffer;
        QHash<Path, QList<GccArguments> > seen;
        Path workingDirectory;
    };
    QHash<QProcess *, MakefileData> mMakefiles;
    QThreadPool mThreadPool;
    int mPendingRunnables;
    Path mDatabaseFile;
};

#endif
