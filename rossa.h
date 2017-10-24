#define POWER_SUPPLY_DIR "/sys/class/power_supply"

char *get_charge_status(int);

double get_charge_percentage(const int);
