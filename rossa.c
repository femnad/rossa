#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libnotify/notify.h>

#define CRITICAL_PERCENTAGE 0.15
#define MINIMUM_CHARGE 0.05
#define EXECUTABLE_NAME "rossa"
#define NOTIFICATION_SUMMARY "Battery Low"
#define SLEEP_PERIOD 60

typedef struct _battery_status {
    int battery_number;
    double charge_percentage;
} battery_status;

void print_usage() {
    printf("Usage: %s [-do]\n", EXECUTABLE_NAME);
}

char *get_battery_info(int battery_number, char *info_file) {
    char *battery_filename = malloc(sizeof(char) * 256);
    sprintf(battery_filename, "/sys/class/power_supply/BAT%d/%s", battery_number, info_file);
    FILE *battery_file = fopen(battery_filename, "r");
    if (battery_file == NULL) {
        perror(battery_filename);
        exit(1);
    }
    char *info = malloc(sizeof(char) * 256);
    fscanf(battery_file, "%s", info);
    fclose(battery_file);
    return info;
}

int get_energy(int battery_number, char *specifier) {
    char *info_file = malloc(sizeof(char) * 256);
    sprintf(info_file, "energy_%s", specifier);
    char *battery_info = get_battery_info(battery_number, info_file);
    long temp = strtol(battery_info, NULL, 10);
    return (int) temp;
}

char *get_charge_status(int battery_number) {
    return get_battery_info(battery_number, "status");
}

bool is_charging(int battery_number) {
    char *charge_status = get_charge_status(battery_number);
    return strcmp(charge_status, "Charging") == 0;
}

double get_charge_percentage(const int battery_number) {
    int current_charge = get_energy(battery_number, "now");
    int full_charge = get_energy(battery_number, "full");
    double charge_percentage = (double) current_charge / full_charge;
    char *status = get_charge_status(battery_number);
    if (strcmp(status, "Unknown") == 0 && charge_percentage < MINIMUM_CHARGE) {
        return 0.0;
    }
    return charge_percentage;
}

double get_total_percentage(battery_status combined_batteries[], int number_of_batteries) {
    double total_charge = 0.0;
    for (int i = 0; i < number_of_batteries; i++) {
        total_charge += combined_batteries[i].charge_percentage;
    }
    return total_charge;
}

int get_number_of_batteries() {
    DIR *power_supply_dir;
    struct dirent *dp;
    power_supply_dir = opendir("/sys/class/power_supply");
    int number_of_batteries = 0;
    while ((dp = readdir(power_supply_dir)) != NULL) {
        if (strstr(dp->d_name, "BAT") != NULL) {
            number_of_batteries++;
        }
    }
    closedir(power_supply_dir);
    return number_of_batteries;
}

int main(int argc, char* argv[]) {
    bool daemonize = false, one_time = false;
    int opt;
    while ((opt = getopt(argc, argv, "doh")) != -1) {
        switch(opt) {
        case 'd':
            daemonize = true;
            break;
        case 'o':
            one_time = true;
            break;
        case 'h':
        default:
            print_usage();
            exit(1);
        }
    }
    int number_of_batteries = get_number_of_batteries();
    if (daemonize) {
        daemon(true, false);
    }
    notify_init(EXECUTABLE_NAME);
    while (true) {
        battery_status status[number_of_batteries];
        bool any_charging = false;
        for (int i = 0; i < number_of_batteries; i++) {
            battery_status s;
            s.battery_number = i;
            s.charge_percentage = get_charge_percentage(i);
            any_charging |= is_charging(i);
            status[i] = s;
        }
        double total_percentage = get_total_percentage(status, number_of_batteries);
        if (!any_charging && total_percentage < CRITICAL_PERCENTAGE) {
            char *notification_body = malloc(sizeof(char) * 256);
            sprintf(notification_body, "Remaining: %0.f%%", total_percentage * 100);
            NotifyNotification *notification = notify_notification_new
                (NOTIFICATION_SUMMARY, notification_body, NULL);
            notify_notification_show(notification, NULL);
        }
        if (one_time) {
            break;
        } else {
            sleep(SLEEP_PERIOD);
        }
    }
    notify_uninit();
    return 0;
}
