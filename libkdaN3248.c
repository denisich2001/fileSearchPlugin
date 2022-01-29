#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "plugin_api.h"
#include <stdarg.h>

//
// Created by Denis on 05.12.2022.
//

//static char *g_lib_name = "libkdaN3248.so";
static char *g_plugin_purpose = "Find files, which the most frequent byte is input byte.";
static char *g_plugin_author = "Kiryanov Denis";
static struct plugin_option g_po_arr[] = {
    {
        {"freq-byte",
           1,
           0, 0,
        },
    "Target value of a byte"
    }
};

static int g_po_arr_len = sizeof(g_po_arr)/sizeof(g_po_arr[0]);

int plugin_get_info(struct plugin_info* ppi) {
    if (!ppi) {
        fprintf(stderr, "ERROR: invalid argument\n");
        return -1;
    }

    ppi->plugin_purpose = g_plugin_purpose;
    ppi->plugin_author = g_plugin_author;
    ppi->sup_opts_len = g_po_arr_len;
    ppi->sup_opts = g_po_arr;

    return 0;
}
int processDec(char *str, int size){
    //have to be less than 256
    int number=0;
    for(int i=0;i<size;++i){
        if(!(str[i]>='0' && str[i]<='9'))
            return -1;
        number*=10;
        number+= (str[i]-'0');
        if(number>=256)
            return -1;
    }
    return number;
}

int processHex(char *str, int size){
    //have to be less than 256
    if(size == 2)
        return -1;
    int number=0;
    for(int i=2;i<size;++i){
        if(!(str[i]>='0' && (str[i])<=('9'))&&
            !(str[i]>='A' && (str[i])<=('F'))&&
            !(str[i]>='a' && (str[i])<=('f'))
            )
            return -1;
        number*=16;
        if((str[i])>=('a') && (str[i])<=('f')){
            number += (10 + (str[i])-('a'));
        }
        if((str[i])>=('A') && (str[i])<=('F')){
            number += (10 + (str[i])-('A'));
        }
        if((str[i])>=('0') && (str[i])<=('9')){
            number += ((str[i])-('0'));
        }
        if(number>=256)
            return -1;
    }
    return number;
}

int processBin(char *str, int size){
    //have to be less than 256
    if(size==2)
        return -1;
    int number=0;
    for(int i=2;i<size;++i){
        if(!((str[i])>=('0') && (str[i])<=('1')))
            return -1;
        number*=2;
        number+= ((str[i])-('0'));
        if(number>=256)
            return -1;
    }
    return number;
}

int processFreqByte(char *str){
    int size = 0;
    while(str[size]){
        size++;
    }
    if(size==0){
        return -1;
    }
    if(size < 3){   
        return processDec(str, size);
    }
    if(str[0] == '0' && str[1]=='x'){
        return processHex(str, size);
    }
    if(str[0] == '0' && str[1]=='b'){
        return processBin(str, size);
    }
    return processDec(str, size);
}
int plugin_process_file(const char * fileName, struct option paramOptions[], size_t paramOptionsSize){
    if (! fileName || !paramOptions || paramOptionsSize <= 0){
        errno = EINVAL;
        return -1;
    }
    int*rightFile = (int*)malloc(sizeof(int) * paramOptionsSize);
    if (!rightFile){
        perror("malloc");
        return -1;
    }
    for (size_t i = 0; i < paramOptionsSize; i++)
       rightFile[i] = 0;
    FILE *  fileRef;
    
    for (size_t i = 0; i < paramOptionsSize; i++){
        int requestByte = processFreqByte(((char *)paramOptions[i].flag));
        if (requestByte != -1){
            fileRef = fopen( fileName, "r");
            if ( fileRef == NULL)
                exit(EXIT_FAILURE);
            printf("File: %s \n",fileName);
            int allBytesFreq[256];
            for(int i=0;i<256;++i){
                allBytesFreq[i]=0;
            }
            int maxFreq = -1;
            //int*rightFile = (int*)malloc(sizeof(int) * paramOptionsSize);
            //char* currByte = (char*)malloc(1*sizeof(char));
            int currByte = getc(fileRef);
            while(currByte != EOF){
                allBytesFreq[currByte]++;
                if(maxFreq<allBytesFreq[currByte]){
                    maxFreq = allBytesFreq[currByte];
                }
                currByte = getc(fileRef);
            }
            if(allBytesFreq[requestByte]==maxFreq){
                rightFile[i]=1;    
            }
            fclose(fileRef);
        }else{
            fprintf(stderr,"Invalid argument (%s)\n",(char *)paramOptions[i].flag);
            if(rightFile) free(rightFile);
            errno = EINVAL;
            return -1;
        }

    }


    int res = rightFile[0];
    for (size_t i = 1; i < paramOptionsSize; i++){
        res = res & rightFile[i];
    }
    if(rightFile) free(rightFile);
    if (res == 1)
        return 0;
    else if (res == 0)
        return 1;
    else
        return -1;
    return 0;

}
