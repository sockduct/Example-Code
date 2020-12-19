/*
 * James Small
 * SDEV-385-81, Fall Semester 2020
 * Week 10 Assignment
 * Date:  November 8, 2020
 * Version:  2020-11-7-01
 *
 * Linux File Utility Program
 * - This assignment will help you to understand Linux operating system file
 *   structure and basic file interface techniques
 *   ○ The systems administrator at your company has asked you to write a file
 *     utility program that gives useful information about the file system on
 *     the company server
 *   ○ Your program should include as a minimum:
 *     . A menu-driven or GUI interface (your choice)
 *     . Thorough commenting
 *     . Error handling
 *     . The ability to select a file or files to view
 *     . The ability to select specific information about a file(s) to display
 *     . The ability to change the permissions on a file or files
 *     . The ability to copy a file to a new directory
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <libgen.h>
#include <locale.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define BLKSZ 4096  // Use same as page size
#define BUFFER_LEN 512
#define FILE_LEN 256
#define FILE_LIMIT 256

// Prototypes:
void change_perms(char files[FILE_LEN][FILE_LIMIT], int number);
void copy_to_dir(char files[FILE_LEN][FILE_LIMIT], int number);
void display_menu();
void display_file_info(char files[FILE_LEN][FILE_LIMIT], int number, char *info_type);
int get_files(char files[FILE_LEN][FILE_LIMIT], int number);
int handlerr(int status, bool exit_on_err, const char *format, ...);

int main() {
    bool loop = true;
    int choice;
    int number;
    int status;
    char files[FILE_LEN][FILE_LIMIT];

    // So we get formatted numbers - e.g., 1024 becomes 1,024
    setlocale(LC_NUMERIC, "");

    while (loop) {
        display_menu();
        status = handlerr(scanf("%d", &choice), false, "scanf");
        if (status == EOF) {
            printf("\n\nEOF detected - quitting.\n");
            exit(1);
        } else if (status != 1) {
            printf("\n\nError:  Unexpected input - try again.\n");
            clearerr(stdin);
            continue;
        }

        if (choice >= 1 && choice <= 5) {
            number = get_files(files, FILE_LIMIT);
        }

        switch (choice) {
            case 1:
                display_file_info(files, number, "ogperm");
                break;
            case 2:
                display_file_info(files, number, "szstor");
                break;
            case 3:
                display_file_info(files, number, "amstimes");
                break;
            case 4:
                change_perms(files, number);
                break;
            case 5:
                copy_to_dir(files, number);
                break;
            case 6:
            default:
                loop = false;
        }
    }

    return 0;
}

void change_perms(char files[FILE_LEN][FILE_LIMIT], int number) {
    int file_descriptor;
    struct stat file_stats;
    mode_t file_mode;
    mode_t new_mode;

    if (number == 0) {
        printf("No file(s) selected!\n");
        return;
    }

    printf("\nChange permissions for file(s):\n");
    for (int i = 0; i < number; ++i) {
        file_descriptor = handlerr(open(files[i], O_RDONLY), true, "open ``%s''",
                                   files[i]);
        handlerr(fstat(file_descriptor, &file_stats), true, "fstat - ``%s''", files[i]);
        file_mode = file_stats.st_mode;
        new_mode = 800;
        printf("         Filename: %s\n", files[i]);
        printf("Current File Mode: %#o\n", file_mode & ~(S_IFMT));
        printf("New file mode:  ");
        handlerr(scanf("%o", &new_mode), false, "scanf");
        if (new_mode > 0777) {
            printf("Error:  Must be [0-7][0-7][0-7] (in octal)\n");
        } else {
            handlerr(fchmod(file_descriptor, new_mode), false, "fchmod - ``%s'' to %u",
                     files[i], new_mode);
        }
        printf("\n");
    }
 }

void copy_to_dir(char files[FILE_LEN][FILE_LIMIT], int number) {
    int in_file_descriptor;
    int out_file_descriptor;
    char outpath[FILE_LEN];
    int status;
    char dirname[FILE_LEN];
    DIR *dirptr;
    mode_t dir_mode = 0775;
    char buffer[BLKSZ];
    struct stat file_stats;

    if (number == 0) {
        printf("No file(s) selected!\n");
        return;
    }

    printf("\nCopy file(s) to directory:\n");
    printf("(New) directory to copy files to:  ");
    status = handlerr(scanf("%255s", dirname), false, "scanf");

    if (status == EOF || status == 0) {
        clearerr(stdin);
        printf("Error:  EOF or unexpected input.\n");
        return;
    }

    // Validate entry:
    dirptr = opendir(dirname);
    if (dirptr == NULL) {
        if (errno == ENOENT) {
            // OK - directory doesn't exist, create:
            handlerr(mkdir(dirname, dir_mode), false, "mkdir - ``%s''", dirname);
            dirptr = opendir(dirname);
            if (dirptr == NULL) {
                // Failed to create directory - should have gotten error from
                // mkdir system call above.
                return;
            } else {
                handlerr(closedir(dirptr), true, "closedir - ``%s''", dirname);
            }
        } else {
            handlerr(-1, false, "opendir - ``%s''", dirname);
            return;
        }
    } else {
        handlerr(closedir(dirptr), true, "closedir - ``%s''", dirname);
    }

    ssize_t incount;
    ssize_t outcount;
    for (int i = 0; i < number; ++i) {
        in_file_descriptor = handlerr(open(files[i], O_RDONLY), true, "open ``%s''",
                                      files[i]);

        handlerr(fstat(in_file_descriptor, &file_stats), true, "fstat - ``%s''", files[i]);
        int out_file_flags = O_CREAT | O_WRONLY | O_TRUNC;
        // int out_file_perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;  // 0644
        int out_file_perms = file_stats.st_mode;
        strncpy(outpath, dirname, FILE_LEN - 1);
        strncat(outpath, "/", FILE_LEN - 1);
        strncat(outpath, files[i], FILE_LEN - 1);
        out_file_descriptor = handlerr(open(outpath, out_file_flags, out_file_perms),
                                       true, "open - ``%s''", files[i]);

        while (true) {
            incount = handlerr(read(in_file_descriptor, buffer, BLKSZ), true,
                                    "read - ``%s''", files[i]);
            if (incount == 0) {
                break;  // EOF
            }

            outcount = handlerr(write(out_file_descriptor, buffer, incount), true,
                                "write - ``%s''", outpath);
            if (outcount < incount) {
                handlerr(-1, true, "write - ``%s''", outpath);
            }
        }

        handlerr(close(in_file_descriptor), true, "close - ``%s''", files[i]);
        handlerr(close(out_file_descriptor), true, "close - ``%s''", outpath);
    }
}

void display_menu() {
    printf("\nMenu:\n"
           "1) Display owner, group, and permissions for file(s)\n"
           "2) Display size and storage information for file(s)\n"
           "3) Display access, modify, and status change times for file(s)\n"
           "4) Change permissions for file(s)\n"
           "5) Copy a file(s) to a (new) directory\n"
           "6) Quit\n"
           "Selection:  ");
}

void display_file_info(char files[FILE_LEN][FILE_LIMIT], int number, char *info_type) {
    int file_descriptor;
    struct stat file_stats;
    struct passwd *user_info;
    char username[BUFFER_LEN];
    struct group *group_info;
    char groupname[BUFFER_LEN];

    if (number == 0) {
        printf("No file(s) selected!\n");
        return;
    }

    if (strcmp(info_type, "amstimes") == 0) {
        printf("\nAccess, Modify, and Status change times for file(s):\n");
    } else if (strcmp(info_type, "ogperm") == 0) {
        printf("\nOwner, group, and permissions for file(s):\n");
    } else if (strcmp(info_type, "szstor") == 0) {
        printf("\nSize and storage information for file(s):\n");
    } else {
        printf("\nError:  Unknown information type ``%s''.\n", info_type);
        return;
    }
    for (int i = 0; i < number; ++i) {
        file_descriptor = handlerr(open(files[i], O_RDONLY), true, "open ``%s''",
                                   files[i]);
        handlerr(fstat(file_descriptor, &file_stats), true, "fstat - ``%s''", files[i]);
        handlerr(close(file_descriptor), true, "close - ``%s''", files[i]);
        if (strcmp(info_type, "amstimes") == 0) {
            printf("               Filename: %s\n", files[i]);
            printf("       Last Access Time: %s", ctime(&file_stats.st_atime));
            printf("       Last Modify Time: %s", ctime(&file_stats.st_mtime));
            printf("Last Status Change Time: %s\n", ctime(&file_stats.st_mtime));
        } else if (strcmp(info_type, "ogperm") == 0) {
            user_info = getpwuid(file_stats.st_uid);
            if (user_info == NULL) {
                strncpy(username, "Unknown", BUFFER_LEN - 1);
            } else {
                strncpy(username, user_info->pw_name, BUFFER_LEN - 1);
            }
            group_info = getgrgid(file_stats.st_gid);
            if (group_info == NULL) {
                strncpy(groupname, "Unknown", BUFFER_LEN - 1);
            } else {
                strncpy(groupname, user_info->pw_name, BUFFER_LEN - 1);
            }

            printf("        Filename: %s\n", files[i]);
            printf(" Owner User (ID): %s (%d)\n", username, file_stats.st_uid);
            printf("Owner Group (ID): %s (%d)\n", groupname, file_stats.st_gid);
            printf("       File Mode: %#o\n\n", file_stats.st_mode & ~(S_IFMT));
        } else { // info_type == "szstore"
            printf("                     Filename: %s\n", files[i]);
            printf("               File Byte Size: %'ld\n", file_stats.st_size);
            printf("Storage Device I/O Block Size: %'ld\n", file_stats.st_blksize);
            printf(" Storage 512 Byte Blocks Used: %'ld\n\n", file_stats.st_blocks);
        }
    }
}

int get_files(char files[FILE_LEN][FILE_LIMIT], int number) {
    int file_count = 0;
    int file_descriptor;
    int status;
    char filename[FILE_LEN];
    struct stat file_stats;

    printf("\nSelect file(s):\n");
    while (file_count < number) {
        printf("Name of file (EOF/Control-D when done):  ");
        // 255 is maximum file length in Linux:
        status = handlerr(scanf("%255s", filename), false, "scanf");

        if (status == EOF || status == 0) {
            clearerr(stdin);
            printf("\n");
            return file_count;
        }

        // Validate entry:
        file_descriptor = handlerr(open(filename, O_RDONLY), false, "open - ``%s''",
                                   filename);
        if (file_descriptor < 0 || file_descriptor == 0) {
            continue;
        }

        // Shouldn't fail here - if do just abort:
        handlerr(fstat(file_descriptor, &file_stats), true, "fstate - ``%s''", filename);

        if ((file_stats.st_mode & S_IFMT) != S_IFREG) {
            printf("Error:  ``%s'' is not a regular file!\n", filename);
            handlerr(close(file_descriptor), true, "close - ``%s''", filename);
            continue;
        }

        handlerr(close(file_descriptor), true, "close - ``%s''", filename);
        strcpy(files[file_count], filename);
        file_count++;
    }

    if (number > 1) {
        printf("\nMaximum number of files (%d) reached!\n", number);
    }
    return number;
}

int handlerr(int status, bool exit_on_err, const char *format, ...) {
    // May want other ways to check for error condition
    // Error conditions:
    // 1) Return code == -1 (typically indicates error)
    //    Exception:  Return code of -1 for getpriority not an error
    // 2) In some cases (e.g., getpriority) must set errno to 0 before call
    //    and check it for non-zero value after call return
    //
    // Note:  This routine currently only deals with case 1
    char syscallstr[BUFFER_LEN];

    va_list arg_list;

    va_start(arg_list, format);
    vsnprintf(syscallstr, BUFFER_LEN, format, arg_list);
    va_end(arg_list);

    if (status == -1 && errno != 0) {
        printf("\nError:  %s - errno %d, %s.\n", syscallstr, errno, strerror(errno));

        if (exit_on_err) {
            exit(1);
        } else {
            // Reset status (e.g., clear EOF)
            status = 0;
        }
    } else if (status < -1) {
        printf("\nWarning:  %s returned negative status of %d - errno currently %d"
               ", %s", syscallstr, status, errno, strerror(errno));
    }

    // Reset errno:
    errno = 0;

    return status;
}
