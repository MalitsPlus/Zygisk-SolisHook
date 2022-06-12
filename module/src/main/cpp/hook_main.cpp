//
// Created by kotori0 on 2020/2/5.
//

#include <cstring>
#include <unistd.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <dobby.h>
#include <string>
#include "hook_main.h"
#include "hook_override.h"

#include IL2CPPCLASS // = il2cppapi/2020.2.4f1/il2cpp-class.h

#define DO_API(r, n, p) r (*n) p // Il2CppClass* (*il2cpp_object_get_class) (Il2CppObject * obj)

#include IL2CPPAPI // = il2cppapi/2020.2.4f1/il2cpp-api-functions.h

#undef DO_API

// initialize all il2cpp apis defined in #IL2CPPAPI file
void init_il2cpp_api() {
#define DO_API(r, n, p) n = (r (*) p)dlsym(il2cpp_handle, #n)
// il2cpp_object_get_class = (Il2CppClass (*) (Il2CppObject * obj))dlsym(il2cpp_handle, "il2cpp_object_get_class")

#include IL2CPPAPI

#undef DO_API
}

int isGame(JNIEnv *env, jstring appDataDir) {
    if (!appDataDir)
        return 0;

    const char *app_data_dir = env->GetStringUTFChars(appDataDir, nullptr);

    int user = 0;
    static char package_name[256];
    if (sscanf(app_data_dir, "/data/%*[^/]/%d/%s", &user, package_name) != 2) {
        if (sscanf(app_data_dir, "/data/%*[^/]/%s", package_name) != 1) {
            package_name[0] = '\0';
            LOGW("can't parse %s", app_data_dir);
            return 0;
        }
    }
    env->ReleaseStringUTFChars(appDataDir, app_data_dir);
    if (strcmp(package_name, game_name) == 0) {
        LOGD("detect game: %s", package_name);
        return 1;
    }
    else {
        return 0;
    }
}

unsigned long get_module_base(const char* module_name)
{
    FILE *fp;
    unsigned long addr = 0;
    char *pch;
    char filename[32];
    char line[1024];

    snprintf(filename, sizeof(filename), "/proc/self/maps");

    fp = fopen(filename, "r");

    if (fp != nullptr) {
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, module_name) && strstr(line, "r-xp")) {
                pch = strtok(line, "-");
                addr = strtoul(pch, nullptr, 16);
                if (addr == 0x8000)
                    addr = 0;
                break;
            }
        }
        fclose(fp);
    }
    return addr;
}

typedef void* (*dlopen_type)(const char* name,
                             int flags,
                             //const void* extinfo,
                             const void* caller_addr);
dlopen_type dlopen_backup = nullptr;
void* dlopen_(const char* name,
              int flags,
              //const void* extinfo,
              const void* caller_addr){

    void* handle = dlopen_backup(name, flags, /*extinfo,*/ caller_addr);
    if(!il2cpp_handle){
        LOGI("dlopen: %s", name);
        if(strstr(name, "libil2cpp.so")){
            il2cpp_handle = handle;
            LOGI("Got il2cpp handle at %lx", (long)il2cpp_handle);
        }
    }
    return handle;
}

typedef size_t (*Hook)(char* instance, Il2CppArray* src);
Hook backup = nullptr;
size_t hook(char* instance, Il2CppArray *src){
    if(backup == nullptr){
        LOGE("backup DOES NOT EXIST");
    }
    size_t r = backup(instance, src);
    return r;
}

void hook_each(unsigned long rel_addr, void* hook, void** backup_){
    LOGI("Installing hook at %lx", rel_addr);
    unsigned long addr = /*base_addr + */rel_addr;

    // 设置属性可写
    void* page_start = (void*)(addr - addr % PAGE_SIZE);
    if (-1 == mprotect(page_start, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC)) {
        LOGE("mprotect failed(%d)", errno);
        return ;
    }

    DobbyHook(
            reinterpret_cast<void*>(addr),
            hook,
            backup_);
    mprotect(page_start, PAGE_SIZE, PROT_READ | PROT_EXEC);
}

void hackOne(const Il2CppAssembly** assembly_list, unsigned long size, const char* assemblyName, const char* nameSpace, const char* className, const char* methodName, int argsCount, void* hookMethod, void** backupMethod) {
    LOGI("== start hacking %s.%s ==", className, methodName);
    LOGI("%ld assemblies here", size);
    int i = 0;
    for (; i < size; i++) {
        if (*(assembly_list + i) != nullptr) {
            if (strcmp((*(assembly_list + i))->aname.name, assemblyName) != 0) {
                // LOGD("Assembly name: %s, count %ld", (*assembly_list)->aname.name, ++i);
                if (i == size - 1) {
                    LOGE("Cannot find assembly %s, end hacking", assemblyName);
                    return;
                }
            } else {
                // got!
                LOGW("Assembly name: %s", (*(assembly_list + i))->aname.name);
                break;
            }
        } else {
            LOGE("assembly_list is null, stop hacking");
            return;
        }
    }

    const Il2CppImage* image = il2cpp_assembly_get_image(*(assembly_list + i));
    LOGI("image got at %lx", (long)image);
    Il2CppClass* clazz = il2cpp_class_from_name(image, nameSpace, className);
    LOGI("clazz got at %lx", (long)clazz);
    Il2CppClass* final_clazz = clazz;
    const MethodInfo* methodInfo = il2cpp_class_get_method_from_name(final_clazz, methodName, argsCount);
    LOGI("methodInfo got at %lx", (long)methodInfo);
    auto addr = (unsigned long)methodInfo->methodPointer;
    LOGI("method got at %lx", addr);
    LOGI("start hook target method...");
    hook_each(addr, hookMethod, backupMethod);
    LOGD("== hack %s.%s finished ==", className, methodName);
}

