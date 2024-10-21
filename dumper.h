#import <stdio.h>
#include <string>
#include <vector>
#include <sstream>
#import "il2cpp.h"
#import "SCLAlertView/SCLAlertView.h"

static void onIL2CPPDebugFunction(std::string message, IL2CPP::LogLevel level);

namespace Dumper {
    struct UnityField;
    struct UnityMethod;
    struct UnityClass;
    struct UnityAssembly;
}

namespace DumpFormat {
    std::string formatAssembly(Dumper::UnityAssembly assembly);
    std::string formatClass(Dumper::UnityClass klass);
    std::string formatField(Dumper::UnityField field);
    std::string formatMethod(Dumper::UnityMethod method);
}

namespace Dumper {
    struct UnityField {
        std::string name;
        std::string type;

        std::string offset;

        UnityField(std::string _name, std::string _type, std::string _offset) : name(_name), type(_type), offset(_offset) {}
    };
    struct UnityMethod {
        std::string name;
        std::string returnType;
        std::vector<std::string> param_types;
        std::vector<std::string> param_names;
        
        std::string offset;

        UnityMethod(std::string _name, std::string _returnType, std::vector<std::string> _param_types, std::vector<std::string> _param_names, std::string _offset) : name(_name), returnType(_returnType), param_types(_param_types), param_names(_param_names), offset(_offset) {}
    };
    struct UnityClass {
        std::string name;
        std::string namespaze;
        std::string parent;

        bool is_abstract;
        bool is_interface;
        bool is_enum;

        std::vector<UnityField> fieldArray;
        std::vector<UnityMethod> methodArray;
        //std::vector<UnityEvents> eventArray; //TODO: add this when i learn what they do

        UnityClass(std::string _name, std::string _namespaze, std::string _parent, bool _is_abstract, bool _is_interface, bool _is_enum, std::vector<UnityField> _fieldArray, std::vector<UnityMethod> _methodArray) : name(_name), namespaze(_namespaze), parent(_parent), is_abstract(_is_abstract), is_interface(_is_interface), is_enum(_is_enum), fieldArray(_fieldArray), methodArray(_methodArray) {}
    };

    struct UnityAssembly {
        std::string name;
        std::vector<UnityClass> klassArray;

        UnityAssembly(std::string _name, std::vector<UnityClass> _klassArray) : name(_name), klassArray(_klassArray) {}
    };
    
    UnityClass DumpClass(void* klass);

    UnityField DumpField(void* field);

    UnityMethod DumpMethod(void* method);

    UnityAssembly DumpAssembly(IL2CPP::Il2CppAssembly* assembly);

    void DumpPrompt();

    void DumpGame();
};

std::string cleanHex(uintptr_t pointer);
std::string cleanPointer(void* ptr);
void WriteToFile(std::string assemblyName, std::string fileContents);

Dumper::UnityAssembly Dumper::DumpAssembly(IL2CPP::Il2CppAssembly* assembly) {
    std::vector<UnityClass> klassArray;

    const void* image = IL2CPP::il2cpp_assembly_get_image(assembly);

    size_t classCount = IL2CPP::il2cpp_image_get_class_count((void*)image);

    for (int i = 0; i < classCount; i++) {
        void* klass = IL2CPP::il2cpp_image_get_class((void*)image, i);
        klassArray.push_back(Dumper::DumpClass(klass));
    }

    return UnityAssembly(assembly->aname.name, klassArray);
}

Dumper::UnityField Dumper::DumpField(void* field) {
    const char* fieldName = IL2CPP::il2cpp_field_get_name(field);

    const void* fieldType = IL2CPP::il2cpp_field_get_type(field);
    std::string fieldTypeName = IL2CPP::getTypeNameFull((void*)fieldType);

    size_t fieldOffset = IL2CPP::il2cpp_field_get_offset(field);

    return UnityField(fieldName, fieldTypeName, cleanHex((uintptr_t)fieldOffset));
}

Dumper::UnityMethod Dumper::DumpMethod(void* method) {
    const char* methodName = IL2CPP::il2cpp_method_get_name(method);

    uint32_t paramCount = IL2CPP::il2cpp_method_get_param_count(method);

    std::vector<std::string> param_names;
    std::vector<std::string> param_types;
    for (int paramIndex = 0; paramIndex < paramCount; paramIndex++) {
        const void* paramType = IL2CPP::il2cpp_method_get_param(method, paramIndex);
        std::string paramTypeName = IL2CPP::getTypeNameFull((void*)paramType);

        const char* paramName = IL2CPP::il2cpp_method_get_param_name(method, paramIndex);
        param_names.push_back(paramName);
        param_types.push_back(paramTypeName);
    }

    const void* returnType = IL2CPP::il2cpp_method_get_return_type(method);
    std::string returnTypeName = IL2CPP::getTypeNameFull((void*)returnType);

    std::string methodOffset = cleanPointer(method);

    bool isInstance = IL2CPP::il2cpp_method_is_instance(method);

    std::string finalReturnTypeName = returnTypeName;

    if (!isInstance) {
        finalReturnTypeName = "static " + returnTypeName;
    }

    return UnityMethod(methodName, finalReturnTypeName, param_types, param_names, methodOffset);
}

