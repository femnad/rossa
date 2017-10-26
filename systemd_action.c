#include <stdio.h>
#include <unistd.h>

#define SYSTEMCTL_EXECUTABLE "systemctl"
#define SYSTEMCTL_EXECUTABLE_PATH "/bin/systemctl"

#define HIBERNATE_COMMAND "hibernate"
#define SUSPEND_COMMAND "suspend"

void
systemd_action(char *action)
{
    char *const exec_argv[] = { SYSTEMCTL_EXECUTABLE, action, NULL };
    char *const exec_env[] = { NULL };
    int rc = execve(SYSTEMCTL_EXECUTABLE_PATH, exec_argv, exec_env);
    if (rc == -1)
    {
        perror("Error in systemctl command");
    }
}

void
hibernate()
{
    systemd_action(HIBERNATE_COMMAND);
}

void
suspend()
{
    systemd_action(SUSPEND_COMMAND);
}
