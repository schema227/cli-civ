#include "tech.h"

#include <string.h>

const char *tech_type_to_string(TechType tech)
{
    switch (tech) {
        case TECH_AGRICULTURE:
            return "Agriculture";
        case TECH_MINING:
            return "Mining";
        case TECH_FORESTRY:
            return "Forestry";
        case TECH_ARCHERY:
            return "Archery";
        case TECH_CONSTRUCTION:
            return "Construction";
        case TECH_HORSEBACK_RIDING:
            return "Horseback Riding";
        case TECH_STEALTH_TACTICS:
            return "Stealth Tactics";
        case TECH_SAILING:
            return "Sailing";
        case TECH_SHIPBUILDING:
            return "Shipbuilding";
        case TECH_NAVIGATION:
            return "Navigation";
        case TECH_WRITING:
            return "Writing";
        case TECH_EDUCATION:
            return "Education";
        case TECH_ENGINEERING:
            return "Engineering";
        case TECH_ADMINISTRATION:
            return "Administration";
        case TECH_NONE:
            return "None";
        case TECH_COUNT:
            break;
    }

    return "Unknown";
}

int tech_type_from_string(const char *text, TechType *tech)
{
    if (text == NULL || tech == NULL) {
        return 0;
    }

    if (strcmp(text, "agriculture") == 0) {
        *tech = TECH_AGRICULTURE;
    } else if (strcmp(text, "mining") == 0) {
        *tech = TECH_MINING;
    } else if (strcmp(text, "forestry") == 0) {
        *tech = TECH_FORESTRY;
    } else if (strcmp(text, "archery") == 0) {
        *tech = TECH_ARCHERY;
    } else if (strcmp(text, "construction") == 0) {
        *tech = TECH_CONSTRUCTION;
    } else if (strcmp(text, "horsebackriding") == 0 || strcmp(text, "horseback_riding") == 0 || strcmp(text, "horseback-riding") == 0) {
        *tech = TECH_HORSEBACK_RIDING;
    } else if (strcmp(text, "stealthtactics") == 0 || strcmp(text, "stealth_tactics") == 0 || strcmp(text, "stealth-tactics") == 0) {
        *tech = TECH_STEALTH_TACTICS;
    } else if (strcmp(text, "sailing") == 0) {
        *tech = TECH_SAILING;
    } else if (strcmp(text, "shipbuilding") == 0) {
        *tech = TECH_SHIPBUILDING;
    } else if (strcmp(text, "navigation") == 0) {
        *tech = TECH_NAVIGATION;
    } else if (strcmp(text, "writing") == 0) {
        *tech = TECH_WRITING;
    } else if (strcmp(text, "education") == 0) {
        *tech = TECH_EDUCATION;
    } else if (strcmp(text, "engineering") == 0) {
        *tech = TECH_ENGINEERING;
    } else if (strcmp(text, "administration") == 0) {
        *tech = TECH_ADMINISTRATION;
    } else {
        return 0;
    }

    return 1;
}

int tech_get_cost(TechType tech)
{
    switch (tech) {
        case TECH_AGRICULTURE:
            return 0;
        case TECH_MINING:
            return 15;
        case TECH_FORESTRY:
            return 12;
        case TECH_ARCHERY:
            return 12;
        case TECH_SAILING:
            return 14;
        case TECH_WRITING:
            return 10;
        case TECH_CONSTRUCTION:
            return 25;
        case TECH_HORSEBACK_RIDING:
            return 22;
        case TECH_SHIPBUILDING:
            return 24;
        case TECH_EDUCATION:
            return 24;
        case TECH_STEALTH_TACTICS:
            return 35;
        case TECH_NAVIGATION:
            return 38;
        case TECH_ENGINEERING:
            return 40;
        case TECH_ADMINISTRATION:
            return 28;
        case TECH_NONE:
        case TECH_COUNT:
            return 0;
    }

    return 0;
}

const char *tech_get_description(TechType tech)
{
    switch (tech) {
        case TECH_AGRICULTURE:
            return "Unlocks Farm, Warrior, Settler.";
        case TECH_MINING:
            return "Unlocks Mine, Defender, City Walls.";
        case TECH_FORESTRY:
            return "Unlocks Sawmill and Lumber Camp.";
        case TECH_ARCHERY:
            return "Unlocks Archer.";
        case TECH_CONSTRUCTION:
            return "Unlocks Catapult.";
        case TECH_HORSEBACK_RIDING:
            return "Unlocks Knight.";
        case TECH_STEALTH_TACTICS:
            return "Unlocks Assassin.";
        case TECH_SAILING:
            return "Unlocks Port and Sloop.";
        case TECH_SHIPBUILDING:
            return "Unlocks Brig.";
        case TECH_NAVIGATION:
            return "Unlocks Galleon and Observatory.";
        case TECH_WRITING:
            return "Unlocks Study Hall.";
        case TECH_EDUCATION:
            return "Unlocks Campus.";
        case TECH_ENGINEERING:
            return "Unlocks Academy.";
        case TECH_ADMINISTRATION:
            return "Adds one science building slot in every city.";
        case TECH_NONE:
        case TECH_COUNT:
            return "";
    }

    return "";
}

