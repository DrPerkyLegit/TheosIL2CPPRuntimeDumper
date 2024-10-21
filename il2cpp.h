#include <dlfcn.h>
#import <mach-o/dyld.h>

NSString* FrameworkPath = @"Frameworks/UnityFramework.framework/UnityFramework"; 
std::string baseAddressName = "UnityFramework";
//needs to be changed if il2cpp isnt in the same place

namespace IL2CPP 
{
    enum LogLevel : int {
        Info = 1,
        Warning = 2,
        Error = 3
    };
    //Credit: caoyin.
    typedef struct _monoString {
        void* klass;
        void* monitor;
        int length;    
        char chars[1];  

        int getLength() {
            return length;
        }
        char* getChars() {
            return chars;
        }
        NSString* toNSString() {
            return [[NSString alloc] initWithBytes:(const void *)(chars) length:(NSUInteger)(length * 2) encoding:(NSStringEncoding)NSUTF16LittleEndianStringEncoding];
        }

        char* toCString() {
            NSString* v1 = toNSString();
            return (char*)([v1 UTF8String]);  
        }
        std::string toCPPString() {
            return std::string(toCString());
        }
    }monoString;

    struct Il2CppAssemblyName {
        const char* name;
    };
    struct Il2CppAssembly {
        void* image;
        uint32_t token;
        int32_t referencedAssemblyStart;
        int32_t referencedAssemblyCount;
        Il2CppAssemblyName aname;
    };
    
    void* (*il2cpp_domain_get)();
    Il2CppAssembly** (*il2cpp_domain_get_assemblies)(const void* domain, size_t* size);

    uint32_t (*il2cpp_method_get_param_count)(void* method);
    const char* (*il2cpp_method_get_name)(void* method);
    const void* (*il2cpp_method_get_param)(void* method, uint32_t index);
    const char* (*il2cpp_method_get_param_name)(void* method, uint32_t index);
    const void* (*il2cpp_method_get_return_type)(void* method);
    bool (*il2cpp_method_is_instance)(void* method);

    const char* (*il2cpp_field_get_name)(void* field);
    const void* (*il2cpp_field_get_type)(void* field);
    size_t (*il2cpp_field_get_offset)(void* field);

    size_t (*il2cpp_image_get_class_count)(void* image);
    void* (*il2cpp_image_get_class)(void* image, size_t index);

    const char* (*il2cpp_class_get_name)(void* klass);
    const void* (*il2cpp_class_get_methods)(void* klass, void* iter);
    const void* (*il2cpp_class_get_fields)(void* klass, void* iter);
    void* (*il2cpp_class_from_name)(const void* image, const char* namespaze, const char* klass);
    const void* (*il2cpp_class_get_method_from_name)(void* klass, const char* name, int argsCount);
    const char* (*il2cpp_class_get_namespace)(void* klass);
    const void* (*il2cpp_class_get_parent)(void* klass);
    bool (*il2cpp_class_is_abstract)(void* klass);
    bool (*il2cpp_class_is_interface)(void* klass);
    bool (*il2cpp_class_is_enum)(void* klass);


    const char* (*il2cpp_type_get_name)(void* type);
    bool (*il2cpp_type_is_pointer_type)(void* type);
    bool (*il2cpp_type_is_static)(void* type);

    void* (*il2cpp_runtime_invoke)(const void* method, void* obj, void** params, void** exception);

    const void* (*il2cpp_assembly_get_image)(const void* assembly);
    bool canAttach();

    void Attach(std::function<void(std::string, IL2CPP::LogLevel)> DebugFunction);

    const char* getUnityVersion();

    const void* getImageByName(const char* name);

    std::string getTypeNameFull(void* type);

    uint64_t findBaseAddress();

}

bool hasAttached = false;

