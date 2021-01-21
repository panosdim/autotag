#include "movie.h"
#include <regex>
#include "AtomicParsley/AP_commons.h"
#include "AtomicParsley/AtomicParsley.h"
#include "AtomicParsley/AP_AtomExtracts.h"
#include "AtomicParsley/AP_iconv.h"
#include "AtomicParsley/AtomicParsley_genres.h"
#include "AtomicParsley/APar_uuid.h"
#include <glog/logging.h>
#include <cstdio>


bool extract_movie_info(const std::filesystem::path &movieFile, Movie &movieInfo) {
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

void save_mp4_cover(const string &cover, const Movie &movieInfo) {
    char *env_PicOptions = getenv("PIC_OPTIONS");
    APar_ScanAtoms(movieInfo.path.c_str());
    if (!APar_assert(metadata_style == ITUNES_STYLE, 1, (char *) "coverart")) {
        PLOG(ERROR) << "No metadata iTunes style found";
    }

    APar_MetaData_atomArtwork_Set(cover.c_str(), env_PicOptions);
    if (modified_atoms) {
        APar_DetermineAtomLengths();
        openSomeFile(movieInfo.path.c_str(), true);
        string output_file = movieInfo.path + "-temp";
        APar_WriteFile(movieInfo.path.c_str(), output_file.c_str(), false);

        // Replace original file
        int status = remove(movieInfo.path.c_str());
        if (status != 0) {
            LOG(ERROR) << "Error deleting original file! " << movieInfo.path;
        }

        status = rename(output_file.c_str(), movieInfo.path.c_str());
        if (status != 0) {
            LOG(ERROR) << "Error renaming temp file! " << output_file;
        }

        // Delete cover
        status = remove(cover.c_str());
        if (status != 0) {
            LOG(ERROR) << "Cover File was not deleted!";
        }
    }
    APar_FreeMemory();
}
