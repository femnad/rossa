#include <unistd.h>

#define SYSTEMCTL_EXECUTABLE "systemctl"
#define SYSTEMCTL_EXECUTABLE_PATH "/bin/systemctl"

#define HIBERNATE_COMMAND "hibernate"

void
systemd_action(char *action)
{
    char *const exec_argv[] = { SYSTEMCTL_EXECUTABLE, action, NULL };
    char *const exec_env[] = { NULL };
    execve(SYSTEMCTL_EXECUTABLE, exec_argv, exec_env);
}

void hibernate()
{
    systemd_action(HIBERNATE_COMMAND);
}
