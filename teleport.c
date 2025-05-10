#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#define MAX_CMD_LEN 256

typedef struct {
    int year;
    const char* commands[20];
    int cmd_count;
    const char* shells[3];
    int shell_count;
} UnixEra;

UnixEra eras[] = {
    {1971, {"ls", "echo", "pwd"}, 3, {"sh"}, 1},
    {1973, {"ls", "echo", "pwd", "cat"}, 4, {"sh"}, 1},
    {1989, {"ls", "echo", "pwd", "cat", "grep"}, 5, {"sh", "bash", "zsh"}, 3},
    {1990, {"ls", "echo", "pwd", "cat", "grep", "mkdir", "rm", "touch"}, 8, {"sh", "bash", "zsh"}, 3},
    {2023, {"ls", "echo", "pwd", "cat", "grep", "mkdir", "rm", "touch", "curl", "wget", "nano", "top"}, 12, {"sh", "bash", "zsh"}, 3}
};

UnixEra* get_closest_era(int year) {
    if (year < 1971) return NULL;
    int closest = 0;
    for (int i = 1; i < sizeof(eras)/sizeof(eras[0]); i++) {
        if (abs(year - eras[i].year) < abs(year - eras[closest].year))
            closest = i;
    }
    return &eras[closest];
}

int is_option_supported(const char* cmd, const char* opt, int year) {
    if (strcmp(cmd, "ls") == 0) {
        if (year >= 1973 && (!strcmp(opt, "-a") || !strcmp(opt, "-l"))) return 1;
        if (year >= 1989 && (!strcmp(opt, "-R") || !strcmp(opt, "-t") || !strcmp(opt, "-r"))) return 1;
        if (year >= 1990 && !strcmp(opt, "--color")) return 1;
        return 0;
    }
    if (strcmp(cmd, "cat") == 0) {
        if (year >= 1989 && (!strcmp(opt, "-n") || !strcmp(opt, "-v"))) return 1;
        if (year >= 1990 && (!strcmp(opt, "-E") || !strcmp(opt, "-T"))) return 1;
        return 0;
    }
    return 1;
}

void run_real_command(const char* input, int year) {
    char buf[MAX_CMD_LEN];
    strncpy(buf, input, sizeof(buf));
    char* cmd = strtok(buf, " ");
    if (!cmd) return;
    char* args[MAX_CMD_LEN / 2];
    int i = 0;
    args[i++] = cmd;
    char* arg;
    int unsupported = 0;
    while ((arg = strtok(NULL, " "))) {
        if (arg[0] == '-' && !is_option_supported(cmd, arg, year)) {
            printf("Option '%s' not supported in year %d.\n", arg, year);
            unsupported = 1;
        }
        args[i++] = arg;
    }
    args[i] = NULL;
    if (unsupported) return;
    FILE* fp = popen(input, "r");
    if (!fp) return;
    char out[512];
    while (fgets(out, sizeof(out), fp)) {
        fputs(out, stdout);
    }
    pclose(fp);
}

void simulate_shell(UnixEra* era, const char* shell) {
    char input[MAX_CMD_LEN];
    while (1) {
        printf("$ ");
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;
        if (strcmp(input, "exit") == 0) break;
        if (strcmp(input, "echo $SHELL") == 0) {
            printf("/bin/%s\n", shell);
            continue;
        }
        int valid = 0;
        for (int i = 0; i < era->cmd_count; i++) {
            if (strncmp(input, era->commands[i], strlen(era->commands[i])) == 0) {
                valid = 1;
                break;
            }
        }
        if (!valid) {
            printf("Command not available in year %d.\n", era->year);
            continue;
        }
        run_real_command(input, era->year);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3 || strcmp(argv[1], "run") != 0) {
        printf("Usage: %s run .teleport\n", argv[0]);
        return 1;
    }

    FILE* fp = fopen(argv[2], "r");
    if (!fp) {
        printf("Cannot open teleport file.\n");
        return 1;
    }

    int year = 0;
    char command[64] = {0};
    char shell[16] = "sh";
    char line[128];

    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "Time: %d", &year) == 1) continue;
        if (sscanf(line, "Command: %s", command) == 1) continue;
        if (sscanf(line, "Shell: %s", shell) == 1) continue;
    }
    fclose(fp);

    if (year < 1971) {
        printf("Invalid year.\n");
        return 1;
    }

    UnixEra* era = get_closest_era(year);
    if (!era) {
        printf("No closest era found. Exiting.\n");
        return 1;
    }

    printf("Selected closest valid year: %d\n", era->year);

    int valid_shell = 0;
    for (int i = 0; i < era->shell_count; i++) {
        if (strcmp(shell, era->shells[i]) == 0) {
            valid_shell = 1;
            break;
        }
    }
    if (!valid_shell) {
        printf("Shell '%s' not supported. Using 'sh'.\n", shell);
        strcpy(shell, "sh");
    }

    simulate_shell(era, shell);
    return 0;
}
