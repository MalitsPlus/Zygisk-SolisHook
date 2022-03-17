//
// Created by kotori0 on 2020/2/5.
//

#ifndef RIRU_MODULETEMPLATE_MASTER_HOOK_MAIN_H
#define RIRU_MODULETEMPLATE_MASTER_HOOK_MAIN_H
#include <jni.h>
#include <android/log.h>
#include <link.h>
#include <vector>
#include "il2cpp.h"

extern "C" {
extern void *enhanced_dlopen(const char *filename, int flags);
extern void *enhanced_dlsym(void *handle, const char *symbol);
extern int enhanced_dlclose(void *handle);
}

static int enable_hack;
static const char* game_name = "game.qualiarts.idolypride"; // FIXME: EDIT THIS TO YOUR TARGET GAME'S NAME
static unsigned long base_addr = 0;
static void* il2cpp_handle = nullptr;
int isGame(JNIEnv *env, jstring appDataDir);
unsigned long get_module_base(const char* module_name);
void *hack_thread(void *arg);

void hackOne(const Il2CppAssembly** assembly_list, unsigned long size, const char* assemblyName, const char* nameSpace, const char* className, const char* methodName, int argsCount, void* hookMethod, void** backupMethod);
void hackOneNested(const Il2CppAssembly** assembly_list, unsigned long size, const char* assemblyName, const char* nameSpace, const char* className, const char* nestedClassName, const char* methodName, int argsCount, void* hookMethod, void** backupMethod);

#define LOG_TAG "UNITYHOOK"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#endif //RIRU_MODULETEMPLATE_MASTER_HOOK_MAIN_H
