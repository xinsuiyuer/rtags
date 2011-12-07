#ifndef Database_h
#define Database_h

#include "Path.h"
#include "Location.h"
#include <QtCore>

class Connection
{
public:
    Connection() {}
    virtual ~Connection() {}

    virtual QByteArray readData(const QByteArray &key) const = 0;
    virtual void writeData(const QByteArray &key, const QByteArray &value) = 0;
};

struct DictionaryEntry {
    QList<QByteArray> scope;
    QSet<Location> locations;
};

static inline QDebug operator<<(QDebug dbg, const DictionaryEntry &entry)
{
    dbg << "DictionaryEntry(";
    dbg.nospace() << entry.scope << ", " << entry.locations << ")";
    return dbg.maybeSpace();
}

static inline bool operator==(const DictionaryEntry &l, const DictionaryEntry &r)
{
    return (l.scope == r.scope && l.locations == r.locations);
}

static inline QDataStream &operator<<(QDataStream &ds, const DictionaryEntry &entry)
{
    return (ds << entry.scope << entry.locations);
}
static inline QDataStream &operator>>(QDataStream &ds, DictionaryEntry &entry)
{
    return (ds >> entry.scope >> entry.locations);
}

static inline uint qHash(const DictionaryEntry &entry)
{
    uint ret = 0;
    int idx = 0;
    foreach(const QByteArray &s, entry.scope) {
        ret += (::qHash(s) << idx++);
        // ### is this good?
    }
    return ret;
}

class Database
{
public:
    Database();
    virtual ~Database();

    enum Mode {
        ReadOnly,
        WriteOnly,
        ReadWrite
    };
    static Database* create(const Path &path, Mode mode);

    Mode mode() const { return mMode; }
    Path path() const { return mPath; }
    bool open(const Path &db, Mode mode);
    void close();
    virtual bool isOpened() const = 0;
    Location followLocation(const Location &source) const;
    QSet<Location> findReferences(const Location &location) const;
    QSet<Location> findSymbol(const QByteArray &symbolName) const;
    QList<QByteArray> symbolNames(const QByteArray &filter) const;
    void writeEntity(const QByteArray &symbolName,
                     const QList<QByteArray> &parentNames,
                     const Location &definition,
                     const QSet<Location> &declarations,
                     const QSet<Location> &references);

    void invalidateEntries(const QSet<Path> &paths);

    Location createLocation(const QByteArray &arg, const Path &cwd = Path());
    QByteArray locationToString(const Location &location) const;

    enum ConnectionType {
        General = 0,
        Dictionary,
        References,
        Targets,
        NumConnectionTypes
    };
    
    class iterator
    {
    public:
        iterator(ConnectionType t)
            : type(t)
        {}

        virtual ~iterator() {}
        virtual QByteArray value() const = 0;
        virtual QByteArray key() const = 0;
        virtual bool seek(const QByteArray &key) = 0;
        virtual bool next() = 0;
        virtual bool isValid() const = 0;
        const ConnectionType type;

        template <typename T> T value(const T &defaultValue = T()) const
        {
            const QByteArray val = value();
            if (val.isEmpty())
                return defaultValue;
            return Database::decode<T>(val);
        }
    };


    template <typename T> T read(const QByteArray &key, const T &defaultValue = T()) const
    {
        return read<T>(General, key, defaultValue);
    }
    template <typename T> void write(const QByteArray &key, const T &t)
    {
        write(General, key, t);
    }

    void remove(const QByteArray &key)
    {
        remove(General, key);
    }

    template <typename T> static QByteArray encode(const T &t)
    {
        QByteArray v;
        {
            QDataStream ds(&v, QIODevice::WriteOnly);
            ds << t;
        }
        return v;
    }

    static QByteArray encode(const QByteArray &ba)
    {
        return ba;
    }

    template <typename T> static T decode(const QByteArray &encoded)
    {
        T t;
        if (!encoded.isEmpty()) {
            QDataStream ds(encoded);
            ds >> t;
        }
        return t;
    }
    static QByteArray decode(const QByteArray &data)
    {
        return data;
    }

    virtual iterator *createIterator(ConnectionType) const = 0;
protected:
    virtual bool openDatabase(const Path &db, Mode mode) = 0;
    virtual void closeDatabase() = 0;
    virtual Connection *createConnection(ConnectionType type) = 0;
private:
    template <typename T> T read(ConnectionType type, const QByteArray &key, const T &defaultValue) const
    {
        if (key.isEmpty())
            return T();
        const QByteArray dat = mConnections[type]->readData(key);
        if (dat.isEmpty())
            return defaultValue;
        return decode<T>(dat);
    }
    template <typename T> void write(ConnectionType type, const QByteArray &key, const T &t)
    {
        Q_ASSERT(!key.isEmpty());
        mConnections[type]->writeData(key, encode<T>(t));
    }
    void remove(ConnectionType type, const QByteArray &key)
    {
        Q_ASSERT(!key.isEmpty());
        mConnections[type]->writeData(key, QByteArray());
    }
    

    Path mPath;
    Mode mMode;
    Connection *mConnections[NumConnectionTypes];
    QHash<Path, unsigned> mFilesByName;
    QHash<unsigned, Path> mFilesByIndex;

    QHash<QByteArray, QSet<DictionaryEntry> > mDictionary;
    int mRefIdxCounter;
};

#endif