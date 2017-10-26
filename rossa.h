#define POWER_SUPPLY_DIR "/sys/class/power_supply"

typedef struct _battery_status
{
    int battery_number;
    double charge_percentage;
} battery_status;

typedef enum _overall_status
{
    LOW_BATTERY, CRITICAL, ROLLING_ALONG, TOO_MUCH
} overall_status;

char *get_charge_status(int);

double get_charge_percentage(const int);
