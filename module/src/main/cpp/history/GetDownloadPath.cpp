// "Oz.GameKit.Version", "AssetBundleInfo" "GetDownloadPath"
typedef cSharpString* (*Hook)(void* instance, cSharpString* src, void* methodInfo);
Hook backup = nullptr;
// KEY POINT
// 替换的 hook 方法写在这里
cSharpString* hook(void* instance, cSharpString *src, void* methodInfo){
    if(backup == nullptr){
        LOGE("Riru-hook: backup DOES NOT EXIST");
    }
    LOGI("Riru-hook: ====== MAGIC SHOW BEGINS ======");

    // 原始调用
    cSharpString* r = backup(instance, src, methodInfo);
    LOGE("Riru-hook: got result at %lx", (long)r);
    LOGE("Riru-hook: length is %d", r->length);

    char* str = getString(r->buf, r->length);

    LOGE("Riru-hook: string is %s", str);
    free(str);

    return r;
}


