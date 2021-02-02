#ifndef AUTOTAG_MOVIE_H
#define AUTOTAG_MOVIE_H

#include <string>
#include <filesystem>

using std::string;

enum FileType {
    MKV, MP4
};

struct Movie {
    string title;
    string releaseYear;
    string path;
    FileType fileType;
};

bool extract_movie_info(const std::filesystem::path &path, Movie &movieInfo);

void save_mp4_cover(const string &cover, const Movie &movieInfo);

void save_mkv_cover(const string &cover, const Movie &movieInfo);

#endif //AUTOTAG_MOVIE_H
