#ifndef Log_h
#define Log_h

#include <QString>
#include <QDebug>
#include <QExplicitlySharedDataPointer>
#include <QSharedData>

void log(int level, const char *format, ...);
void debug(const char *format, ...);
void warning(const char *format, ...);
enum LogFlag {
    Append = 0x1,
    // ### rotate log files?
};
bool initLogging(int logLevel, const QByteArray &logFile, unsigned flags);
int logLevel();

class Log
{
public:
    Log(int level = 0);
    Log(const Log &other);
    Log &operator=(const Log &other);
    template <typename T> Log &operator<<(const T &t)
    {
        if (mData) {
            *mData->dbg << t;
            return *this;
        }
        return *this;
    }
private:
    class Data : public QSharedData
    {
    public:
        Data(int lvl)
            : level(lvl)
        {
            dbg = new QDebug(&out);
        }
        ~Data()
        {
            delete dbg;
            log(level, "%s", qPrintable(out));
        }

        const int level;
        QDebug *dbg;
        QString out;
    };

    QExplicitlySharedDataPointer<Data> mData;
};

static inline Log log(int level = 0)
{
    return Log(level);
}
static inline Log warning()
{
    return Log(0);
}

static inline Log debug()
{
    return Log(1);
}

static inline Log verboseDebug()
{
    return Log(2);
}

#endif