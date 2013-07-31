#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

static inline int is_readonly (char* str)
{
        return (str[0] == 'r' && str[1] != 'w' && str[3] == 'p');
}

static inline int is_readwrite (char* str)
{
        return (str[0] == 'r' && str[1] == 'w' && str[3] == 'p');
}

static inline int is_writeonly (char* str)
{
        return (str[0] != 'r' && str[1] == 'w' && str[3] == 'p');
}

static inline int is_privateonly (char* str)
{
        return (str[0] != 'r' && str[1] != 'w' && str[2] != 'x' && str[3] == 'p');
}

static inline int is_shared (char* str)
{
        return (str[3] == 's');
}

#define MAX_FILE 128
#define MAX_PATH 128
#define MAX_PROT 8
int main(int argc, char* argv[])
{
        unsigned long start, end, ultmp;
        unsigned int itmp0, itmp1, inode;
        char prot[MAX_PROT], path[MAX_PATH], file[MAX_FILE];
        unsigned long ro_total =0, rw_total =0, wo_total =0, po_total =0, shared_total =0, other_total=0;

        //check if the directory exists and add "/maps" to the path
        if (! (argc > 1) ) {
                printf("argc %d\n", argc);
                printf("WARN: You must specify the pid\n");
                return 0;
        }

        DIR * dd = opendir(argv[1]);
        if (!dd) {
                printf("ERROR: directory does not exist\n");
                return 0;
        }
        closedir(dd);

        memset(file, 0, MAX_FILE);
        strcpy(file, argv[1]);
        strcat(file, "/maps");
        FILE * ff = fopen(file, "r");
        if (!ff) {
                printf("ERROR: open error\n");
                return -1;
        }
/*      printf("opened %p\n", ff);
*/
int __ret = 0;
do {
        memset(prot, 0, MAX_PROT);
        __ret = fscanf(ff, "%lx-%lx %4s %lx %02u:%02u %u",
                &start, &end, prot, &ultmp, &itmp0, &itmp1, &inode);
        if (__ret == EOF)
                goto end_loop;

        memset(path, 0, MAX_PATH);
        int i=0;
do {
        __ret = fgetc(ff);
        path[i++] = (__ret != '\n' && __ret != EOF) ? __ret : 0;
} while (__ret != '\n' && __ret != EOF) ;

        unsigned long area_size = (end -start);
/*      printf("%lx-%lx %4s %lx %02u:%02u %u %s - %lu %d\n",
                start, end, prot, ultmp, itmp0, itmp1, inode, path,
                area_size, is_readonly(prot));
*/
        if (is_readonly(prot)) ro_total += area_size ;
        else if (is_readwrite(prot)) rw_total += area_size;
        else if (is_writeonly(prot)) wo_total += area_size;
        else if (is_privateonly(prot)) po_total += area_size;
        else if (is_shared(prot)) shared_total += area_size;
        else
                other_total += area_size;

end_loop:
        ;
} while (__ret != EOF && __ret != 0);

        unsigned long total = ro_total + rw_total + wo_total
                        + po_total + shared_total + other_total;

        if (total == 0) {
                printf("WARN: nothing mapped\n");
                return 0;
        }

/*        printf("\n");
        printf("ro: %ld (%ld)\n"
               "rw: %ld (%ld)\n"
               "wo: %ld (%ld)\n"
               "po: %ld (%ld)\n"
               "sh: %ld (%ld)\n"
               "ot: %ld (%ld)\n",
                ro_total, (unsigned long)((ro_total * 100) / total),
                rw_total, (unsigned long)((rw_total * 100) / total),
                wo_total, (unsigned long)((wo_total * 100) / total),
                po_total, (unsigned long)((po_total * 100) / total),
                shared_total, (unsigned long)((shared_total * 100) / total),
                other_total, (unsigned long)((other_total * 100) / total));
        printf("total: %ld\n", total);
*/
        printf("%ld %ld "
               "%ld %ld "
               "%ld %ld "
               "%ld %ld "
               "%ld %ld "
               "%ld %ld ",
                ro_total, (unsigned long)((ro_total * 100) / total),
                rw_total, (unsigned long)((rw_total * 100) / total),
                wo_total, (unsigned long)((wo_total * 100) / total),
                po_total, (unsigned long)((po_total * 100) / total),
                shared_total, (unsigned long)((shared_total * 100) / total),
                other_total, (unsigned long)((other_total * 100) / total));
        printf("%ld \n", total);

        fclose(ff);
        return 0;
}
