#ifndef TECH_H
#define TECH_H

typedef enum {
    TECH_NONE = -1,
    TECH_AGRICULTURE,
    TECH_MINING,
    TECH_FORESTRY,
    TECH_ARCHERY,
    TECH_CONSTRUCTION,
    TECH_HORSEBACK_RIDING,
    TECH_STEALTH_TACTICS,
    TECH_SAILING,
    TECH_SHIPBUILDING,
    TECH_NAVIGATION,
    TECH_WRITING,
    TECH_EDUCATION,
    TECH_ENGINEERING,
    TECH_ADMINISTRATION,
    TECH_COUNT
} TechType;

typedef enum {
    CITY_SCIENCE_STUDY_HALL,
    CITY_SCIENCE_CAMPUS,
    CITY_SCIENCE_ACADEMY,
    CITY_SCIENCE_OBSERVATORY,
    CITY_SCIENCE_COUNT
} CityScienceBuilding;

const char *tech_type_to_string(TechType tech);
int tech_type_from_string(const char *text, TechType *tech);
int tech_get_cost(TechType tech);
const char *tech_get_description(TechType tech);
int tech_get_prerequisites(TechType tech, TechType prerequisites[], int max_prerequisites);

const char *city_science_building_to_string(CityScienceBuilding building);
int city_science_building_from_string(const char *text, CityScienceBuilding *building);
int city_science_building_get_cost(CityScienceBuilding building);
int city_science_building_get_science(CityScienceBuilding building);
TechType city_science_building_required_tech(CityScienceBuilding building);

#endif
