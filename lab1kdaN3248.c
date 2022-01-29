#define _XOPEN_SOURCE 500
#define _GNU_SOURCE 
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include "plugin_api.h"
#include <getopt.h>
#include <string.h>
#include <dlfcn.h>
#include <ftw.h>

// КИРЬЯНОВ ДЕНИС АНДРЕЕВИЧ
// N3248
// Variant 4
#define lastFunc(x) (void)(x)

int goingThroughFiles(const char* fpath, const struct stat* statB, int flag_t, struct FTW* bufFTW);

char** input = NULL;
int* loptInd = NULL;
int* startIndex;

const char* find_library(const char* filename) {   //Определяем динамические библиотеки
    const char* so = strrchr(filename, '.');
    if (!so || so == filename) return "";
    return so + 1;
}
struct comandOptions {unsigned int not, and, or;};


struct LibraryParam {
    struct plugin_option *opts;
    char *name;	
    int numberOptions;
    void *start;
};
char* pluginpath;  // переменная под директорию с библиотеками
int librariesSize = 0;
int args_size = 0;

char *CheckStrDenis="CheckStrDenis!";
struct comandOptions comand = {0, 0, 0};
struct LibraryParam *libraries;
struct option *long_opts;

void freeAll() {
    for (int i=0;i<librariesSize;i++){
        if (libraries[i].start) dlclose(libraries[i].start);
    }
    if (pluginpath) free(pluginpath);
    if (startIndex) free(startIndex);
    if (long_opts) free(long_opts);
    if (loptInd) free(loptInd);
    if (input) free(input);
    if (libraries) free(libraries);
}

