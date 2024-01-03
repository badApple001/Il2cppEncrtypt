#pragma once
#ifdef SAMPLE_EXPORTS
#define DLL_CALL __declspec(dllexport)
#else
#define DLL_CALL __declspec(dllimport)
#endif

typedef void (*LogCallback)(char* message, int iSize);


extern "C" DLL_CALL void OverrideLoader(char* path);

extern "C" DLL_CALL void EncryptionCode(char* path);

extern "C" DLL_CALL void SetDisplayLog(LogCallback logCall);



