#define _XOPEN_SOURCE 500
#include <ftw.h>
#include <limits.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <err.h>

int root_owned = 0;
int absolute = 0;
char *prefix = "";

static const char *
filetype(int tflag)
{
    if (tflag == FTW_F)
        return "reg";
    if (tflag == FTW_D)
        return "dir";
    if (tflag == FTW_SL)
        return "sym";
    fprintf(stderr, "unknown file type flags '%d'\n", tflag);
    exit(1);
}

static int
printfspec(const char *fpath, const struct stat *sb,
            int tflag, struct FTW *ftwbuf)
{
    int len;
    char pathbuf[PATH_MAX];

    if (strcmp(fpath, ".") == 0)
        return 0;

    printf("%s%s\n", prefix, fpath+2);
    printf("type=%s\n", filetype(tflag));
    printf("mode=%04o\n", sb->st_mode & ~S_IFMT);
    if (!root_owned) {
        if (sb->st_uid != 0)
            printf("uid=%d\n", sb->st_uid);
        if (sb->st_gid != 0)
            printf("gid=%d\n", sb->st_gid);
    }

    if (tflag == FTW_SL) {
        len = readlink(fpath, pathbuf, sizeof(pathbuf));
        if (len < 0)
            err(1, "readlink of %s failed", fpath);
        if (len == sizeof(pathbuf))
            errx(1, "link target of %s too long", fpath);
        pathbuf[len] = 0;
        if (strchr(pathbuf, '\n'))
            errx(1, "link target contains new line");
        printf("link=%s\n", pathbuf);
    }
    if (tflag == FTW_F && absolute) {
        if (realpath(fpath, pathbuf) == NULL)
            err(1, "realpath of %s failed", fpath);
        if (strchr(pathbuf, '\n'))
            errx(1, "file path contains new line, aborting");
        printf("source=%s\n", pathbuf);
    }
    puts("");
    return 0;
}

int
main(int argc, char **argv)
{
    int opt;
    const char *dir = ".";

    while ((opt = getopt(argc, argv, "p:ar")) != -1) {
        switch (opt) {
        case 'p':
           prefix = optarg;
           break;
        case 'a':
           absolute = 1;
           break;
        case 'r':
           root_owned = 1;
           break;
        default: 
           fprintf(stderr, "Usage: %s [-p PREFIX] [-a] [-r] [PATH]\n", argv[0]);
           exit(EXIT_FAILURE);
        }
    }

    if (optind < argc)
        dir = argv[optind];

    if (chdir(dir) != 0)
        err(1, "chdir failed");

    if (nftw(".", printfspec, 20, FTW_PHYS) < 0)
        err(1, "walk of %s failed", dir);

    if (ferror(stdout))
        errx(1, "io error");
    return 0;
}