int main(int argc, char* argv[]) {
    //setenv("LDBG", "0", 1);
    atexit(&freeAll);
    if (argc == 1) {
        fprintf(stderr, "Испольуйте: %s <write directory>\nДля помощи введите флаг. -h\n", argv[0]);
        return 1;
    }
    char* search_dir = "";
    pluginpath = get_current_dir_name();

    int countP = 0;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-P") == 0) {
            if (countP) {
                fprintf(stderr, "ERROR: the option cannot be repeated -- 'P'\n");
                return 1;
            }
            if (i == (argc - 1)) {
                fprintf(stderr, "ERROR: option 'P' needs an argument\n");
                return 1;
            }
            DIR* currDir;
            if ((currDir = opendir(argv[i + 1])) == NULL) {     //Не пустая ли директория
                perror(argv[i + 1]);
                return 1;
            }
            else {
                closedir(currDir);
                pluginpath = strdup(argv[i + 1]);			//Добавляем директорию под плагины
                countP = 1;
            }
        }
        else if (strcmp(argv[i], "-v") == 0) {
            printf("Лабораторная работа №1\n");
            printf("Автор: Кирьянов Денис N3248.\n");
            return 0;
        }
        else if (strcmp(argv[i], "-h") == 0) {

            printf("-P <dir>  Задать каталог с плагинами.\n");
            printf("-A        Объединение опций плагинов с помощью операции «И» (действует по умолчанию).\n");
            printf("-O        Объединение опций плагинов с помощью операции «ИЛИ».\n");
            printf("-N        Инвертирование условия поиска (после объединения опций плагинов с помощью -A или -O).\n");
            printf("-v        Вывод версии программы и информации о программе (ФИО исполнителя, номер группы, номер варианта лабораторной).\n");
            printf("-h        Вывод справки по опциям.\n");
            return 0;
        }
    }

    DIR* currDir;
    struct dirent* dir;
    currDir = opendir(pluginpath);
    if (currDir != NULL) {
        while ((dir = readdir(currDir)) != NULL) {
            if ((dir->d_type) == 8) {
                librariesSize++;   			//Считаем количество динамических библиотек для выделения памяти
            }
        }
        closedir(currDir);
    }
    else {
        perror("opendir");
        exit(EXIT_FAILURE);
    }
    libraries = (struct LibraryParam*)malloc(librariesSize * sizeof(struct LibraryParam));
    if (libraries == 0) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    librariesSize = 0;
    currDir = opendir(pluginpath);
    if (currDir != NULL) {    // Проверяем библиотеку с плагинами
        while ((dir = readdir(currDir)) != NULL) {
            if ((dir->d_type) == 8) {
                if (strcmp(find_library(dir->d_name), "so") == 0) {
                    char filename[258];
                    snprintf(filename, sizeof filename, "%s/%s", pluginpath, dir->d_name);  //полный путь до нашей библиотеки -> в filename
                    void* start = dlopen(filename, RTLD_LAZY);
                    if (!start) {
                        fprintf(stderr, "ERROR: dlopen() failed: %s\n", dlerror());
                        continue;
                    }
                    void* func = dlsym(start, "plugin_get_info");
                    if (!func) {
                        fprintf(stderr, "ERROR: dlsym() failed: %s\n", dlerror());
                    }
                    struct plugin_info pi = { 0 };
                    typedef int (*funcPGI_t)(struct plugin_info*);     //funcPGI_t это тип указателя на plugin_info
                    funcPGI_t funcPGI = (funcPGI_t)func;

                    int ret = funcPGI(&pi);
                    if (ret < 0) {
                        fprintf(stderr, "ERROR: plugin_get_info() failed\n");
                    }
                    libraries[librariesSize].name = dir->d_name;
                    libraries[librariesSize].opts = pi.sup_opts;
                    libraries[librariesSize].numberOptions = pi.sup_opts_len;
                    libraries[librariesSize].start = start;
                    if (getenv("LDBG")) {
                        fprintf(stderr, "DEBUG: Find library:\n\tPlugin name: %s\n\tPlugin purpose: %s\n\tPlugin author: %s\n", dir->d_name, pi.plugin_purpose, pi.plugin_author);
                    }
                    librariesSize++;
                }
            }
        }
        closedir(currDir);
    }
    size_t opt_count = 0;
    for (int i = 0; i < librariesSize; i++) {
        opt_count += libraries[i].numberOptions;
    }

    long_opts = (struct option*)malloc(opt_count * sizeof(struct option));
    if (!long_opts) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    opt_count = 0;
    for (int i = 0; i < librariesSize; i++) {
        for (int j = 0; j < libraries[i].numberOptions; j++) {
            long_opts[opt_count] = libraries[i].opts[j].opt;
            opt_count++;
        }
    }
    int checker;
    int check_exdir = 0;
    int optindex = -1;
    while ((checker = getopt_long(argc, argv, "-P:AON", long_opts, &optindex)) != -1) {
        switch (checker) {
        case 'O':
            if (!comand.or) {
                if (!comand.and) {
                    comand.or = 1;
                }
                else {
                    fprintf(stderr, "ERROR: can be either 'A' or 'O' \n");
                    return 1;
                }
            }
            else {
                fprintf(stderr, "ERROR: the option 'O' can't be repeated\n");
                return 1;
            }
            break;
        case 'A':
            if (!comand.and) {
                if (!comand.or) {
                    comand.and = 1;
                }
                else {
                    fprintf(stderr, "ERROR: can be either 'A' or 'O' \n");
                    return 1;
                }
            }
            else {
                fprintf(stderr, "ERROR: the option 'A' cannot be repeated\n");
                return 1;
            }
            break;
        case 'N':
            if (!comand.not) {
                comand.not = 1;
            }
            else {
                fprintf(stderr, "ERROR: the option cannot be repeated -- 'N'\n");
                return 1;
            }
            break;
        case 'P':
            break;
        case ':':
            return 1;
        case '?':
            return 1;
        default:
            if (optindex != -1) {
                args_size++;
                if (getenv("LDBG")) {
                    fprintf(stderr, "DEBUG: Argument found: %s with option: %s\n", optarg, long_opts[optindex].name);
                }
                loptInd = (int*)realloc(loptInd, args_size * sizeof(int));
                if (!loptInd) {
                    perror("realloc");
                    exit(EXIT_FAILURE);
                }
                input = (char**)realloc(input, args_size * sizeof(char*));
                if (!input) {
                    perror("realloc");
                    exit(EXIT_FAILURE);
                }
                input[args_size - 1] = optarg;
                loptInd[args_size - 1] = optindex;
                optindex = -1;
            }
            else {
                if (check_exdir) {
                    fprintf(stderr, "ERROR: the examine directory has already been set at '%s'\n", search_dir);
                    return 1;
                }
                if ((currDir = opendir(optarg)) == NULL) {    //optarg  указатель на следующий аргумент, стоящий после того, что был найден
                    perror(optarg);
                    return 1;
                }
                else {
                    search_dir = optarg;  //установили директорию под поиск 
                    check_exdir = 1;
                    closedir(currDir);
                }
            }
        }
    }

    if (strcmp(search_dir, "") == 0) {
        fprintf(stderr, "ERROR: The directory for research isn't specified\n");
        return 1;
    }
    if (getenv("LDBG")) {
        fprintf(stderr, "DEBUG: Directory for research: %s\n", search_dir);
    }
    if ((comand.and == 0) && (comand.or == 0)){
        comand.and = 1;		// Если ничего не вводили из команд, то по умолчанию используется and.
    }

    if (getenv("LDBG")) {
        fprintf(stderr, "DEBUG: Information about input comands:\n\tAND: %d\tOR: %d\tNOT: %d\n", comand.and, comand.or, comand.not);
    }
    startIndex = (int*)malloc(args_size * sizeof(int));
    if (!startIndex) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < args_size; i++) {
        const char* opt_name = long_opts[loptInd[i]].name;
        for (int j = 0; j < librariesSize; j++) {
            for (int k = 0; k < libraries[j].numberOptions; k++) {
                if (strcmp(opt_name, libraries[j].opts[k].opt.name) == 0) {
                    startIndex[i] = j;
                }
            }
        }
    }

    if (getenv("LDBG")) {
        fprintf(stderr, "DEBUG: Directory with libraries: %s\n", pluginpath);
    }
    int res = nftw(search_dir, goingThroughFiles, 20, FTW_PHYS || FTW_DEPTH);
    if (res < 0) {
        perror("nftw");
        return 1;
    }
    printf("%s\n", CheckStrDenis);

    for (int i = 0; i < librariesSize; i++) {
        if (libraries[i].start) dlclose(libraries[i].start);
    }


    return 0;
}


