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
    return 0;
}
