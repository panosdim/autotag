#include "movie.h"
#include <regex>

bool extract_movie_info(const std::filesystem::path &movieFile, MovieInfo &movieInfo) {
    std::regex movieRegex(R"(([ .\w']+?)\W(\d{4})\W?.*)");
    std::smatch movieResult;
    if (movieFile.extension() == ".mp4" || movieFile.extension() == ".mkv") {
        string filename = movieFile.filename();
        if (std::regex_match(filename, movieResult, movieRegex)) {
            movieInfo.title = std::regex_replace(movieResult[1].str(), std::regex("\\."), " ");
            movieInfo.releaseYear = movieResult[2];
            movieInfo.path = movieFile;
            movieInfo.fileType = movieFile.extension() == ".mp4" ? MP4 : MKV;

            return true;
        }
    }

    return false;
}