void hackOneNested(const Il2CppAssembly** assembly_list, unsigned long size, const char* assemblyName, const char* nameSpace, const char* className, const char* nestedClassName, const char* methodName, int argsCount, void* hookMethod, void** backupMethod) {
    LOGI("== start hacking %s.%s ==", className, methodName);
    LOGI("%ld assemblies here", size);
    int i = 0;
    for (; i < size; i++) {
        if (*(assembly_list + i) != nullptr) {
            if (strcmp((*(assembly_list + i))->aname.name, assemblyName) != 0) {
                // LOGD("Assembly name: %s, count %ld", (*assembly_list)->aname.name, ++i);
                if (i == size - 1) {
                    LOGE("Cannot find assembly %s, end hacking", assemblyName);
                    return;
                }
            } else {
                LOGW("Assembly name: %s", (*(assembly_list + i))->aname.name);
                break;
            }
        } else {
            LOGE("assembly_list is null, stop hacking");
            return;
        }
    }

    const Il2CppImage* image = il2cpp_assembly_get_image(*(assembly_list + i));
    LOGI("image got at %lx", (long)image);
    Il2CppClass* clazz = il2cpp_class_from_name(image, nameSpace, className);
    LOGI("clazz got at %lx", (long)clazz);
    Il2CppClass* final_clazz = nullptr;
    // 注意：clazz->nested_type_count 可能不是count，需要验证结构体正确性
    uint16_t nested_type_count = clazz->nested_type_count;
    LOGI("nested_type_count %d", nested_type_count);
    if (nested_type_count) {
        void* iter = nullptr;
        while (const Il2CppClass* nestedClazz = il2cpp_class_get_nested_types(clazz, &iter)) {
            if (!strcmp(nestedClazz->name, nestedClassName)) {
                final_clazz = (Il2CppClass*)nestedClazz;
                break;
            }
        }
//        void** iter = (void**)malloc(size * sizeof(size_t));
//        LOGD("**iter at %ld", long(iter));
//        LOGD("*iter at %ld", long(*iter));
//        // 注意这里取的是第一个嵌套类，如果有多个嵌套类需要进一步处理
//        nestedClazz = il2cpp_class_get_nested_types(clazz, iter);
    } else {
        LOGE("No nested class in %s, end hacking", className);
        return;
    }
    if (final_clazz == nullptr) {
        LOGE("No nested class named %s in %s", nestedClassName, className);
        return;
    }
    LOGI("finalclazz got at %lx", (long)final_clazz);
    LOGI("finalclazz name is %s", final_clazz->name);

//    for (int i = 0; i < nested_type_count; i++) {
//        LOGI("nestedTypes at %lx", (long)clazz->nestedTypes);
//        Il2CppClass *c = (Il2CppClass*)clazz->nestedTypes + i;
//        const char *nst_name = c->name;
//        LOGI("one nested class named %s", nst_name);
//        if (strcmp(nst_name, nestedClassName) == 0) {
//            final_clazz = c;
//            break;
//        }
//    }
    const MethodInfo* methodInfo = il2cpp_class_get_method_from_name(final_clazz, methodName, argsCount);
    LOGI("methodInfo got at %lx", (long)methodInfo);
    auto addr = (unsigned long)methodInfo->methodPointer;
    LOGI("method got at %lx", addr);
    LOGI("start hook target method...");
    hook_each(addr, hookMethod, backupMethod);
    LOGD("== hack %s.%s finished ==", className, methodName);
}

// entrance point
void *hack_thread(void *arg)
{
    LOGI("hack thread: %d", gettid());
    srand(time(nullptr));
    void* loader_dlopen = DobbySymbolResolver(nullptr, "__dl__Z9do_dlopenPKciPK17android_dlextinfoPKv");
    hook_each((unsigned long)loader_dlopen, (void*)dlopen_, (void**)&dlopen_backup);

    while (true)
    {
        base_addr = get_module_base("libil2cpp.so");
        if (base_addr != 0 && il2cpp_handle != nullptr) {
            break;
        }
    }
    LOGD("detect libil2cpp.so %lx, start sleep", base_addr);
    sleep(2);

    LOGD("hack game begin");
    init_il2cpp_api();

    auto* domain = il2cpp_domain_get();
    il2cpp_thread_attach(domain);
    size_t ass_len = 0;
    const Il2CppAssembly** assembly_list = il2cpp_domain_get_assemblies(domain, &ass_len);

    hackMain(assembly_list, ass_len);

    return nullptr;
}
