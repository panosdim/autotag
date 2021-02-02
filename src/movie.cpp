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
#include <tagparser/mediafileinfo.h>
#include <tagparser/diagnostics.h>
#include <tagparser/progressfeedback.h>
#include <tagparser/abstractattachment.h>
#include <iostream>

bool logLineFinalized = true;
static string lastStep;

using namespace std;

void logNextStep(const TagParser::AbortableProgressFeedback &progress) {
    // finalize previous step
    if (!logLineFinalized) {
        cout << "\r - [100%] " << lastStep << endl;
        logLineFinalized = true;
    }
    // print line for next step
    lastStep = progress.step();
    cout << "\r - [" << setw(3) << static_cast<unsigned int>(progress.stepPercentage()) << "%] " << lastStep << flush;
    logLineFinalized = false;
}

void logStepPercentage(const TagParser::AbortableProgressFeedback &progress) {
    cout << "\r - [" << setw(3) << static_cast<unsigned int>(progress.stepPercentage()) << "%] " << lastStep << flush;
}

void finalizeLog() {
    if (logLineFinalized) {
        return;
    }
    cout << "\r - [100%] " << lastStep << '\n';
    logLineFinalized = true;
    lastStep.clear();
}

bool extract_movie_info(const filesystem::path &movieFile, Movie &movieInfo) {
    regex movieRegex(R"(([ .\w']+?)\W(\d{4})\W?.*)");
    smatch movieResult;
    if (movieFile.extension() == ".mp4" || movieFile.extension() == ".mkv") {
        string filename = movieFile.filename();
        if (regex_match(filename, movieResult, movieRegex)) {
            movieInfo.title = regex_replace(movieResult[1].str(), regex("\\."), " ");
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
        LOG(ERROR) << "No metadata iTunes style found";
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
    }
    APar_FreeMemory();
}

void save_mkv_cover(const string &cover, const Movie &movieInfo) {
    TagParser::MediaFileInfo fileInfo;
    TagParser::Diagnostics diag;

    try {
        fileInfo.setPath(movieInfo.path);
        fileInfo.open();
        fileInfo.parseEverything(diag);
        auto container = fileInfo.container();

        if (fileInfo.attachmentsParsingStatus() == TagParser::ParsingStatus::Ok && container) {
            for (size_t i = 0, count = container->attachmentCount(); i < count; ++i) {
                auto attachment = container->attachment(i);
                if (attachment->mimeType() == "image/jpeg") {
                    attachment->setIgnored(true);
                    LOG(INFO) << "Existing cover found and marked for removal";
                }
            }
            auto new_attachment = container->createAttachment();
            new_attachment->setName("cover");
            new_attachment->setFile(cover, diag);

            // Create progress
            TagParser::AbortableProgressFeedback progress(logNextStep, logStepPercentage);

            // Save changes
            LOG(INFO) << "Saving changes";
            fileInfo.applyChanges(diag, progress);

            // notify about completion
            finalizeLog();
        }
    } catch (const TagParser::Failure &error) {
        LOG(ERROR) << "A parsing failure occurred when reading the file " << movieInfo.path;
    } catch (const ios_base::failure &) {
        LOG(ERROR) << "An IO failure occurred when reading the file " << movieInfo.path;
    }
}
