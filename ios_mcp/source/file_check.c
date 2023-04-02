#include <string.h>

#include "file_check.h"
#include "gfx.h"
#include "fsa.h"
#include "utils.h"
#include "menu.h"



#define COPY_BUFFER_SIZE 1024
#define MAX_PATH_LENGHT 0x27F
#define MAX_DIRECTORY_DEPTH 60

#define printf_fs_error(error_cnt, fmt, ...) ({\
    gfx_set_font_color(COLOR_ERROR); \
    gfx_printf(16, 120, GfxPrintFlag_ClearBG, "Total Errors: %u", error_cnt); \
    gfx_printf(16, 100, GfxPrintFlag_ClearBG, fmt, ##__VA_ARGS__); \
    gfx_set_font_color(COLOR_PRIMARY); })
    
static void* malloc_e(size_t sz){
    void *buff = IOS_HeapAlloc(LOCAL_PROCESS_HEAP_ID, sz);
    if(!buff)
        print_error(16 + 8 + 2 + 50, "Out of memory!");
    return buff;
}

static int error_count = 0;

static int write_log(int fsaHandle, int logHandle, const char *operation, const char *path, int res, void *dataBuffer){
    snprintf(dataBuffer, COPY_BUFFER_SIZE, "%s;%s;-%08X\n", operation, path, res);
    res = FSA_WriteFile(fsaHandle, dataBuffer, strnlen(dataBuffer, COPY_BUFFER_SIZE), 1, logHandle, 0);
    if(res == 1)
        res = FSA_FlushFile(fsaHandle, logHandle);
    if(res<0){
      printf_error(6 + 8 + 2 + 8, "Error writing log: -%08X", -res);
      return -1;
    }
    return 0;
}

static int log_error(int fsaHandle, int logHandle, const char *operation, const char *path, int error, void *dataBuffer){
    error_count++;
    printf_fs_error(error_count, "Error %s %s: -%08X", operation, path, error);
    return write_log(fsaHandle, logHandle, operation, path, -error, dataBuffer);
}


static int tryToReadFile(int fsaHandle, const char *path, int logHandle, char *dataBuffer, int *in_out_error_cnt){
    int ret = 0;
    int readHandle = 0;
    int res = FSA_OpenFile(fsaHandle, path, "r", &readHandle);
    if (res < 0) {
        log_error(fsaHandle, logHandle, "OpenFile", path, res, dataBuffer);
        return 1;
    }

    do{
        res = FSA_ReadFile(fsaHandle, dataBuffer, 1, COPY_BUFFER_SIZE, readHandle, 0);
    } while (res > 0);

    if (res < 0) {
        log_error(fsaHandle, logHandle, "ReadFile", path, res, dataBuffer);
        ret = 2;
    }

    FSA_CloseFile(fsaHandle, readHandle);

    return ret;
}

static int open_dir_e(int fsaHandle, const char *path, int *dir_handle, int logHandle, void *dataBuffer, int *in_out_error_cnt){
    int res = FSA_OpenDir(fsaHandle, path, dir_handle);
    if (res < 0) {
        log_error(fsaHandle, logHandle, "OpenDir", path, res, dataBuffer);
        return 1;
    }
    return 0;
}

int checkDirRecursive(int fsaHandle, const char *base_path, int logHandle){
    void* dataBuffer = NULL;
    char *path = NULL;
    int ret = -1;
    error_count = 0;


    dataBuffer = IOS_HeapAllocAligned(CROSS_PROCESS_HEAP_ID, COPY_BUFFER_SIZE, 0x40);
    if (!dataBuffer) {
        print_error(16 + 8 + 2 + 8, "Out of IO memory!");
        goto error;
    }

    path = (char*) malloc_e(MAX_PATH_LENGHT + 1);
    if (!path) {
        goto error;
    }
    strncpy(path, base_path, MAX_DIRECTORY_DEPTH);

    int depth = -1;
    int dir_stack[MAX_DIRECTORY_DEPTH] = { 0 };
    int res=open_dir_e(fsaHandle, path, dir_stack, logHandle, dataBuffer, &ret);
    if(res)
        goto error;
    depth = 0;
    uint32_t dir_path_len = strnlen(path, MAX_PATH_LENGHT);
    path[dir_path_len] = '/';
    dir_path_len++;
    path[dir_path_len] ='\0';

    ret = 0;
    while(depth >= 0){
        FSDirectoryEntry dir_entry;
        res = FSA_ReadDir(fsaHandle, dir_stack[depth], &dir_entry);
        if(res < 0){
            if(res != END_OF_DIR){
                ret++;
                log_error(fsaHandle, logHandle, "ReadDir", path, res, dataBuffer);
            }
            FSA_CloseDir(fsaHandle, dir_stack[depth]);
            dir_stack[depth] = 0;
            depth--;
            do {
                dir_path_len--;
            } while((dir_path_len > 0) && (path[dir_path_len - 1] != '/'));
            path[dir_path_len] = '\0';
            continue;
        }
        if(dir_entry.stat.flags & DIR_ENTRY_IS_LINK)
            continue; //skip symlinks
        strncpy(path + dir_path_len, dir_entry.name, MAX_PATH_LENGHT);
        gfx_print(16, 16 + 8 + 2 + 15, GfxPrintFlag_ClearBG, path);
        if(!(dir_entry.stat.flags & DIR_ENTRY_IS_DIRECTORY)){
            strncpy(path+dir_path_len, dir_entry.name, MAX_PATH_LENGHT - (dir_path_len + 1));
            res = tryToReadFile(fsaHandle, path, logHandle, dataBuffer, &ret);
            if(res < 0){
                ret = res;
                break;
            }
        } else { // Directory
            if(depth >= MAX_DIRECTORY_DEPTH){
                gfx_set_font_color(COLOR_ERROR);
                gfx_printf(16, 200, GfxPrintFlag_ClearBG, "Exceeded max dir depth %u. Skipping:", depth);
                gfx_print(16, 200, GfxPrintFlag_ClearBG, path);
                gfx_set_font_color(COLOR_PRIMARY);
                write_log(fsaHandle, logHandle, "ExceedDepth", path, depth, dataBuffer);
                continue;
            }
            res = open_dir_e(fsaHandle, path, dir_stack + depth + 1, logHandle, dataBuffer, &ret);
            if(res < 0){
                ret = res;
                break;
            }
            if(res){
                path[dir_path_len] = '\0';
                continue;
            }
            depth++;
            dir_path_len = strnlen(path, MAX_PATH_LENGHT);
            path[dir_path_len] = '/';
            dir_path_len++;
            path[dir_path_len] = '\0';
        }
    }

error:
    //TODO: close remaining directories in error case
    for(; depth >= 0; depth--){
        if(dir_stack[depth])
            FSA_CloseDir(fsaHandle, dir_stack[depth]);
    }
    write_log(fsaHandle, logHandle, "finished", base_path, error_count, dataBuffer);

    if(path)
        IOS_HeapFree(LOCAL_PROCESS_HEAP_ID, path);
    if(dataBuffer)
        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
    return ret?ret:error_count;
}