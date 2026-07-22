#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include "server.h"

typedef struct {
    const char *variant;
    const char *canonical; // Matches country names returned by location API
} country_alias;

static const country_alias ALIASES[] = { // Converts different formats from server list to the format returned by API.
    {"Brasil",                                    "Brazil"},
    {"Russian Federation",                        "Russia"},
    {"US",                                        "United States"},
    {"Great Britain",                             "United Kingdom"},
    {"GB",                                        "United Kingdom"},
    {"Netherland",                                "Netherlands"},
    {"Belguim",                                   "Belgium"},
    {"Finnland",                                  "Finland"},
    {"Lithuanua",                                 "Lithuania"},
    {"Phillipines",                               "Philippines"},
    {"Francef",                                   "France"},
    {"Viet Nam",                                  "Vietnam"},
    {"Polska",                                    "Poland"},
    {"Iran, Islamic Republic of",                 "Iran"},
    {"Bolivia, Plurinational State of",           "Bolivia"},
    {"Syrian Arab Republic",                      "Syria"},
    {"Venezuela, Bolivarian Republic of",         "Venezuela"},
    {"Republic of Moldova",                       "Moldova"},
    {"Republic of Maldives",                      "Maldives"},
    {"Republic of Singapore",                     "Singapore"},
    {"Republic of the Union of Myanmar",          "Myanmar"},
    {"Tanzania, United Republic of",               "Tanzania"},
    {"Macedonia, the former Yugoslav Republic of", "Macedonia"},
    {"Palestinian Territory, Occupied",            "Palestine"},
    {"Democratic Republic of the Congo",           "Congo"},
    {"DR Congo",                                   "Congo"},
    {"Lao PDR",                                    "Laos"},
    {"Macao",                                      "Macau"},
    {"Trinidad",                                   "Trinidad and Tobago"},
    {"Abkhaziya",                                  "Georgia"},
    {NULL, NULL}
};

static const char* normalize_country(const char *json_country) // Returns standardized country name.
{
    for(const country_alias *a = ALIASES; a->variant; a++)
    {
        if(strcmp(json_country, a->variant) == 0)
        {
            return a->canonical;
        }
    }

    return json_country; // Used to show the corrected country name to the user, not the one from json
}

static cJSON* load_server_list() // Reads server information from JSON file and parses it using cJSON.
{
    FILE *file = fopen("speedtest_server_list.json", "r");

    if(file == NULL)
    {
        printf("Cannot open server list\n");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char *json_text = malloc(size + 1);

    if(json_text == NULL)
    {
        fclose(file);
        return NULL;
    }

    fread(json_text, 1, size, file);
    json_text[size] = '\0';
    fclose(file);

    cJSON *json = cJSON_Parse(json_text);
    free(json_text);

    if(json == NULL)
    {
        printf("JSON parse error\n");
        return NULL;
    }

    return json;
}

int country_matches(const char *json_country, const struct Location *location) // Checks if server country matches user's country.
{
    if(strcmp(json_country, location->country) == 0)
        return 1;

    if(strcmp(json_country, location->country_code) == 0)
        return 1;

    for (const country_alias *a = ALIASES; a->variant; a++)
    {
        if (strcmp(json_country, a->variant) == 0 && strcmp(location->country, a->canonical) == 0)
            return 1;
    }

    return 0;
}

// Sends a small request to the server to measure response time.
// Used for selecting the fastest available server in user's country.
double check_server_response_time(const char *host)
{
    CURL *curl = curl_easy_init();

    if(!curl)
    {
        return -1;
    }

    char url[256];

    snprintf(url, sizeof(url), "http://%s/speedtest/random200x200.jpg", host);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode result = curl_easy_perform(curl);

    double time = -1;

    if(result == CURLE_OK)
    {
        curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &time);
    }

    curl_easy_cleanup(curl);
    return time;
}

int find_server(int id, struct Server *server) // Finds a server from JSON list by its ID.
{
    cJSON *json = load_server_list();

    if(json == NULL)
    {
        return -1;
    }

    int count = cJSON_GetArraySize(json);

    for (int i = 0; i < count; i++)
    {
        cJSON *item = cJSON_GetArrayItem(json, i);
        cJSON *id_item = cJSON_GetObjectItem(item, "id");

        if (id_item && id_item->valueint == id)
        {

            cJSON *country = cJSON_GetObjectItem(item, "country");
            cJSON *city = cJSON_GetObjectItem(item, "city");
            cJSON *provider = cJSON_GetObjectItem(item, "provider");
            cJSON *host = cJSON_GetObjectItem(item, "host");

            if(!country || !city || !provider || !host)
            {
                continue;
            }

            strncpy(server->country, normalize_country(country->valuestring), sizeof(server->country)-1);
            server->country[sizeof(server->country)-1] = '\0';

            strncpy(server->city, city->valuestring, sizeof(server->city)-1);
            server->city[sizeof(server->city)-1] = '\0';

            strncpy(server->provider, provider->valuestring, sizeof(server->provider)-1);
            server->provider[sizeof(server->provider)-1] = '\0';

            strncpy(server->host, host->valuestring, sizeof(server->host)-1);
            server->host[sizeof(server->host)-1] = '\0';

            server->id = id;

            cJSON_Delete(json);
            return 0;
        }
    }
    cJSON_Delete(json);
    printf("No server found with ID: %d\n", id);
    return 1;
}

// Searches for available servers in user's country
// and selects the one with the lowest response time.
int find_best_server(struct Location *location, struct Server *server)
{
    cJSON *json = load_server_list();

    if(json == NULL)
    {
        return -1;
    }
    
    int count = cJSON_GetArraySize(json);

    double best_time = 999999;
    int found = 0;

    for(int i = 0; i < count; i++)
    {
        cJSON *item = cJSON_GetArrayItem(json, i);

        cJSON *country_item = cJSON_GetObjectItem(item, "country");

        if(country_item && country_matches(country_item->valuestring, location))
        {

            cJSON *host = cJSON_GetObjectItem(item,"host");

            if(!host)
            {
                continue;
            }

            printf("Checking server: %s\n", host->valuestring);

            double response_time = check_server_response_time(host->valuestring);

            if(response_time >= 0)
            {
                printf("Response time: %.3f seconds\n", response_time);
            }
            else
            {
                printf("Server unavailable\n");
            }

            if(response_time >= 0 && response_time < best_time)
            {
                best_time = response_time;
                found = 1;


                cJSON *id = cJSON_GetObjectItem(item,"id");
                cJSON *city = cJSON_GetObjectItem(item,"city");
                cJSON *provider = cJSON_GetObjectItem(item,"provider");

                if(!id || !city || !provider)
                {
                    continue;
                }

                server->id = id->valueint;

                strncpy(server->country, normalize_country(country_item->valuestring), sizeof(server->country)-1);
                server->country[sizeof(server->country)-1] = '\0';

                strncpy(server->city, city->valuestring, sizeof(server->city)-1);
                server->city[sizeof(server->city)-1] = '\0';

                strncpy(server->provider, provider->valuestring, sizeof(server->provider)-1);
                server->provider[sizeof(server->provider)-1] = '\0';

                strncpy(server->host, host->valuestring, sizeof(server->host)-1);
                server->host[sizeof(server->host)-1] = '\0';
            }
        }
    }

    cJSON_Delete(json);

    if(found)
    {
        return 0;
    }

    printf("No server found for country: %s (%s)\n", location->country, location->country_code);

    return 1;
}