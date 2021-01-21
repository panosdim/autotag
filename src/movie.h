#ifndef AUTOTAG_MOVIE_H
#define AUTOTAG_MOVIE_H

#include <string>
#include <filesystem>

using std::string;

enum FileType {
    MKV, MP4
};

struct MovieInfo {
    string title;
    string releaseYear;
    string path;
    FileType fileType;
};

bool extract_movie_info(const std::filesystem::path &path, MovieInfo &movieInfo);

#endif //AUTOTAG_MOVIE_H
