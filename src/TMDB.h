#ifndef AUTOTAG_TMDB_H
#define AUTOTAG_TMDB_H

#include <string>
#include <cstdio>
#include "movie.h"

using std::string;

class TMDB {
    string apiKey = "?api_key=";
    const string baseUrl = "https://api.themoviedb.org/3/";
    string imageBaseUrl;
public:
    explicit TMDB(const string &token);

    FILE *downloadCover(const MovieInfo &movieInfo);
};


#endif //AUTOTAG_TMDB_H
