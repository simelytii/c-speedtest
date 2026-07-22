#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include "upload.h"

// Generates upload data and counts the amount of sent bytes.
static size_t read_callback(char *buffer, size_t size, size_t nmemb, void *userp)
{
    size_t total_size = size * nmemb;
    size_t *uploaded = (size_t *)userp;
    memset(buffer, 'A', total_size); // Fill buffer with random data for upload test.
    *uploaded += total_size;
    return total_size;
}

// Ignores server response because only upload speed is needed for output
static size_t ignore_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    (void)contents;
    (void)userp;

    return size * nmemb;
}

// Runs upload speed test using selected server.
// Returns upload speed in Mbps or -1 if test fails.
double run_upload_test(struct Server *server)
{
    if(server == NULL || server->host[0] == '\0')
    {
        printf("Invalid server\n");
        return -1;
    }

    printf("Upload test started\n");
    printf("Using server: %s\n", server->host);

    CURL *curl = curl_easy_init();

    if (!curl)
    {
        return -1;
    }

    size_t uploaded = 0; // Stores total amount of uploaded data in bytes.

    char url[256];

    snprintf(url, sizeof(url), "http://%s/speedtest/upload.php", server->host);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, NULL);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
    curl_easy_setopt(curl, CURLOPT_READDATA, &uploaded);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)(100 * 1024 * 1024)); // Defines upload data size limit (100 MB)
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L); // limit execution time to 15 seconds
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ignore_callback);

    printf("Starting upload...\n");

    CURLcode result = curl_easy_perform(curl);
    
    long http_code;

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if(result == CURLE_OPERATION_TIMEDOUT)
    {
        printf("Upload stopped after 15 seconds\n");
    }
    else if(result != CURLE_OK)
    {
        printf("Curl error: %s\n",
            curl_easy_strerror(result));

            curl_easy_cleanup(curl);
            return -1;
    }

    if(result != CURLE_OPERATION_TIMEDOUT)
    {
        if(http_code != 200)
        {
            printf("HTTP error: %ld\n", http_code);

            curl_easy_cleanup(curl);
            return -1;
        }
    }

    double total_time;

    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_time);

    double megabits = (uploaded * 8.0) / 1000000.0; // Convert uploaded bytes to megabits.

    if(total_time <= 0)
    {
        curl_easy_cleanup(curl);
        return -1;
    }

    double speed = megabits / total_time;

    curl_easy_cleanup(curl);

    return speed;
}