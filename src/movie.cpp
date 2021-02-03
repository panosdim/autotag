#include "movie.h"
#include <regex>
#include <glog/logging.h>
#include <cstdio>
#include <tagparser/mediafileinfo.h>
#include <tagparser/diagnostics.h>
#include <tagparser/progressfeedback.h>
#include <tagparser/abstractattachment.h>
#include <tagparser/tag.h>
#include <iostream>

bool logLineFinalized = true;
static string lastStep;

using namespace std;
using namespace TagParser;

void logNextStep(const AbortableProgressFeedback &progress) {
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

void logStepPercentage(const AbortableProgressFeedback &progress) {
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
    MediaFileInfo fileInfo;
    Diagnostics diag;

    try {
        fileInfo.setPath(movieInfo.path);
        fileInfo.open();
        fileInfo.parseEverything(diag);
        auto container = fileInfo.container();

        if (fileInfo.tagsParsingStatus() == ParsingStatus::Ok && container) {
            const auto tags = fileInfo.tags();
            if (tags.empty()) {
                LOG(ERROR) << " - File has no (supported) tag information.\n";
            }
            // iterate through all tags
            for (auto *tag : tags) {
                // add cover from file
                try {
                    // assume the file refers to a picture
                    MediaFileInfo coverFileInfo(cover);
                    Diagnostics coverDiag;
                    coverFileInfo.open(true);
                    coverFileInfo.parseContainerFormat(coverDiag);
                    auto buff = make_unique<char[]>(coverFileInfo.size());
                    coverFileInfo.stream().seekg(static_cast<streamoff>(coverFileInfo.containerOffset()));
                    coverFileInfo.stream().read(buff.get(), static_cast<streamoff>(coverFileInfo.size()));
                    TagValue value(move(buff), coverFileInfo.size(), TagDataType::Picture);
                    value.setMimeType(coverFileInfo.mimeType());
                    if (tag->setValue(KnownField::Cover, value)) {
                        LOG(INFO) << "Cover Tag set";
                    }
                } catch (const TagParser::Failure &) {
                    LOG(ERROR) << "Unable to parse specified cover file." << cover;
                } catch (const std::ios_base::failure &) {
                    LOG(ERROR) << "An IO error occurred when parsing the specified cover file." << cover;
                }
            }

            // Create progress
            AbortableProgressFeedback progress(logNextStep, logStepPercentage);

            // Save changes
            LOG(INFO) << "Saving changes";
            fileInfo.applyChanges(diag, progress);

            // notify about completion
            finalizeLog();
        }
    } catch (const Failure &error) {
        LOG(ERROR) << "A parsing failure occurred when reading the file " << movieInfo.path;
    } catch (const ios_base::failure &) {
        LOG(ERROR) << "An IO failure occurred when reading the file " << movieInfo.path;
    }
}

void save_mkv_cover(const string &cover, const Movie &movieInfo) {
    MediaFileInfo fileInfo;
    Diagnostics diag;

    try {
        fileInfo.setPath(movieInfo.path);
        fileInfo.open();
        fileInfo.parseEverything(diag);
        auto container = fileInfo.container();

        if (fileInfo.attachmentsParsingStatus() == ParsingStatus::Ok && container) {
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
            AbortableProgressFeedback progress(logNextStep, logStepPercentage);

            // Save changes
            LOG(INFO) << "Saving changes";
            fileInfo.applyChanges(diag, progress);

            // notify about completion
            finalizeLog();
        }
    } catch (const Failure &error) {
        LOG(ERROR) << "A parsing failure occurred when reading the file " << movieInfo.path;
    } catch (const ios_base::failure &) {
        LOG(ERROR) << "An IO failure occurred when reading the file " << movieInfo.path;
    }
}