int tech_get_prerequisites(TechType tech, TechType prerequisites[], int max_prerequisites)
{
    int count = 0;

#define ADD_PREREQ(value) do { if (count < max_prerequisites) { prerequisites[count] = (value); } count++; } while (0)

    switch (tech) {
        case TECH_MINING:
        case TECH_FORESTRY:
        case TECH_ARCHERY:
        case TECH_HORSEBACK_RIDING:
        case TECH_SAILING:
        case TECH_WRITING:
            ADD_PREREQ(TECH_AGRICULTURE);
            break;
        case TECH_CONSTRUCTION:
            ADD_PREREQ(TECH_MINING);
            break;
        case TECH_STEALTH_TACTICS:
            ADD_PREREQ(TECH_ARCHERY);
            ADD_PREREQ(TECH_HORSEBACK_RIDING);
            break;
        case TECH_SHIPBUILDING:
            ADD_PREREQ(TECH_SAILING);
            break;
        case TECH_NAVIGATION:
            ADD_PREREQ(TECH_SHIPBUILDING);
            break;
        case TECH_EDUCATION:
            ADD_PREREQ(TECH_WRITING);
            break;
        case TECH_ENGINEERING:
            ADD_PREREQ(TECH_CONSTRUCTION);
            ADD_PREREQ(TECH_EDUCATION);
            break;
        case TECH_ADMINISTRATION:
            ADD_PREREQ(TECH_WRITING);
            ADD_PREREQ(TECH_MINING);
            break;
        case TECH_AGRICULTURE:
        case TECH_NONE:
        case TECH_COUNT:
            break;
    }

#undef ADD_PREREQ

    return count;
}

const char *city_science_building_to_string(CityScienceBuilding building)
{
    switch (building) {
        case CITY_SCIENCE_STUDY_HALL:
            return "Study Hall";
        case CITY_SCIENCE_CAMPUS:
            return "Campus";
        case CITY_SCIENCE_ACADEMY:
            return "Academy";
        case CITY_SCIENCE_OBSERVATORY:
            return "Observatory";
        case CITY_SCIENCE_COUNT:
            break;
    }

    return "Unknown";
}

int city_science_building_from_string(const char *text, CityScienceBuilding *building)
{
    if (text == NULL || building == NULL) {
        return 0;
    }

    if (strcmp(text, "studyhall") == 0 || strcmp(text, "study_hall") == 0 || strcmp(text, "study-hall") == 0) {
        *building = CITY_SCIENCE_STUDY_HALL;
    } else if (strcmp(text, "campus") == 0) {
        *building = CITY_SCIENCE_CAMPUS;
    } else if (strcmp(text, "academy") == 0) {
        *building = CITY_SCIENCE_ACADEMY;
    } else if (strcmp(text, "observatory") == 0) {
        *building = CITY_SCIENCE_OBSERVATORY;
    } else {
        return 0;
    }

    return 1;
}

int city_science_building_get_cost(CityScienceBuilding building)
{
    switch (building) {
        case CITY_SCIENCE_STUDY_HALL:
            return 8;
        case CITY_SCIENCE_CAMPUS:
            return 15;
        case CITY_SCIENCE_ACADEMY:
            return 22;
        case CITY_SCIENCE_OBSERVATORY:
            return 20;
        case CITY_SCIENCE_COUNT:
            return 0;
    }

    return 0;
}

int city_science_building_get_science(CityScienceBuilding building)
{
    switch (building) {
        case CITY_SCIENCE_STUDY_HALL:
            return 2;
        case CITY_SCIENCE_CAMPUS:
            return 4;
        case CITY_SCIENCE_ACADEMY:
            return 6;
        case CITY_SCIENCE_OBSERVATORY:
            return 5;
        case CITY_SCIENCE_COUNT:
            return 0;
    }

    return 0;
}

TechType city_science_building_required_tech(CityScienceBuilding building)
{
    switch (building) {
        case CITY_SCIENCE_STUDY_HALL:
            return TECH_WRITING;
        case CITY_SCIENCE_CAMPUS:
            return TECH_EDUCATION;
        case CITY_SCIENCE_ACADEMY:
            return TECH_ENGINEERING;
        case CITY_SCIENCE_OBSERVATORY:
            return TECH_NAVIGATION;
        case CITY_SCIENCE_COUNT:
            return TECH_NONE;
    }

    return TECH_NONE;
}