Dumper::UnityClass Dumper::DumpClass(void* klass) {
    std::vector<UnityField> fieldArray;
    std::vector<UnityMethod> methodArray;

    const char* klassName = IL2CPP::il2cpp_class_get_name(klass);

    const char* namespazeName = IL2CPP::il2cpp_class_get_namespace(klass);

    const void* parentKlass = IL2CPP::il2cpp_class_get_parent(klass);
    const char* parentKlassName = "";
    if (parentKlass != NULL) {
        parentKlassName = IL2CPP::il2cpp_class_get_name((void*)parentKlass);
    }

    bool is_abstract = IL2CPP::il2cpp_class_is_abstract(klass);
    bool is_interface = IL2CPP::il2cpp_class_is_interface(klass);
    bool is_enum = IL2CPP::il2cpp_class_is_enum(klass);

    void* methodItor = nullptr;
    bool shouldStopMethodLoop = false;

    for (int i = 0; !shouldStopMethodLoop; i++) {
        const void* method = IL2CPP::il2cpp_class_get_methods(klass, &methodItor);
        if (method == NULL) {
            shouldStopMethodLoop = true;
            continue;
        }
        methodArray.push_back(Dumper::DumpMethod((void*)method));
    }

    void* fieldItor = nullptr;
    bool shouldStopFieldLoop = false;

    for (int i = 0; !shouldStopFieldLoop; i++) {
        const void* field = IL2CPP::il2cpp_class_get_fields(klass, &fieldItor);
        if (field == NULL) {
            shouldStopFieldLoop = true;
            continue;
        }
        fieldArray.push_back(Dumper::DumpField((void*)field));
    }

    return UnityClass(klassName, namespazeName, parentKlassName,is_abstract, is_interface, is_enum, fieldArray, methodArray);
}

uint64_t baseAddress;
std::vector<Dumper::UnityAssembly> dumpedAssemblyCache;
void Dumper::DumpPrompt() {
    IL2CPP::Attach(onIL2CPPDebugFunction);
    baseAddress = IL2CPP::findBaseAddress();

    SCLAlertView *alert = [[SCLAlertView alloc] initWithNewWindow];
    [alert addButton:@"Start Dump" actionBlock:^(void) {
        Dumper::DumpGame();  
    }];

    [alert showSuccess:@"Attach Success" subTitle:@"Runtime Dumper has attached correctly" closeButtonTitle:@"Cancel" duration:0.0f];

    //removed cause of crash
    //std::string unityVersion = IL2CPP::getUnityVersion();
    //std::string message = "Runtime Dumper has attached correctly, Unity Version: " + unityVersion;
    //[alert showSuccess:@"Attach Success" subTitle:[NSString stringWithUTF8String:message.c_str()] closeButtonTitle:@"Cancel" duration:0.0f];
}

void Dumper::DumpGame() {
    if (!IL2CPP::canAttach()) {
        SCLAlertView *alert = [[SCLAlertView alloc] initWithNewWindow];
        //alert that you cant attach
        [alert addButton:@"Try Again" actionBlock:^(void) {
            Dumper::DumpGame();  
        }];
        [alert showError:@"Attach Error" subTitle:@"Runtime Dumper was unable to attach to unity" closeButtonTitle:@"Cancel Dump" duration:0.0f];

    }
    dumpedAssemblyCache.clear();

    void* domain = IL2CPP::il2cpp_domain_get();

    size_t assemblySize;

    IL2CPP::Il2CppAssembly** assemblies = IL2CPP::il2cpp_domain_get_assemblies(domain, &assemblySize);

    for (int i = 0; i < assemblySize; i++) {
        UnityAssembly dumpedAssembly = Dumper::DumpAssembly(assemblies[i]);
        dumpedAssemblyCache.push_back(dumpedAssembly);
    }

    SCLAlertView *alert = [[SCLAlertView alloc] initWithNewWindow];
    [alert addButton:@"Export To Documents" actionBlock:^(void) {
        for (int i = 0; i < dumpedAssemblyCache.size(); i++) {
            Dumper::UnityAssembly dumpedAssembly = dumpedAssemblyCache[i];
            std::string assemblyOut = DumpFormat::formatAssembly(dumpedAssembly);
            WriteToFile(dumpedAssembly.name, assemblyOut);
        } 
    }];
    //TODO: add zip lib so i can post data to a webserver
    [alert showSuccess:@"Dump Done" subTitle:@"Check Export Options" closeButtonTitle:nil duration:0.0f];

}

