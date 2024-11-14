#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <stdbool.h>
#include <sys/mount.h>
#include <stdint-gcc.h>
#include <sys/stat.h>
#define _GNU_SOURCE

void printEnv(char* var) {
    char* varPtr = getenv(var);
    if (varPtr == NULL)
        printf("Variable %s not found\n", var);
    else
        printf("%s\n", varPtr);
}

void sighup() {
    signal(SIGHUP, sighup); /* reset signal */
    printf("Configuration reloaded\n");
}

bool is_bootable_device(char* device_name) {
    while(*device_name  == ' ') device_name++;

    const char* root = "/dev/";
    char full_path[128];
    sprintf(full_path, "%s%s", root, device_name);

    FILE* device_file = fopen(full_path, "rb");

    if(device_file == NULL) {
        printf("There is no such disk!\n");
        return false;
    }

    int position = 510;
    if(fseek(device_file, position, SEEK_SET) != 0) {
        printf("Error while SEEK operation!\n");
        fclose(device_file);
        return false;
    }

    uint8_t data[2];
    if(fread(data, 1, 2, device_file) != 2) {
        printf("Error while fread operation\n");
        fclose(device_file);
        return false;
    }
    fclose(device_file);

    if(data[1]==0xaa && data[0]==0x55)
        return true;
    return false;
}

bool appendToFile(char* path1, char* path2) {
    FILE *f1 = fopen(path1, "a");
    FILE *f2 = fopen(path2, "r");
    if (!f1 || !f2) {
        printf("Error while reading file %s\n", path2);
        return false;
    }
    char buf[256];

    while (fgets(buf, 256, f2) != NULL) {
        fputs(buf, f1);
    }
    fclose(f1);
    fclose(f2);
    return true;
}

char* concat(const char *s1, const char *s2) {
    char *result = malloc(strlen(s1) + strlen(s2) + 1);
    strcpy(result, s1);
    memset (s1,'\0',sizeof(s1));
    strcat(result, s2);
    return result;
}

void makeDump(DIR* dir, char* path) {
    FILE* res = fopen("res.txt", "w+");
    fclose(res);
    struct dirent* ent;
    char* file_path;
    while ((ent = readdir(dir)) != NULL) {

        asprintf(&file_path, "%s/%s", path, ent->d_name); // asprintf работает
        if(!appendToFile("res.txt", file_path)) {
            return;
        }
    }
    printf("Dump completed!\n");
}

bool stringEquals(const char* string1, const char* string2) {
    return !strcmp(string1, string2);
}

const char* vfs = "/tmp/cronvfs";
const char* crontab = "/var/spool/cron/crontabs";

void mountVFS(char* SOURCE) {
    char* MOUNT_POINT = "/tmp/vfs";
    struct stat st;
    if (stat(MOUNT_POINT, &st) == -1) {
        if (mkdir(MOUNT_POINT, 0755) == -1) {
            printf("Ошибка при создании каталога\n");
            return;
        }
    }

    // Монтируем VFS
    if (mount(SOURCE, MOUNT_POINT, "vfs", 0, NULL) == -1) {
        printf("Ошибка при монтировании VFS\n");
        return;
    }

    printf("VFS успешно смонтирован в %s\n", MOUNT_POINT);
    return;
}

static void cron_create(int i, char *path, char *str) {
    FILE *f;
    char *filename;

    sprintf(filename, "task%d", i);
    concat(path, filename);
    free(filename);

    f = fopen(path, "w");
    if (f) {
        fprintf(f, "%s\n", str);
        fclose(f);
    }

}


static void cron_list(const char *filename, char *path) {
    int i = 0;
    int c;
    char* str;
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror(filename);
        return;
    }

    while ((c = fgetc(f)) != EOF) {
        if (c != '\n') {
            concat(&str, (const char *) c);
            continue;
        }
        if (strlen(str) && str[0] != '#') {
            cron_create(i, path, str);
            i++;
        }
        str = "";
    }
    fclose(f);
}


void cron_mount() {
    int mr;

    mr = mkdir(vfs, 0777);
    if (mr == -1) {
        perror(vfs);
        return;
    }

    mr = mount("tmpfs", vfs, "tmpfs", 0, NULL);
    if (mr == -1) {
        perror("mount");
        return;
    }
}

void cron_umount() {
    umount(vfs);
    rmdir(vfs);
}

void cron() {
    DIR *dir;
    struct dirent *dirent;
    char* path = crontab;
    char* vfs_path = vfs;

    dir = opendir(crontab);
    if (!dir) {
        perror(crontab);
        return;
    }

    while ((dirent = readdir(dir)) != NULL) {
        struct stat buf;

        asprintf(path, "%s", dirent->d_name);

        lstat(path, &buf);
        if ((buf.st_mode & S_IFMT) == S_IFREG) {
            cron_list(path, &vfs_path);
        }

    }

    closedir(dir);
}

int main() {
    signal(SIGHUP, sighup);

    char input[100];
    FILE *file = fopen("hist.txt", "a");
    printf("$ ");
    while (fgets(input, sizeof(input), stdin)) {

        if (stringEquals(input, "exit\n") || stringEquals(input, "\\q\n") != 0) {
            fclose(file);
            return 0;
        }

        if (file) {
            fprintf(file, "%s", input);
            fclose(file);
            file = fopen("hist.txt", "a");
        }

        printf("%s", input);

        char *command = strtok(input, " ");

        if (stringEquals(command, "echo")) {
            command = strtok(NULL, " ");
            while (command[strlen(command) - 1] != '\0') {
                printf("%s ", command);
                command = strtok(NULL, " ");
            }
            printf("%s", command);
        }
        else if(!strcmp(command, "\\e")) {
            command = strtok(NULL, " $");
            while (command[strlen(command) - 1] != '\n') {
                printEnv(command);
                command = strtok(NULL, " $");
            }
            command[strlen(command) - 1] = '\0';
            printEnv(command);
        }
        else if(stringEquals(command, "\\exec")) {
            command = strtok(NULL, " ");
            command[strlen(command) - 1] = '\0';
            if (fork() == 0) { // разделиться
                execv(command, (char *const *) ""); // заменить процесс
                perror("exec one failed"); // если что-то пошло не так, выдать ошибку
                exit(1); // здесь завершаем новый процесс, если не отработал execl
            }
            sleep(1);
        }
        else if(stringEquals(command, "\\l")) {
            command = strtok(NULL, " ");
            command[strlen(command) - 1] = '\0';
            if (is_bootable_device(command))
                printf("Disk %s is bootable\n", command);
            else
                printf("Disk %s isn't bootable\n", command);
        }
        else if(stringEquals(command, "\\cron")) {
            command = strtok(NULL, " ");
            command[strlen(command) - 1] = '\0';
            mountVFS(command);
        }
        else if(stringEquals(command, "\\proc")) {
            command = strtok(NULL, " ");
            command[strlen(command) - 1] = '\0';

            char* path;
            asprintf(&path, "/proc/%s/map_files", command);

            DIR* dir = opendir(path);
            if (dir) {
                makeDump(dir, path);
            }
            else {
                printf("Process not found\n");
            }
        }
        else {
            printf("command not found\n");
        }

        printf("$ ");
    }

    if (file) {
        fclose(file);
    }
    return 0;
}