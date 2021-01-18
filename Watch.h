// Watch class keeps track of Watch descriptors (wd), parent Watch descriptors (pd), and names (from event->name).
// The class provides some helpers for inotify, primarily to enable recursive monitoring:
// 1. To add a Watch (inotify_add_watch), a complete path is needed, but events only provide file/dir name with no path.
// 2. Delete events provide parent Watch descriptor and file/dir name, but removing the Watch (infotify_rm_watch) needs a wd.
//

#ifndef AUTOTAG_WATCH_H
#define AUTOTAG_WATCH_H

#include <sys/inotify.h>
#include <string>
#include <map>

using std::map;
using std::string;

class Watch {
    struct wd_elem {
        int pd;
        string name;

        bool operator()(const wd_elem &l, const wd_elem &r) const {
            return l.pd < r.pd ? true : l.pd == r.pd && l.name < r.name ? true : false;
        }
    };

    map<int, wd_elem> watch;
    map<wd_elem, int, wd_elem> rwatch;

public:
    // Insert event information, used to create new Watch, into Watch object.
    void insert(int pd, const string &name, int wd);

    // Erase Watch specified by pd (parent Watch descriptor) and name from Watch list.
    // Returns full name (for display etc), and wd, which is required for inotify_rm_watch.
    string erase(int pd, const string &name, int *wd);

    // Given a Watch descriptor, return the full directory name as string. Recourses up parent WDs to assemble name,
    // an idea borrowed from Windows change journals.
    string get(int wd);

    // Given a parent wd and name (provided in IN_DELETE events), return the Watch descriptor.
    // Main purpose is to help remove directories from Watch list.
    int get(int pd, string name);

    void cleanup(int fd);
};

#endif //AUTOTAG_WATCH_H