bool IL2CPP::canAttach() {
    bool handleValid = false;
    //Credit: Il2CPP theos resolver
    NSString *appPath = [[NSBundle mainBundle] bundlePath];
    NSString *unityFrameworkPath = [appPath stringByAppendingPathComponent:FrameworkPath];
    void* handle = dlopen([unityFrameworkPath UTF8String], RTLD_LAZY);

    if (handle) handleValid = true;

    dlclose(handle);
    return handleValid;
}

std::string IL2CPP::getTypeNameFull(void* type) {
    std::string returnString = "";
    const char* paramTypeName = IL2CPP::il2cpp_type_get_name(type);
    bool isStatic = IL2CPP::il2cpp_type_is_static(type);
    bool isPointerType = IL2CPP::il2cpp_type_is_pointer_type(type);

    if (isStatic) {
        returnString += "static ";
    }
    returnString += paramTypeName;
    if (isPointerType) {
        returnString += "*";
    }
    return returnString;
}

//TODO: find out why this is broken
const char* IL2CPP::getUnityVersion() {
    if (!hasAttached) {
        return "Please Attach First";
    }

    const void* unityImage = IL2CPP::getImageByName("UnityEngine");

    void* ApplicationClass = IL2CPP::il2cpp_class_from_name(unityImage, "UnityEngine", "Application");

    const void* getVersionMethod = IL2CPP::il2cpp_class_get_method_from_name(ApplicationClass, "get_unityVersion", 0);

    IL2CPP::monoString* versionMonoString = (IL2CPP::monoString*)IL2CPP::il2cpp_runtime_invoke(getVersionMethod, nullptr, nullptr, nullptr);

    return versionMonoString->toCString();
}

const void* IL2CPP::getImageByName(const char* name) {
    void* domain = IL2CPP::il2cpp_domain_get();

    size_t assemblySize;
    IL2CPP::Il2CppAssembly** assemblies = IL2CPP::il2cpp_domain_get_assemblies(domain, &assemblySize);
    for (int i = 0; i < assemblySize; i++) {
        IL2CPP::Il2CppAssembly* assembly = assemblies[i];
        if (name == assembly->aname.name) {
            return (const void*)assembly;
        }
    }
    return NULL;
}

uint64_t IL2CPP::findBaseAddress() {
    uint32_t image_count = _dyld_image_count();
    for (uint32_t i = 0; i < image_count; i++) {
        const char *image_name = _dyld_get_image_name(i);
        if(strstr(image_name, baseAddressName.c_str())) {
            return _dyld_get_image_vmaddr_slide(i);
        }
    }
    return _dyld_get_image_vmaddr_slide(0);
}

