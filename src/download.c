#include <stdio.h>
#include <curl/curl.h>
#include "download.h"


// Counts received bytes during download.
// Downloaded data is ignored because only transfer size is needed.
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    (void)contents;
    size_t total_size = size * nmemb;

    size_t *downloaded = (size_t *)userp;
    *downloaded += total_size;

    return total_size;
}

// Runs download speed test using selected server.
// Returns download speed in Mbps or -1 if test fails.
double run_download_test(struct Server *server)
{
    if(server == NULL || server->host[0] == '\0')
    {
        printf("Invalid server\n");
        return -1;
    }

    printf("Download test started\n");
    printf("Using server: %s\n", server->host);

    CURL *curl = curl_easy_init(); // Initialize libcurl handle used for HTTP request.

    if(!curl)
    {
        printf("Curl initialization failed");
        return -1;
    }
    size_t downloaded = 0; // Stores total amount of received data in bytes.
    char url[512];
    snprintf(url, sizeof(url), "http://%s/speedtest/random4000x4000.jpg", server->host);
    printf("Downloading from: %s\n", url);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L); // limit execution time to 15 seconds
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback); // use callback to count downloaded bytes
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &downloaded);
    
    printf("Starting download...\n");

    CURLcode result = curl_easy_perform(curl); // Start HTTP download request.

    long http_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if(result == CURLE_OPERATION_TIMEDOUT)
    {
        printf("Upload stopped after 15 seconds\n");
    }
    else if(result != CURLE_OK)
    {
        printf("Curl error: %s\n", curl_easy_strerror(result));

        curl_easy_cleanup(curl);
        return -1;
    }

    if (http_code != 200)
    {
        printf("HTTP error: %ld\n", http_code);

        curl_easy_cleanup(curl);
        return -1;
    }

    double total_time;
    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_time);

    if(total_time <= 0)
    {
        curl_easy_cleanup(curl);
        return -1;
    }

    if(downloaded == 0)
    {
        printf("No data downloaded\n");
        curl_easy_cleanup(curl);
        return -1;
    }

    double megabits = (downloaded * 8.0) / 1000000.0; // Convert downloaded bytes to megabits.
    double speed = megabits / total_time;
   
    curl_easy_cleanup(curl);

    return speed;
}