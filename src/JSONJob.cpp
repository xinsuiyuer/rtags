#include "JSONJob.h"
#include "CursorInfo.h"
#include "Project.h"
#include "RTags.h"
#include "Server.h"

JSONJob::JSONJob(const QueryMessage &q, const shared_ptr<Project> &project)
    : Job(q, WriteUnfiltered|QuietJob, project), match(q.match())
{
    assert(project.get());
}

static String toJSON(const Location &loc, uint32_t fileId, int length, int srcRootLength)
{
    if (loc.fileId() == fileId) {
        return String::format<64>("{\"offset\":%d,\"length\":%d}", loc.offset(), length);
    } else {
        return String::format<64>("{\"file\":\"%s\",\"offset\":%d,\"length\":%d}",
                                     Location::path(loc.fileId()).constData() + srcRootLength, loc.offset(), length);
    }
}

void JSONJob::execute()
{
    shared_ptr<Project> proj = project();
    const Path root = proj->path();
    const DependencyMap deps = proj->dependencies();
    // error() << deps.keys();
    assert(proj);
    const SymbolMap &map = proj->symbols();
    write("{");
    bool firstObject = true;
    for (DependencyMap::const_iterator it = deps.begin(); it != deps.end(); ++it) {
        const Path path = Location::path(it->first);
        if (path.startsWith(root) && (match.isEmpty() || match.match(path))) {
            const Location loc(it->first, 0);
            const int srcRootLength = project()->path().size();
            if (firstObject) {
                firstObject = false;
            } else {
                write(",");
            }
            write<64>("\"%s\":[", path.constData() + srcRootLength);
            bool firstSymbol = true;
            SymbolMap::const_iterator sit = map.lower_bound(loc);
            while (sit != map.end() && sit->first.fileId() == it->first) {
                Location targetLocation;
                CursorInfo target = sit->second.bestTarget(map, 0, &targetLocation);
                const String type = sit->second.kindSpelling();
                if (firstSymbol) {
                    firstSymbol = false;
                } else {
                    write(",");
                }
                if (!targetLocation.isNull()) {
                    write<256>("{\"location\":%s,\"type\":\"%s\",\"target\":%s}",
                               toJSON(sit->first, it->first, sit->second.symbolLength, srcRootLength).constData(), type.constData(),
                               toJSON(targetLocation, it->first, target.symbolLength, srcRootLength).constData());
                } else {
                    write<256>("{\"location\":%s,\"type\":\"%s\"}",
                               toJSON(sit->first, it->first, sit->second.symbolLength, srcRootLength).constData(), type.constData());
                }

                ++sit;
            }
            write("]");
        }
    }
    write("}");
}

