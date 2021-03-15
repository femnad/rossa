#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libnotify/notify.h>
#include <libupower-glib/upower.h>

#include "config.h"
#include "systemd_action.h"
#include "rossa.h"
#include "version.h"

void
print_version()
{
    printf("%s\n", VERSION);
}

void
print_usage()
{
    printf("Usage: %s [-do]\n", EXECUTABLE_NAME);
}

char*
get_battery_info(int battery_number, char *info_file)
{
    char *battery_filename = malloc(sizeof(char) * 256);
    sprintf(battery_filename, "%s/BAT%d/%s", POWER_SUPPLY_DIR, battery_number, info_file);
    FILE *battery_file = fopen(battery_filename, "r");
    if (battery_file == NULL) {
        perror(battery_filename);
        exit(1);
    }
    free(battery_filename);
    char *info = malloc(sizeof(char) * 256);
    fscanf(battery_file, "%s", info);
    fclose(battery_file);
    return info;
}

bool
is_status(int battery_number, char *expected_status)
{
    char *charge_status = get_charge_status(battery_number);
    return strcmp(charge_status, expected_status) == 0;
}

bool is_charging(int battery_number) {
    return is_status(battery_number, "Charging");
}

bool is_full(int battery_number) {
    return is_status(battery_number, "Full");
}

bool is_full_or_almost_full(int battery_number) {
    return is_full(battery_number) ||
        (get_charge_percentage(battery_number) >= OUGHT_TO_BE_ENOUGH);
}

char *get_charge_status(int battery_number) {
    return get_battery_info(battery_number, "status");
}

int get_energy(int battery_number, char *specifier) {
    char *info_file = malloc(sizeof(char) * 256);
    sprintf(info_file, "%s_%s", SYS_CLASS_WORD, specifier);
    char *battery_info = get_battery_info(battery_number, info_file);
    long temp = strtol(battery_info, NULL, 10);
    free(info_file);
    return (int) temp;
}

double get_charge_percentage(const int battery_number) {
    int current_charge = get_energy(battery_number, "now");
    int full_charge = get_energy(battery_number, "full");
    double charge_percentage = (double) current_charge / full_charge;
    char *status = get_charge_status(battery_number);
    if (strcmp(status, "Unknown") == 0 && charge_percentage < MINIMUM_CHARGE) {
        return 0.0;
    }
    free(status);
    return charge_percentage;
}

double get_total_percentage(int number_of_batteries) {
    double total_charge = 0.0;

    for (int i = 0; i < number_of_batteries; i++) {
        total_charge += get_charge_percentage(i);
    }

    return total_charge;
}

overall_status get_overall_status(int number_of_batteries) {
    bool any_charging = false;
    bool all_full_enough = true;

    double total_percentage = 0;
    for (int i = 0; i < number_of_batteries; i++) {
        total_percentage += get_charge_percentage(i);
        any_charging |= is_charging(i);
        all_full_enough &= is_full_or_almost_full(i);
    }

    if (!any_charging)
    {
        if (total_percentage < CRITICAL_PERCENTAGE)
        {
            return CRITICAL;
        }
        else if (total_percentage < LOW_PERCENTAGE)
        {
            return LOW_BATTERY;
        }
    }

    if (all_full_enough)
    {
        return TOO_MUCH;
    }

    return ROLLING_ALONG;
}

int get_number_of_batteries() {
    DIR *power_supply_dir;
    struct dirent *dp;
    power_supply_dir = opendir(POWER_SUPPLY_DIR);
    int number_of_batteries = 0;
    while ((dp = readdir(power_supply_dir)) != NULL) {
        if (strstr(dp->d_name, "BAT") != NULL) {
            number_of_batteries++;
        }
    }
    closedir(power_supply_dir);
    return number_of_batteries;
}

void show_remaining_battery_percentage(int number_of_batteries, char *summary,
        bool show_remaning) {
    char *notification_body = malloc(sizeof(char) * 256);

    double total_percentage = get_total_percentage(number_of_batteries);
    if (show_remaning) {
        sprintf(notification_body, "Remaining: %0.f%%", total_percentage * 100);
    } else {
        sprintf(notification_body, "Unplug or whatever");
    }

    NotifyNotification *notification = notify_notification_new
        (summary, notification_body, NULL);
    GError *err = NULL;
    bool success = notify_notification_show(notification, &err);
    if (!success) {
        fprintf(stderr, "Error on notification: %s\n", err->message);
        g_error_free(err);
    }
    free(notification_body);
}

void
up_tool_device_changed_cb (UpDevice *device, GParamSpec *pspec, gpointer user_data)
{
    printf("device changed\n");
}

void
do_monitor()
{
    GError *error = NULL;
    GMainLoop *loop;
    GPtrArray *devices;
    UpClient *client;

    client = up_client_new_full(NULL, &error);
    if (client == NULL) {
        fprintf(stderr, "Unable to connect to upowerd: %s", error->message);
        g_error_free(error);
        exit(1);
    }

    devices = up_client_get_devices2 (client);
    for (guint i=0; i < devices->len; i++) {
        UpDevice *device;
        device = g_ptr_array_index (devices, i);
        g_signal_connect (device, "notify", G_CALLBACK (up_tool_device_changed_cb), NULL);
    }

    loop = g_main_loop_new (NULL, FALSE);
    printf("main loop\n");
    g_main_loop_run (loop);
    printf("main loop end\n");
}

int main(int argc, char* argv[]) {
    bool daemonize = false, one_time = false;
    int opt;
    while ((opt = getopt(argc, argv, "dohv")) != -1) {
        switch(opt) {
        case 'd':
            daemonize = true;
            break;
        case 'o':
            one_time = true;
            break;
        case 'v':
            print_version();
            exit(0);
        case 'h':
        default:
            print_usage();
            exit(1);
        }
    }
    if (daemonize) {
        daemon(true, false);
    }

    if (one_time) {
        printf("one time");
    }
    notify_init(EXECUTABLE_NAME);

    do_monitor();

    notify_uninit();
    return 0;
}
