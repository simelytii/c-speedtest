#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include "location.h"

struct Memory // Used to store data received from HTTP response.
{
    char *data;
    size_t size;
};

// Dynamically stores received API response data in memory.
static size_t location_write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t total = size * nmemb;

    struct Memory *mem = userp;

    char *ptr = realloc(mem->data, mem->size + total + 1); // Increase allocated memory size to fit newly received data.

    if(ptr == NULL)
    {
        return 0;
    }

    mem->data = ptr;

    memcpy(&(mem->data[mem->size]),
           contents,
           total);

    mem->size += total;

    mem->data[mem->size] = '\0';

    return total;
}

// Detects user's location using external API.
// Stores country information in Location structure.
int get_location(struct Location *location)
{
    if(location == NULL)
    {
        return -1;
    }
    
    CURL *curl = curl_easy_init();

    if(!curl)
    {
        return -1;
    }

    struct Memory response;

    response.data = malloc(1); // Allocate initial memory block for response data.

    if(response.data == NULL)
    {
        curl_easy_cleanup(curl);
        return -1;
    }

    response.size = 0;

    curl_easy_setopt(curl, CURLOPT_URL, "http://ip-api.com/json/");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, location_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode result = curl_easy_perform(curl);

    if(result != CURLE_OK)
    {
        curl_easy_cleanup(curl);
        free(response.data);
        return -1;
    }

    cJSON *json = cJSON_Parse(response.data); // Parse API response from JSON format.

    if(json == NULL)
    {
        curl_easy_cleanup(curl);
        free(response.data);
        return -1;
    }

    cJSON *status_item = cJSON_GetObjectItem(json, "status");

    if(!status_item || strcmp(status_item->valuestring, "success") != 0)
    {
        cJSON_Delete(json);
        free(response.data);
        curl_easy_cleanup(curl);
        return -1;
    }

    cJSON *country_item = cJSON_GetObjectItem(json, "country");

    cJSON *country_code_item = cJSON_GetObjectItem(json, "countryCode");

    if(country_item && country_item->valuestring)
    {
        strncpy(location->country, country_item->valuestring, sizeof(location->country)-1);
        location->country[sizeof(location->country)-1] = '\0'; // Copy API data safely into fixed-size structure fields.
    }
    else
    {
        strcpy(location->country, "Unknown");
    }

    if(country_code_item && country_code_item->valuestring)
    {
        strncpy(location->country_code, country_code_item->valuestring, sizeof(location->country_code)-1);
        location->country_code[sizeof(location->country_code)-1] = '\0'; // Copy API data safely into fixed-size structure fields.
    }
    else
    {
        location->country_code[0] = '\0';
    }

    cJSON_Delete(json);
    free(response.data);
    curl_easy_cleanup(curl);

    return 0;
}