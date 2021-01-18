#include "Watch.h"

// Insert event information, used to create new Watch, into Watch object.
void Watch::insert(int pd, const string &name, int wd) {
    wd_elem elem = {pd, name};
    watch[wd] = elem;
    rwatch[elem] = wd;
}

// Erase Watch specified by pd (parent Watch descriptor) and name from Watch list.
// Returns full name (for display etc), and wd, which is required for inotify_rm_watch.
string Watch::erase(int pd, const string &name, int *wd) {
    wd_elem pelem = {pd, name};
    *wd = rwatch[pelem];
    rwatch.erase(pelem);
    const wd_elem &elem = watch[*wd];
    string dir = elem.name;
    watch.erase(*wd);
    return dir;
}

// Given a Watch descriptor, return the full directory name as string. Recourses up parent WDs to assemble name,
// an idea borrowed from Windows change journals.
string Watch::get(int wd) {
    const wd_elem &elem = watch[wd];
    return elem.pd == -1 ? elem.name : this->get(elem.pd) + "/" + elem.name;
}

void Watch::cleanup(int fd) {
    for (map<int, wd_elem>::iterator wi = watch.begin(); wi != watch.end(); wi++) {
        inotify_rm_watch(fd, wi->first);
        watch.erase(wi);
    }
    rwatch.clear();
}