#include <stdio.h>
#include <curl/curl.h>
#include "download.h"

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    (void)contents;
    size_t total_size = size * nmemb;

    size_t *downloaded = (size_t *)userp;
    *downloaded += total_size;

    return total_size;
}

double run_download_test(struct Server *server)
{
    if(server == NULL || server->host[0] == '\0')
    {
        printf("Invalid server\n");
        return -1;
    }

    printf("Download test started\n");
    printf("Using server: %s\n", server->host);

    CURL *curl = curl_easy_init();

    if(!curl)
    {
        printf("Curl initialization failed");
        return -1;
    }
    size_t downloaded = 0;
    char url[256];
    snprintf(url, sizeof(url), "http://%s/speedtest/random4000x4000.jpg", server->host);
    printf("Downloading from: %s\n", url);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &downloaded);
    
    printf("Starting download...\n");

    CURLcode result = curl_easy_perform(curl);

    long http_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (result != CURLE_OK && result != CURLE_OPERATION_TIMEDOUT)
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

    if (result == CURLE_OPERATION_TIMEDOUT)
    {
        printf("Download stopped after 15 seconds\n");
    }

    double total_time;
    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_time);

    double megabits = (downloaded * 8.0) / 1000000.0;

    if(total_time <= 0)
    {
        curl_easy_cleanup(curl);
        return -1;
    }

    double speed = megabits / total_time;
   
    curl_easy_cleanup(curl);

    return speed;
}