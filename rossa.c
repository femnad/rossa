#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BASE_CRITICAL_PERCENTAGE 0.1
#define BATTERY_NUMBER_MULTIPLIER 1.5
#define EXECUTABLE_NAME "rossa"
#define NOTIFICATION_COMMAND "notify-send"
#define NOTIFICATION_SUMMARY "Battery Info:"
#define SLEEP_PERIOD 60

char *trim_new_line(char *a_string) {
    return strtok(a_string, "\n");
}

typedef struct _battery_status {
    int battery_number;
    double charge_percentage;
} battery_status;

void print_usage() {
    printf("Usage: %s [-do]\n", EXECUTABLE_NAME);
}

int get_energy(int battery_number, char *specifier) {
    char *battery_filename = malloc(sizeof(char) * 256);
    sprintf(battery_filename, "/sys/class/power_supply/BAT%d/energy_%s", battery_number, specifier);
    FILE *battery_file = fopen(battery_filename, "r");
    int current_energy = 0;
    fscanf(battery_file, "%d", &current_energy);
    return current_energy;
}

double get_charge_percentage(const int battery_number) {
    int current_charge = get_energy(battery_number, "now");
    int full_charge = get_energy(battery_number, "full");
    double charge_percentage = (double) current_charge / full_charge;
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

double get_critical_percentage(const int number_of_batteries) {
    return number_of_batteries * BASE_CRITICAL_PERCENTAGE * BATTERY_NUMBER_MULTIPLIER;
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
    double critical_percentage = get_critical_percentage(number_of_batteries);
    if (daemonize) {
        daemon(true, false);
    }
    while (true) {
        battery_status status[number_of_batteries];
        for (int i = 0; i < number_of_batteries; i++) {
            battery_status s;
            s.battery_number = i;
            s.charge_percentage = get_charge_percentage(i);
            status[i] = s;
        }
        double total_percentage = get_total_percentage(status, number_of_batteries);
        if (total_percentage < critical_percentage) {
            FILE *fp = popen("acpi", "r");
            char *acpi_output = NULL;
            size_t len;
            int cursor = 0;
            char **lines = malloc(sizeof(char *));
            int notification_len = 0;
            while (getline(&acpi_output, &len, fp) != -1) {
                notification_len += len;
                if (cursor > 0) {
                    notification_len++; // new line char
                }
                *(lines + cursor) = malloc(sizeof(char) * len);
                strcpy(*(lines + cursor), trim_new_line(acpi_output));
                cursor++;
            }
            char *notification_body = malloc(sizeof(char) * notification_len);
            memset(notification_body, '\0', 1);
            for (int i = 0; i < cursor; i++) {
                if (i > 0) {
                    strcat(notification_body, "\n");
                }
                strcat(notification_body, *(lines + i));
            }
            int command_len = strlen(NOTIFICATION_COMMAND) +
                strlen(NOTIFICATION_SUMMARY) + notification_len - 1 + 6; // spaces
            char *command = malloc(sizeof(char) * command_len);
            sprintf(command, "%s '%s' '%s'", NOTIFICATION_COMMAND,
                    NOTIFICATION_SUMMARY, notification_body);
            system(command);
            free(command);
            fclose(fp);
        }
        if (one_time) {
            break;
        } else {
            sleep(SLEEP_PERIOD);
        }
    }
    return 0;
}
