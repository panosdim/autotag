#include "TMDB.h"
#include "restclient-cpp/restclient.h"
#include <nlohmann/json.hpp>
#include <glog/logging.h>
#include <curl/curl.h>

using json = nlohmann::json;

TMDB::TMDB(const string &token) {
    apiKey += token;
    RestClient::Response r = RestClient::get(baseUrl + "configuration" + apiKey);
    if (r.code == 200) {
        auto response = json::parse(r.body);
        string posterSize;
        for (auto &element : response["images"]["poster_sizes"]) {
            if (posterSize.empty()) {
                posterSize = element;
            }
            if (element == "w500") {
                posterSize = element;
                break;
            }
        }
        string secureBaseUrl = response["images"]["secure_base_url"];
        imageBaseUrl = secureBaseUrl + posterSize;
    } else {
        PLOG(ERROR) << "Can't retrieve configuration from TMDB." << r.code;
    }
}

FILE *TMDB::downloadCover(const MovieInfo &movieInfo) {
    CURL *curl;
    CURLcode res;
    string query = "&query=";
    curl = curl_easy_init();
    if (curl) {
        query += curl_easy_escape(curl, movieInfo.title.c_str(), movieInfo.title.length());
    } else {
        PLOG(ERROR) << "Can't initialize cURL";
    }

    string year = "&year=" + movieInfo.releaseYear;
    RestClient::Response r = RestClient::get(baseUrl + "search/movie" + apiKey + query + year);
    if (r.code == 200) {
        auto response = json::parse(r.body);
        if (response["total_results"] >= 1) {
            string posterPath = response["results"][0]["poster_path"];
            string imageUrl = imageBaseUrl + posterPath;

            FILE *fp = fopen("/tmp/cover.jpg", "wb");
            if (!fp) {
                PLOG(ERROR) << "Can't open file to save cover image.";
            }

            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
            curl_easy_setopt(curl, CURLOPT_URL, imageUrl.c_str());
            // Grab image
            res = curl_easy_perform(curl);
            if (res)
                PLOG(ERROR) << "Cannot grab the image!";

            // Close the file
            fclose(fp);

            // Clean up the resources
            curl_easy_cleanup(curl);

            return fp;
        } else {
            // Clean up the resources
            curl_easy_cleanup(curl);
            PLOG(ERROR) << "No movie found in TMDB.";
        }
    } else {
        // Clean up the resources
        curl_easy_cleanup(curl);
        PLOG(ERROR) << "Can't search movie in TMDB." << r.code;
    }

    // Clean up the resources
    curl_easy_cleanup(curl);
    return nullptr;
}
