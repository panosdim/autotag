#include <cstdio>
#include <csignal>
#include <climits>
#include <sys/inotify.h>
#include <iostream>
#include <unistd.h>
#include <string>
#include <filesystem>
#include <glog/logging.h>
#include "Watch.h"
#include "movie.h"
#include "SimpleIni.h"
#include "TMDB.h"

using std::cout;
using std::endl;
namespace fs = std::filesystem;

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + NAME_MAX + 1))
#define WATCH_FLAGS (IN_CREATE | IN_DELETE | IN_CLOSE)

// Keep going  while run == true, or, in other words, until user hits ctrl-c
static bool run = true;

void sig_callback([[maybe_unused]] int sig) {
    run = false;
}

int main(int argc, char *argv[]) {
    if (argc < 2 || string(argv[1]) == "--help") {
        cout << argv[0] << " pathname..." << endl;
        exit(EXIT_FAILURE);
    }

    // Initialize Google’s logging library.
    google::InitGoogleLogging(argv[0]);

    // Read ini file
    CSimpleIniA ini;
    ini.SetUnicode();

    SI_Error rc = ini.LoadFile("config.ini");
    if (rc < 0) {
        LOG(ERROR) << "Can't open config.ini file.";
        return -1;
    }

    auto token = ini.GetValue("TMDB", "apiKey", "NOT_FOUND");
    TMDB tmdb(token);

    // std::map used to keep track of wd (Watch descriptors) and directory names
    // As directory creation events arrive, they are added to the Watch map.
    // Directory delete events should be (but currently aren't in this sample) handled the same way.
    Watch watch;

    // watch_set is used by select to wait until inotify returns some data to
    // be read using non-blocking read.
    fd_set watch_set;

    char buffer[EVENT_BUF_LEN];
    string current_dir, new_dir;

    // Call sig_callback if user hits ctrl-c
    signal(SIGINT, sig_callback);

    // creating the INOTIFY instance
    // inotify_init1 not available with older kernels, consequently inotify reads block.
    // inotify_init1 allows directory events to complete immediately, avoiding buffering delays. In practice,
    // this significantly improves monitoring of newly created subdirectories.
#ifdef IN_NONBLOCK
    int fd = inotify_init1(IN_NONBLOCK);
#else
    int fd = inotify_init();
#endif

    // checking for error
    if (fd < 0) {
        perror("inotify_init");
    }

    // use select Watch list for non-blocking inotify read
    FD_ZERO(&watch_set);
    FD_SET(fd, &watch_set);

    // add argument to Watch list. Normally, should check directory exists first
    const char *root = argv[1];
    int wd = inotify_add_watch(fd, root, WATCH_FLAGS);

    // add wd and directory name to Watch map
    watch.insert(-1, root, wd);

    string processed_file;

    // Continue until run == false. See signal and sig_callback above.
    while (run) {
        // select waits until inotify has 1 or more events.
        // select syntax is beyond the scope of this sample but, don't worry, the fd+1 is correct:
        // select needs the the highest fd (+1) as the first parameter.
        select(fd + 1, &watch_set, nullptr, nullptr, nullptr);

        // Read event(s) from non-blocking inotify fd (non-blocking specified in inotify_init1 above).
        int length = read(fd, buffer, EVENT_BUF_LEN);
        if (length < 0) {
            perror("read");
        }

        // Loop through event buffer
        for (int i = 0; i < length;) {
            auto *event = (struct inotify_event *) &buffer[i];
            if (event->len) {
                if (event->mask & IN_CREATE) {
                    current_dir = watch.get(event->wd);
                    if (event->mask & IN_ISDIR) {
                        new_dir = current_dir + "/" + event->name;
                        wd = inotify_add_watch(fd, new_dir.c_str(), WATCH_FLAGS);
                        watch.insert(event->wd, event->name, wd);
                        LOG(INFO) << "Watch new directory " << new_dir;
                    }
                } else if (event->mask & IN_DELETE) {
                    if (event->mask & IN_ISDIR) {
                        watch.erase(event->wd, event->name, &wd);
                        inotify_rm_watch(fd, wd);
                        LOG(INFO) << "Remove watch of deleted directory " << event->name;
                    }
                } else if (event->mask & IN_CLOSE) {
                    if (!(event->mask & IN_ISDIR)) {
                        fs::path new_file(current_dir);
                        new_file /= event->name;
                        Movie movieInfo;

                        if (extract_movie_info(new_file, movieInfo)) {
                            if (processed_file != new_file) {
                                processed_file = new_file;
                                LOG(INFO) << "New movie found in " << movieInfo.path << " with title "
                                          << movieInfo.title
                                          << " and released at "
                                          << movieInfo.releaseYear;
                                string cover = tmdb.downloadCover(movieInfo);
                                if (cover.empty()) {
                                    LOG(ERROR) << "Cover file not found";
                                } else {
                                    switch (movieInfo.fileType) {
                                        case MP4:
                                            save_mp4_cover(cover, movieInfo);
                                        case MKV:
                                            save_mkv_cover(cover, movieInfo);
                                    }

                                    // Delete cover
                                    int status = remove(cover.c_str());
                                    if (status != 0) {
                                        LOG(ERROR) << "Cover File was not deleted!";
                                    }
                                }
                            }
                        }
                    }
                }
            }
            i += EVENT_SIZE + event->len; // NOLINT(cppcoreguidelines-narrowing-conversions)
        }
    }

    // Cleanup
    cout << "Cleaning Up" << endl;
    watch.cleanup(fd);
    close(fd);
    fflush(stdout);
}