typedef int (*prFunc_t)(const char* name, struct option in_opts[], size_t in_opts_len);

int goingThroughFiles(const char* fpath, const struct stat* statB, int flag_t, struct FTW* bufFTW) {
    if (flag_t == FTW_F) {     // FTW_F-обычный файл
        int conditions = comand.not ^ comand.and;
        for (int i = 0; i < args_size; i++)
        {
            struct option opt = long_opts[loptInd[i]];
            char* arg = input[i];
            if (arg) {
                opt.has_arg = 1;
                opt.flag = (void*)arg;
            }
            else {
                opt.has_arg = 0;
            }
            void* func = dlsym(libraries[startIndex[i]].start, "plugin_process_file");
            prFunc_t proc_func = (prFunc_t)func;
            int res_func;
            res_func = proc_func(fpath, &opt, 1);
            if (res_func) {
                if (res_func > 0) {res_func = 0;}
                else {
                    fprintf(stderr, "Ошибка плагина\n");
                    return 1;
                }
            }
            else {
                res_func = 1;
            }
            if (comand.not ^ comand.and) {
                conditions = conditions & (comand.not ^ res_func);
            }
            else {
                conditions = conditions | (comand.not ^ res_func);

            }
        }
        if (conditions) {
            printf("Подходящий файл: %s\n", fpath);
        }
        else {
            if (getenv("LDBG")) {
                fprintf(stderr, "\tDEBUG: Файл %s не подходит под критерий поиска\n", fpath);
            }
        }
    }
    lastFunc(statB);
    lastFunc(bufFTW);
    return 0;
}
