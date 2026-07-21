#ifndef LOCATION_H
#define LOCATION_H

struct Location
{
    char country[100];
    char country_code[10];
};

int get_location(struct Location *location);

#endif