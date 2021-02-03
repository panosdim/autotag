# AutoTag

Application that wait for inotify a folder for MP4 and MKV movie files and automatic attach cover art in the files. In
order to find the cover art it uses the [TMDB API](https://developers.themoviedb.org/3/getting-started/introduction).

## Usage

AutoTag can be run manually or as a daemon in a systemd compatible system. To run it manually use

```
$ autotag "/mnt/Movies"
```

## Installation

In order to install first you need to build it with `$ cmake -DCMAKE_BUILD_TYPE=Release -B Release .` and then install
it `$ install.sh`. If you are on a Linux machine with systemd, installer will ask you if you want to run it as a daemon.

## Prerequisites

The development headers for the following libraries should be installed:

* libcurl
* zlib

```
sudo apt install libcurl4-openssl-dev zlib1g-dev
```

## Libraries Used

- [tagparser](https://github.com/Martchus/tagparser)
- [glog](https://github.com/google/glog)
- [json](https://github.com/nlohmann/json)
- [restclient-cpp](https://github.com/mrtazz/restclient-cpp)
- [simpleini](https://github.com/brofield/simpleini)