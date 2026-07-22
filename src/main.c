#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include "config.h"
#include "download.h"
#include "server.h"
#include "upload.h"
#include "location.h"

// Prints information about available command line options and usage examples.

static void print_help()
{
    printf("\nC Speedtest\n\n");

    printf("Usage:\n");
    printf("  ./speedtest --download --server ID\n");
    printf("  ./speedtest --upload --server ID\n");
    printf("  ./speedtest --auto\n\n");

    printf("Options:\n");
    printf("  -d, --download        Run download speed test\n");
    printf("  -u, --upload          Run upload speed test\n");
    printf("  -s, --server ID       Select server by ID\n");
    printf("  -a, --auto            Run automatic test\n");
    printf("  -h, --help            Show this help message\n\n");

    printf("Examples:\n");
    printf("  ./speedtest --download --server 9714\n");
    printf("  ./speedtest --upload --server 9714\n");
    printf("  ./speedtest --auto\n\n");
}

// Parses command line arguments, selects a server and runs requested speed tests.

int main(int argc, char *argv[])
{
    struct Config config = {0}; // Initialize configuration structure. Values are set to 0, meaning no test option is selected.

    config.server = -1; // Server ID is initialized to -1 because 0 can be a valid input value.

    struct option long_options[] = // Defines available long command line options for getopt_long.
    {
        {"download", no_argument,       0, 'd'},
        {"upload",   no_argument,       0, 'u'},
        {"server",   required_argument, 0, 's'},
        {"auto",     no_argument,       0, 'a'},
        {"help",     no_argument,       0, 'h'},
        {0, 0, 0, 0} // marks the end of the array.
    };

    int option;

    // Read command line arguments until no more options are found.
    while ((option = getopt_long(argc, argv, "dus:ah", long_options, NULL)) != -1)
    {
        switch(option)
        {
            case 'd':
                config.download = 1;
                break;

            case 'u':
                config.upload = 1;
                break;

            case 's': // Convert server ID from string argument to integer.
            {
                char *end;
                config.server = strtol(optarg, &end, 10); // optarg contains the value provided after --server.

                if(*end != '\0')
                {
                    printf("Invalid server ID\n");
                    return 1;
                }
                break;
            }

            case 'a':
                config.auto_test = 1;
                break;

            case 'h':
                print_help();
                return 0;
        }
    }
    
    if (config.auto_test) // Automatic mode runs both download and upload tests.
    {
        config.download = 1;
        config.upload = 1;
    }
    

    if (!config.download && !config.upload && !config.auto_test) // Ensures that the user selected at least one test mode.
    {
        printf("\nError: No test selected.\n\n");

        printf("Choose one option:\n");
        printf("  --download --server ID\n");
        printf("  --upload --server ID\n");
        printf("  --auto\n\n");

        return 1;
    }

    if (config.server == -1 && !config.auto_test) // Manual tests require a server ID. Automatic mode selects the server automatically.
    {
        printf("\nError: Server ID is required for manual tests.\n\n");

        printf("Examples:\n");
        printf("  Download test:\n");
        printf("    ./speedtest --download --server ID\n\n");

        printf("  Upload test:\n");
        printf("    ./speedtest --upload --server ID\n\n");

        printf("  Automatic test:\n");
        printf("    ./speedtest --auto\n\n");

        return 1;
    }

    struct Server server;
    int result = 0;
    // Initialize results with -1 to indicate that the test has not been completed.
    double download_speed = -1;
    double upload_speed = -1;

    if(config.auto_test)
    {
        struct Location location;
        
        printf("\nStarting automatic speed test...\n");

        printf("Detecting location...\n");

        if(get_location(&location)==0)
        {
            printf("User location: %s (%s)\n",
           location.country,
           location.country_code);
        }
        else
        {
            printf("Location detection failed\n");
            printf("Automatic test cannot continue without location\n");
            return 1;
        }

        printf("Finding best server...\n");
        result = find_best_server(&location, &server);
    }
    else
    {
        result = find_server(config.server, &server);
    }

    if (!result) // Server search successful (0 success).
    {
        printf("\nSelected server:\n");
        printf("Country: %s\n", server.country);
        printf("City: %s\n", server.city);
        printf("Provider: %s\n", server.provider);
        printf("Host: %s\n", server.host);

        printf("\nRunning tests...\n\n");

        if(config.download)
        {
            download_speed = run_download_test(&server);
            printf("Download test finished\n\n");
        }

        if(config.upload)
        {
            upload_speed = run_upload_test(&server);
            printf("Upload test finished\n\n");
        }
    }
    else
    {
        return 1;
    }
    
    printf("========== SPEEDTEST RESULT ==========\n");

    printf("Server: %s\n", server.host);
    printf("Country: %s\n", server.country);

    if(download_speed >= 0)
        printf("Download speed: %.2f Mbps\n", download_speed);
    else if(config.download)
        printf("Download test FAILED (check connection or server availability)\n");


    if(upload_speed >= 0)
        printf("Upload speed: %.2f Mbps\n", upload_speed);
    else if(config.upload)
        printf("Upload test FAILED (check connection or server availability)\n");

    printf("======================================\n");

    return 0;
}