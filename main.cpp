#include <stdio.h>
#include <signal.h>
#include <limits.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <string>
#include "Watch.h"

using std::cout;
using std::endl;

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + NAME_MAX + 1))
#define WATCH_FLAGS (IN_CREATE | IN_DELETE)

// Keep going  while run == true, or, in other words, until user hits ctrl-c
static bool run = true;

void sig_callback(int sig) {
    run = false;
}

int main(int argc, char *argv[]) {
    if (argc < 2 || string(argv[1]) == "--help") {
        cout << argv[0] << " pathname..." << endl;
        exit(EXIT_FAILURE);
    }

    // std::map used to keep track of wd (Watch descriptors) and directory names
    // As directory creation events arrive, they are added to the Watch map.
    // Directory delete events should be (but currently aren't in this sample) handled the same way.
    Watch watch;

    // watch_set is used by select to wait until inotify returns some data to
    // be read using non-blocking read.
    fd_set watch_set;

    char buffer[EVENT_BUF_LEN];
    string current_dir, new_dir;
    int total_file_events = 0;
    int total_dir_events = 0;

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

    // Continue until run == false. See signal and sig_callback above.
    while (run) {
        // select waits until inotify has 1 or more events.
        // select syntax is beyond the scope of this sample but, don't worry, the fd+1 is correct:
        // select needs the the highest fd (+1) as the first parameter.
        select(fd + 1, &watch_set, NULL, NULL, NULL);

        // Read event(s) from non-blocking inotify fd (non-blocking specified in inotify_init1 above).
        int length = read(fd, buffer, EVENT_BUF_LEN);
        if (length < 0) {
            perror("read");
        }

        // Loop through event buffer
        for (int i = 0; i < length;) {
            struct inotify_event *event = (struct inotify_event *) &buffer[i];
            // Never actually seen this
            if (event->wd == -1) {
                printf("Overflow\n");
            }
            // Never seen this either
            if (event->mask & IN_Q_OVERFLOW) {
                printf("Overflow\n");
            }
            if (event->len) {
                if (event->mask & IN_IGNORED) {
                    printf("IN_IGNORED\n");
                }
                if (event->mask & IN_CREATE) {
                    current_dir = watch.get(event->wd);
                    if (event->mask & IN_ISDIR) {
                        new_dir = current_dir + "/" + event->name;
                        wd = inotify_add_watch(fd, new_dir.c_str(), WATCH_FLAGS);
                        watch.insert(event->wd, event->name, wd);
                        total_dir_events++;
                        printf("New directory %s created.\n", new_dir.c_str());
                    } else {
                        total_file_events++;
                        printf("New file %s/%s created.\n", current_dir.c_str(), event->name);
                    }
                } else if (event->mask & IN_DELETE) {
                    if (event->mask & IN_ISDIR) {
                        new_dir = watch.erase(event->wd, event->name, &wd);
                        inotify_rm_watch(fd, wd);
                        total_dir_events--;
                        printf("Directory %s deleted.\n", new_dir.c_str());
                    } else {
                        current_dir = watch.get(event->wd);
                        total_file_events--;
                        printf("File %s/%s deleted.\n", current_dir.c_str(), event->name);
                    }
                }
            }
            i += EVENT_SIZE + event->len;
        }
    }

    // Cleanup
    printf("cleaning up\n");
    cout << "total dir events = " << total_dir_events << ", total file events = " << total_file_events << endl;
    watch.cleanup(fd);
    close(fd);
    fflush(stdout);
}