//maybe actually do something with this? not being used currently
static void onIL2CPPDebugFunction(std::string message, IL2CPP::LogLevel level) {
    SCLAlertView *alert = [[SCLAlertView alloc] initWithNewWindow];
    //TODO: add copy error button
    /*[alert addButton:@"Copy Error" actionBlock:^(void) {
         
    }];*/
    std::string errorTitle = "IL2CPP Debug: " + std::to_string(level);
    [alert showError:[NSString stringWithUTF8String:errorTitle.c_str()] subTitle:[NSString stringWithUTF8String:message.c_str()] closeButtonTitle:@"Ok" duration:0.0f];
}

std::string DumpFormat::formatAssembly(Dumper::UnityAssembly assembly) {
    std::string returnString = "";

    for (int i = 0; i < assembly.klassArray.size(); i++) {
        Dumper::UnityClass klass = assembly.klassArray[i];
        returnString += DumpFormat::formatClass(klass);
    }

    return returnString;
}
//maybe make this stuff better? idk im lazy
std::string DumpFormat::formatClass(Dumper::UnityClass klass) {
    std::string returnString = "//Namespace: " + klass.namespaze + "\n";

    std::string classType = "public class";
    if (klass.is_abstract) {
        classType = "public abstract class";
    } else if (klass.is_interface) {
        classType = "public interface";
    } else if (klass.is_enum) {
        classType = "public enum";
    }

    returnString += classType + " " + klass.name;
    if (klass.parent != "") {
        returnString += " : " + klass.parent + "\n";
    } else {
        returnString += "\n";
    }

    returnString += "{\n";

    if (klass.fieldArray.size() > 0) {
        returnString += "    //Fields:\n\n";
    }

    for (int i = 0; i < klass.fieldArray.size(); i++) {
        Dumper::UnityField field = klass.fieldArray[i];
        returnString += "    " + DumpFormat::formatField(field) + "\n";
    }

    if (klass.methodArray.size() > 0) {
        returnString += "\n    //Methods:\n\n";
    }

    for (int i = 0; i < klass.methodArray.size(); i++) {
        Dumper::UnityMethod field = klass.methodArray[i];
        returnString += "    " + DumpFormat::formatMethod(field) + "\n";
    }

    returnString += "}\n\n";

    return returnString;
}

std::string DumpFormat::formatField(Dumper::UnityField field) {
    std::string returnString = "";
    returnString += field.type + " " + field.name + "; //Offset: " + field.offset;

    return returnString;
}

std::string DumpFormat::formatMethod(Dumper::UnityMethod method) {
    std::string returnString = "//Offset: " + method.offset + "\n";

    returnString += "    " + method.returnType + " " + method.name + "(";

    for (int i = 0; i < method.param_types.size(); i++) {
        returnString += method.param_types[i] + " " + method.param_names[i] + ", ";
    }

    //maybe a better way to do this?
    if (method.param_types.size() > 0) {
        returnString.pop_back(); //remove trailing " "
        returnString.pop_back(); //remove trailing ","
    }

    returnString += ");";

    return returnString;
}

//straight up chatgpt code because i didnt know how to write a file
void WriteToFile(std::string assemblyName, std::string fileContents) {
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths firstObject];

    // Create a dumpOut directory path
    NSString *dumpOutDirectory = [documentsDirectory stringByAppendingPathComponent:@"DumpedModules"];
    
    // Create the directory if it doesn't exist
    [[NSFileManager defaultManager] createDirectoryAtPath:dumpOutDirectory withIntermediateDirectories:YES attributes:nil error:nil];

    // Create a file path
    NSString *filePath = [dumpOutDirectory stringByAppendingPathComponent:[NSString stringWithUTF8String:(assemblyName + ".cs").c_str()]];

    // Write the content to the file
    NSError *error = nil;
    BOOL success = [[NSString stringWithUTF8String:fileContents.c_str()] writeToFile:filePath atomically:YES encoding:NSUTF8StringEncoding error:&error];

    if (!success) {
        NSLog(@"Error writing file: %@", error.localizedDescription);
    }
}

std::string cleanHex(uintptr_t pointer) {
    std::ostringstream oss;
    oss << "0x" << std::hex << pointer;
    return oss.str();
}

std::string cleanPointer(void* ptr) {
    uintptr_t finalOffset = reinterpret_cast<uintptr_t>(ptr) - (uintptr_t)baseAddress;

    return cleanHex(finalOffset);
}