void IL2CPP::Attach(std::function<void(std::string, IL2CPP::LogLevel)> DebugFunction) {
   /* bool hasDebug = false;
    if (DebugFunction != NULL) {
        hasDebug = true;
    }*/

    //Credit: Il2CPP theos resolver
    NSString *appPath = [[NSBundle mainBundle] bundlePath];
    NSString *unityFrameworkPath = [appPath stringByAppendingPathComponent:FrameworkPath];
    void* handle = dlopen([unityFrameworkPath UTF8String], RTLD_LAZY);

    IL2CPP::il2cpp_domain_get = reinterpret_cast<void* (*)()>(dlsym(handle, "il2cpp_domain_get"));
    IL2CPP::il2cpp_domain_get_assemblies = reinterpret_cast<Il2CppAssembly** (*)(const void*, size_t*)>(dlsym(handle, "il2cpp_domain_get_assemblies"));


    IL2CPP::il2cpp_method_get_param_count = reinterpret_cast<uint32_t(*)(void*)>(dlsym(handle, "il2cpp_method_get_param_count"));
    IL2CPP::il2cpp_method_get_name = reinterpret_cast<const char* (*)(void*)>(dlsym(handle, "il2cpp_method_get_name"));
    IL2CPP::il2cpp_method_get_param = reinterpret_cast<const void* (*)(void*, uint32_t)>(dlsym(handle, "il2cpp_method_get_param"));
    IL2CPP::il2cpp_method_get_param_name = reinterpret_cast<const char* (*)(void*, uint32_t)>(dlsym(handle, "il2cpp_method_get_param_name"));
    IL2CPP::il2cpp_method_get_return_type = reinterpret_cast<const void*(*)(void*)>(dlsym(handle, "il2cpp_method_get_return_type"));
    IL2CPP::il2cpp_method_is_instance = reinterpret_cast<bool(*)(void*)>(dlsym(handle, "il2cpp_method_is_instance"));

    IL2CPP::il2cpp_field_get_name = reinterpret_cast<const char* (*)(void*)>(dlsym(handle, "il2cpp_field_get_name"));
    IL2CPP::il2cpp_field_get_type = reinterpret_cast<const void* (*)(void*)>(dlsym(handle, "il2cpp_field_get_type"));
    IL2CPP::il2cpp_field_get_offset = reinterpret_cast<size_t (*)(void*)>(dlsym(handle, "il2cpp_field_get_offset"));

    IL2CPP::il2cpp_image_get_class_count = reinterpret_cast<size_t (*)(void*)>(dlsym(handle, "il2cpp_image_get_class_count"));
    IL2CPP::il2cpp_image_get_class = reinterpret_cast<void* (*)(void*, size_t)>(dlsym(handle, "il2cpp_image_get_class"));

    IL2CPP::il2cpp_class_get_name = reinterpret_cast<const char* (*)(void*)>(dlsym(handle, "il2cpp_class_get_name"));
    IL2CPP::il2cpp_class_get_methods = reinterpret_cast<const void* (*)(void*, void*)>(dlsym(handle, "il2cpp_class_get_methods"));
    IL2CPP::il2cpp_class_get_fields = reinterpret_cast<const void* (*)(void*, void*)>(dlsym(handle, "il2cpp_class_get_fields"));
    IL2CPP::il2cpp_class_from_name = reinterpret_cast<void* (*)(const void*, const char*, const char*)>(dlsym(handle, "il2cpp_class_from_name"));
    IL2CPP::il2cpp_class_get_method_from_name = reinterpret_cast<const void* (*)(void*, const char*, int)>(dlsym(handle, "il2cpp_class_get_method_from_name"));
    IL2CPP::il2cpp_class_get_namespace = reinterpret_cast<const char* (*)(void*)>(dlsym(handle, "il2cpp_class_get_namespace"));
    IL2CPP::il2cpp_class_get_parent = reinterpret_cast<const void* (*)(void*)>(dlsym(handle, "il2cpp_class_get_parent"));
    IL2CPP::il2cpp_class_is_abstract = reinterpret_cast<bool (*)(void*)>(dlsym(handle, "il2cpp_class_is_abstract"));
    IL2CPP::il2cpp_class_is_interface = reinterpret_cast<bool (*)(void*)>(dlsym(handle, "il2cpp_class_is_interface"));
    IL2CPP::il2cpp_class_is_enum = reinterpret_cast<bool (*)(void*)>(dlsym(handle, "il2cpp_class_is_enum"));

    IL2CPP::il2cpp_type_get_name = reinterpret_cast<const char* (*)(void*)>(dlsym(handle, "il2cpp_type_get_name"));
    IL2CPP::il2cpp_type_is_pointer_type = reinterpret_cast<bool (*)(void*)>(dlsym(handle, "il2cpp_type_is_pointer_type"));
    IL2CPP::il2cpp_type_is_static = reinterpret_cast<bool (*)(void*)>(dlsym(handle, "il2cpp_type_is_static"));

    IL2CPP::il2cpp_runtime_invoke = reinterpret_cast<void* (*)(const void*, void*, void**, void**)>(dlsym(handle, "il2cpp_runtime_invoke"));

    IL2CPP::il2cpp_assembly_get_image = reinterpret_cast<const void* (*)(const void*)>(dlsym(handle, "il2cpp_assembly_get_image"));

    dlclose(handle);
    hasAttached = true;
}