// MusicFeed (C++ port) — fetches 20 tracks from Apple's public iTunes Search
// API and prints them to the console as an aligned table.
// https://developer.apple.com/library/archive/documentation/AudioVideo/Conceptual/iTuneSearchAPI/

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iomanip>
#include <iostream>
#include <string>

using json = nlohmann::json;

namespace {

// libcurl writes received bytes here, one chunk at a time.
std::size_t WriteToString(char* ptr, std::size_t size, std::size_t nmemb, void* userdata) {
    auto* out = static_cast<std::string*>(userdata);
    out->append(ptr, size * nmemb);
    return size * nmemb;
}

// Perform an HTTPS GET and return the response body. Throws on failure.
std::string HttpGet(const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialise libcurl");
    }

    std::string body;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "MusicFeed/1.0");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);

    const CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("Request failed: ") + curl_easy_strerror(res));
    }
    return body;
}

// Truncate long fields so the table stays aligned.
std::string Trim(const std::string& value, std::size_t max) {
    if (value.size() <= max) return value;
    return value.substr(0, max - 1) + "...";
}

// Safely read a string field that may be missing/null.
std::string Field(const json& obj, const char* key) {
    auto it = obj.find(key);
    if (it == obj.end() || it->is_null()) return "";
    return it->get<std::string>();
}

}  // namespace

int main() {
    constexpr int kWanted = 20;
    const std::string url =
        "https://itunes.apple.com/search?term=jazz&entity=song&limit=" + std::to_string(kWanted);

    std::cout << "Fetching " << kWanted << " tracks from the iTunes Search API...\n\n";

    json data;
    try {
        data = json::parse(HttpGet(url));
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        return 1;
    }

    const auto& results = data["results"];
    if (!results.is_array() || results.empty()) {
        std::cerr << "No data returned from the service.\n";
        return 1;
    }

    std::cout << std::left << std::setw(4) << "#" << std::setw(41) << "Track"
              << std::setw(26) << "Artist" << "Genre\n";
    std::cout << std::string(90, '-') << '\n';

    int line = 0;
    for (const auto& t : results) {
        if (line >= kWanted) break;
        ++line;
        std::cout << std::left << std::setw(4) << line
                  << std::setw(41) << Trim(Field(t, "trackName"), 40)
                  << std::setw(26) << Trim(Field(t, "artistName"), 25)
                  << Field(t, "primaryGenreName") << '\n';
    }

    return 0;
}
