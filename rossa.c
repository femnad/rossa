#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NOTIFICATION_COMMAND "notify-send"
#define NOTIFICATION_SUMMARY "Battery Info:"
#define SLEEP_PERIOD 600

char *trim_new_line(char *a_string) {
    return strtok(a_string, "\n");
}

int main(int argc, char* argv[]) {
    while (1) {
        daemon(1, 0);
        sleep(SLEEP_PERIOD);
        FILE *fp = popen("acpi", "r");
        char *acpi_output = NULL;
        size_t len;
        getline(&acpi_output, &len, fp);
        int command_len = strlen(NOTIFICATION_COMMAND) +
            strlen(NOTIFICATION_SUMMARY) + strlen(acpi_output) - 1 + 6;
        char *command = malloc(sizeof(char) * command_len);
        sprintf(command, "%s '%s' '%s'", NOTIFICATION_COMMAND,
                NOTIFICATION_SUMMARY, trim_new_line(acpi_output));
        system(command);
        free(command);
        fclose(fp);
    }
    return 0;
}
