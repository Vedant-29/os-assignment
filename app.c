/***************************************************
 * app.c
 ***************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <string.h>

#define MAX_GROUPS 30
#define MSGQ_PERMS 0666

/* Message structure used between app and group (optional usage).
   You may design it differently if you prefer. */
typedef struct {
    long mtype;
    int group_id;
    char text[256];
} AppMessage;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <testcase_number>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Build path to input.txt */
    char testcase_folder[50];
    snprintf(testcase_folder, sizeof(testcase_folder), "testcases_%s", argv[1]);

    char input_file_path[128];
    snprintf(input_file_path, sizeof(input_file_path), "%s/input.txt", testcase_folder);

    FILE *fp = fopen(input_file_path, "r");
    if (!fp) {
        perror("fopen input.txt");
        exit(EXIT_FAILURE);
    }

    /* Read from input.txt:
       1) n (#groups)
       2) validation_key
       3) app_key
       4) moderator_key
       5) violation_threshold
       6) group file paths
    */
    int n;                 // number of groups
    int validation_key;    // key for validation message queue
    int app_key;           // key for app-group message queue
    int moderator_key;     // key for moderator-group message queue
    int violation_threshold;

    fscanf(fp, "%d", &n);
    fscanf(fp, "%d", &validation_key);
    fscanf(fp, "%d", &app_key);
    fscanf(fp, "%d", &moderator_key);
    fscanf(fp, "%d", &violation_threshold);

    if (n > MAX_GROUPS) {
        fprintf(stderr, "Number of groups exceeds maximum supported.\n");
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    /* Read group file paths */
    char group_files[MAX_GROUPS][128];
    for(int i = 0; i < n; i++){
        fscanf(fp, "%s", group_files[i]);
    }
    fclose(fp);

    /* Spawn each group */
    pid_t group_pids[MAX_GROUPS];
    for(int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0) {
            /* Child - exec groups.out 
               Pass:
                 1) group file path
                 2) group index i (or parse from file name)
                 3) testcase_number
                 4) validation_key
                 5) app_key
                 6) moderator_key
                 7) violation_threshold
            */
            char group_index_str[10];
            snprintf(group_index_str, sizeof(group_index_str), "%d", i);

            char val_key_str[20], app_key_str[20], mod_key_str[20], viol_str[20];
            sprintf(val_key_str, "%d", validation_key);
            sprintf(app_key_str, "%d", app_key);
            sprintf(mod_key_str, "%d", moderator_key);
            sprintf(viol_str, "%d", violation_threshold);

            execl("./groups.out", 
                  "./groups.out",
                  group_files[i],
                  group_index_str,
                  argv[1],
                  val_key_str,
                  app_key_str,
                  mod_key_str,
                  viol_str,
                  (char*)NULL);

            /* If execl fails: */
            perror("execl groups.out");
            exit(EXIT_FAILURE);
        } else {
            /* Parent just stores PID */
            group_pids[i] = pid;
        }
    }

    /* Wait for all group processes to complete */
    for(int i = 0; i < n; i++){
        int status;
        waitpid(group_pids[i], &status, 0);

        /* Print when group terminates.
           The assignment says: 
           All users terminated. Exiting group process X.
         */
        printf("All users terminated. Exiting group process %d.\n", i);
    }

    return 0;
}
