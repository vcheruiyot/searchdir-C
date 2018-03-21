 /*
* File: searchdir.c
        * Author: YOUR NAME HERE
        * ----------------------
        */

#include "cmap.h"
#include "cvector.h"
#include <sys/time.h>
#include <time.h>
#include <dirent.h>
#include <error.h>
#include <stdio.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define DATE_MAX 6
#define MAX_INODE_ELEMENTS 100
#define NFILES_ESTIMATE 20
#define MAX_INODE_LEN   21  // digits in max unsigned long is 20, plus \0
int cmp_str(const void *f1, const void *f2){
    char str1[MAX_INODE_ELEMENTS]; char str2[MAX_INODE_ELEMENTS];
    strcpy(str1, *(char **)f1); strcpy(str2, *(char **)f2);
    int result = strlen(str1) - strlen(str2);
    if(result == 0){
        return strcmp(str1, str2);
    }else{
        return result;
    }
}

void  CleanupElemnfn(void *element){
    free(*(char **)element);
}
void CleanupMap(void *element){
	cvec_dispose(*(CVector **)element);
}
int cmp_fn(const void *f1, const void *f2){
    return (*(unsigned long *)f1 - *(unsigned long *)f2);
}
void gather_files_t(void *matches, const char *searchstr, const char *dirname, CVector *visited, const char path)
{
    DIR *dp = opendir(dirname);
    struct dirent *entry;
    while(dp != NULL && ((entry = readdir(dp))) != NULL){
        if(entry->d_name[0] == '.')continue;
        unsigned long inode_v = entry->d_ino;
        if(cvec_count(visited) > 0){
            int found = cvec_search(visited, &inode_v, cmp_fn, 0, true);
            if(found != -1)continue;
        }
        cvec_append(visited, &inode_v);
        cvec_sort(visited, cmp_fn);
        char *fullpath = (char *)malloc(sizeof(char *) * (strlen(entry->d_name) + strlen(dirname) + 2));
        sprintf(fullpath, "%s/%s", dirname, entry->d_name); /* construct full path */
        struct stat ss;
	struct tm *tmobject;

        int value = stat(fullpath, &ss);
        unsigned long inode = ss.st_ino;
        char arr_val[22];


        if(value == 0 && S_ISDIR(ss.st_mode)){  /* if subdirectory, recur */
            cvec_append(visited, &inode);
            char arr[strlen(fullpath) + 1]; arr[0] = '\0';
            strcpy(arr, fullpath);
            free(fullpath);
            gather_files_t(matches, searchstr, arr, visited, path);
        }else{
            switch(path){
            case 'd':
                tmobject = localtime((const time_t *)(&ss.st_atim));
                char date_buffer[DATE_MAX];
                strftime(date_buffer, DATE_MAX,"%m/%d",tmobject);
                CVector **found = (CVector **)cmap_get((CMap *)matches, date_buffer);
                if(found == NULL){
                    CVector *found1 = cvec_create(sizeof(char *), NFILES_ESTIMATE, CleanupElemnfn);//how to clean up?
		    cvec_append(found1, &fullpath);
                    cmap_put((CMap *)matches, date_buffer, &found1);
		    
		}else{cvec_append(*found, &fullpath);}
                break;
            case 'i':
                sprintf(arr_val, "%lu", inode);
                cmap_put((CMap *)matches, arr_val, &fullpath);
                break;
            case 'n':
                if(strstr(entry->d_name, searchstr) != NULL){
                    cvec_append((CVector *)matches, &fullpath);
                }else{
                    free(fullpath);
                }
                break;
            }
        }
    }
    closedir(dp);
}

static void inode_search(const char *dirname){
    CMap *matches = cmap_create(sizeof(char *), NFILES_ESTIMATE, CleanupElemnfn);
    CVector *visited = cvec_create(sizeof(unsigned long), MAX_INODE_ELEMENTS, NULL);
    gather_files_t(matches, "", dirname, visited,'i');
    char input[PATH_MAX];
    while(1){
        printf("Enter inode(or q to quit): ");
        int scanresult = scanf("%s", input);
        if(strcmp(input, "q") == 0 ||  scanresult < 0)break;
        char **key = (char **)cmap_get(matches, input);
        if(key == NULL)continue;
        printf("%s\n", *key);
    }
    cmap_dispose(matches);
    cvec_dispose(visited);

}
static void print_cvec(CVector *matches){
    for (char **cur = cvec_first(matches); cur != NULL; cur = cvec_next(matches, cur))
        printf("%s\n", *cur);
}
static void date_search(const char *dirname){
    CMap *matches = cmap_create(sizeof(CVector **), NFILES_ESTIMATE, CleanupMap);
    CVector *visited = cvec_create(sizeof(unsigned long), MAX_INODE_ELEMENTS, NULL);
    gather_files_t(matches, "", dirname, visited, 'd');
    char input[DATE_MAX];
    while(1){
        printf("Enter date MM/DD (or q to quit): ");
        int scanresult = scanf("%s", input);
        if(strcmp(input, "q") == 0 || scanresult < 0)break;
        CVector **paths = (CVector **)cmap_get(matches, input);
	if(paths == NULL)continue;
        print_cvec(*paths);
        cvec_dispose(*paths);
    }
    cmap_dispose(matches);
    cvec_dispose(visited);
}

static void namesearch(const char *searchstr, const char *dirname)
{
    CVector *matches = cvec_create(sizeof(char *), NFILES_ESTIMATE, CleanupElemnfn);
    CVector *visited = cvec_create(sizeof(unsigned long), MAX_INODE_ELEMENTS, NULL);
    gather_files_t(matches, searchstr, dirname, visited, 'n');
    //need to sort the cvector matches based on length
    cvec_sort(matches, cmp_str);
    print_cvec(matches);
    cvec_dispose(matches);
    cvec_dispose(visited);
}

int main(int argc, char *argv[])
{
    if (argc < 2) error(1,0, "Usage: searchdir [-d or -i or searchstr] [(optional) directory].");

    char *dirname = argc < 3 ? "." : argv[2];
    if (access(dirname, R_OK) == -1) error(1,0, "cannot access path \"%s\"", dirname);

    if (0 == strcmp("-d", argv[1])) {
        date_search(dirname);
    } else if (0 == strcmp("-i", argv[1])) {
        inode_search(dirname);

    } else {
        namesearch(argv[1], dirname);
    }
    return 0;
}

