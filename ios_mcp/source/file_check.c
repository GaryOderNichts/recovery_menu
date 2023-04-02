#include "file_check.h"
#include "gfx.h"
#include "fsa.h"

#define COPY_BUFFER_SIZE 1024
#define DIR_ENTRY_IS_LINK 0x10000
#define MAX_PATH_LENGHT 0x27F
#define END_OF_DIR -0x30004
#define MAX_DIRECTORY_DEPTH 60

#define printf_fs_error(error_cnt, fmt, ...) ({\
    gfx_set_font_color(COLOR_ERROR); \
    gfx_printf(16, 120, GfxPrintFlag_ClearBG, "Total Errors: %u", error_cnt); \
    gfx_printf(16, 100, GfxPrintFlag_ClearBG, fmt, ##__VA_ARGS__); \
    gfx_set_font_color(COLOR_PRIMARY); })

static int tryToReadFile(int fsaHandle, const char *path, int fileHandle, char *dataBuffer, int *in_out_error_cnt)
{
    int ret = 0;
    int readHandle = 0;
    int res = FSA_OpenFile(fsaHandle, path, "r", &readHandle);
    if (res < 0) {
        (*in_out_error_cnt)++;
        ret = 1;
        printf_fs_error(*in_out_error_cnt, "Error opening file %s: -%08X", path, -res);
        snprintf(dataBuffer, COPY_BUFFER_SIZE, "OpenFile;%s;-%08X\n", path, -res);
        res = FSA_WriteFile(fsaHandle, dataBuffer, strnlen(dataBuffer, COPY_BUFFER_SIZE), 1, fileHandle, 0);
        if(res != 1)
            return res;
        return 1;
    }

    do{
        res = FSA_ReadFile(fsaHandle, dataBuffer, 1, COPY_BUFFER_SIZE, readHandle, 0);
    } while (res > 0);

    if (res < 0)
    {
        (*in_out_error_cnt)++;
        printf_fs_error(*in_out_error_cnt, "Error reading file %s: -%08X", path, -res);
        ret = 1;
        snprintf(dataBuffer, COPY_BUFFER_SIZE, "ReadFile;%s;-%08X\n", path, -res);
        res = FSA_WriteFile(fsaHandle, dataBuffer, strnlen(dataBuffer, 0x400), 1, fileHandle, 0);
        if (res != 1) {
            printf_error(6 + 8 + 2 + 8, "Error writing log: -%08X", -res);
            ret = -1;
        }
    }

    FSA_CloseFile(fsaHandle, readHandle);

    return ret;
}

int checkDirRecursive(int fsaHandle, const char *base_path, int logHandle)
{
    void* dataBuffer = NULL;
    char *path = NULL;
    int ret = 0;


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
    int res=open_dir_e(path, dir_stack, logHandle, dataBuffer, &ret);
    if(ret)
        goto error;
    depth = 0;
    uint32_t dir_path_len = strnlen(path, MAX_PATH_LENGHT);
    path[dir_path_len] = '/';
    dir_path_len++;
    path[dir_path_len] ='\0';

    while(depth >= 0){
        FSDirectoryEntry dir_entry;
        res = FSA_ReadDir(fsaHandle, dir_stack[depth], &dir_entry);
        if(res < 0){
            if(res != END_OF_DIR){
                ret++;
                printf_fs_error(ret, "Error reading dir %s: -%08X", path, -res);
                //printf_error(200, "Error reading dir %s: -%08X", path, -res);
                snprintf(dataBuffer, COPY_BUFFER_SIZE, "ReadDir;%s;-%08X\n", path, -res);
                res = FSA_WriteFile(fsaHandle, dataBuffer, strnlen(dataBuffer, MAX_PATH_LENGHT), 1, logHandle, 0);
                if (res != 1) {
                    printf_error(6 + 8 + 2 + 8, "Error writing log: -%08X", -res);
                    ret = -1;
                    break;
                }
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
                gfx_printf(16, 200, GfxPrintFlag_ClearBG, "Exceeded max directory depth %u. Skipping Directory:", depth);
                gfx_print(16, 200, GfxPrintFlag_ClearBG, path);
                gfx_set_font_color(COLOR_PRIMARY); 
                snprintf(dataBuffer, COPY_BUFFER_SIZE, "ExceedDepth;%s;u\n", path, depth);
                res = FSA_WriteFile(fsaHandle, dataBuffer, strnlen(dataBuffer, MAX_PATH_LENGHT), 1, logHandle, 0);
                if (res != 1) {
                    printf_error(6 + 8 + 2 + 8, "Error writing log: -%08X", -res);
                    ret = -1;
                    break;
                }
                continue;
            }
            res = open_dir_e(path, dir_stack + depth + 1, logHandle, dataBuffer, &ret);
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

    snprintf(dataBuffer, COPY_BUFFER_SIZE, "finished;%s;-%08X\n", path, ret);
    FSA_WriteFile(fsaHandle, dataBuffer, strnlen(dataBuffer, MAX_PATH_LENGHT), 1, logHandle, 0);

    if(path)
        IOS_HeapFree(LOCAL_PROCESS_HEAP_ID, path);
    if(dataBuffer)
        IOS_HeapFree(CROSS_PROCESS_HEAP_ID, dataBuffer);
    return ret;
}