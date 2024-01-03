#include "pch.h"
#include "Native.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <random>
#include <climits>
#include <sstream>
#include <direct.h>
using namespace std;

static random_device s_seed; //硬件生成随机种子
static ranlux48 s_randomEngine(s_seed()); //利用种子生成随机数引擎
static uniform_int_distribution<unsigned int> s_uint_distrib(UINT_MAX / 2, UINT_MAX);
static LogCallback s_logCallback = NULL;
static char version[] = "1.0.0";

unsigned int get_random_uint() {
	return s_uint_distrib(s_randomEngine);
}


char* encrtypt_file(char* src, size_t& file_size) {

	//随机密钥长度
	uniform_int_distribution<int> key_distrib(130, 140);
	int kl = key_distrib(s_randomEngine);

	//随机密钥的数组指针
	unsigned int* p_passwordArr = new unsigned int[kl];
	for (int i = 0; i < kl; i++)
	{
		p_passwordArr[i] = get_random_uint();
	}

	//加密区长度 单位：int指针
	int klsize = (kl + 1) * sizeof(uint32_t);
	const int safe_size = 1024;//安全区大小
	//加密区大小
	const size_t encrtypt_size = file_size - safe_size;
	//申请一个新的内存卡 它将包含密码和源文件
	char* des = (char*)malloc(file_size + klsize);
	//将安全区代码Cpy到新的内存块
	memcpy(des, src, safe_size);

	//密文区指针
	unsigned int* da = (unsigned int*)(des + safe_size);
	//加密源码区指针
	unsigned int* db = (unsigned int*)(src + safe_size);
	//密文区首四个字节低十六位为加密密文数组长度
	*(da++) = (get_random_uint() & 0xFFFF0000) | (kl & 0xFFFF);
	//写入密码组
	memcpy(da, p_passwordArr, kl * sizeof(uint32_t));
	//指向加密区
	da += kl;

	for (size_t i = 0; i < encrtypt_size; i += 4) {
		int index = (i + (i / kl)) % kl;
		da[i / 4] = p_passwordArr[index] ^ db[i / 4];
	}

	file_size += klsize;
	delete[] p_passwordArr;
	return des;
}


int get_little_endian(unsigned int x) {
	return ((x >> 24) & 0xff) | ((x << 8) & 0xff0000) | ((x >> 8) & 0xff00) | ((x << 24) & 0xff000000);
}


bool file_exist(const char* path) {
	struct stat _Stat;
	if (stat(path, &_Stat) != 0) {

		return false;
	}
	return true;
}


void Log(const char* fmt, ...) {
	if (s_logCallback == NULL)return;
	char acLogStr[512];// = { 0 };
	va_list ap;
	va_start(ap, fmt);
	vsprintf(acLogStr, fmt, ap);
	va_end(ap);
	s_logCallback(acLogStr, strlen(acLogStr));
}

void CopyTo(const char* s, const char* d) {
	std::ifstream  src(s, std::ios::binary);
	std::ofstream  dst(d, std::ios::binary);
	dst << src.rdbuf();
}

void OverrideLoader(char* path) {
	char* workpath = _getcwd(NULL, 0);
	Log(workpath);

	if (NULL == path || strcmp(path, "") == 0) {
		free(workpath);
		workpath = NULL;
		return;
	}

	string localpath = workpath;
	string new_loaderCpp = localpath + "\\Assets/EasyObfuscation/Editor/local/vm_MetadataLoader.cpp";
	string new_memoryMappedFileH = localpath + "\\Assets/EasyObfuscation/Editor/local/utils_MemoryMappedFile.h";
	string new_memoryMappedFileCpp = localpath + "\\Assets/EasyObfuscation/Editor/local/utils_MemoryMappedFile.cpp";

	if (!file_exist(new_loaderCpp.c_str()))
	{
		Log("miss: %s", new_loaderCpp.c_str());
		return;
	}
	if (!file_exist(new_memoryMappedFileH.c_str()))
	{
		Log("miss: %s", new_memoryMappedFileH.c_str());
		return;
	}
	if (!file_exist(new_memoryMappedFileCpp.c_str()))
	{
		Log("miss: %s", new_memoryMappedFileCpp.c_str());
		return;
	}

	string dest = path;
	string dest_loaderCpp = dest + "\\unityLibrary\\src\\main\\Il2CppOutputProject\\IL2CPP\\libil2cpp\\vm\\MetadataLoader.cpp";
	string dest_memoryMappedFileH = dest + "\\unityLibrary\\src\\main\\Il2CppOutputProject\\IL2CPP\\libil2cpp\\utils\\MemoryMappedFile.h";
	string dest_memoryMappedFileCpp = dest + "\\unityLibrary\\src\\main\\Il2CppOutputProject\\IL2CPP\\libil2cpp\\utils\\MemoryMappedFile.cpp";

	if (!file_exist(dest_loaderCpp.c_str()))
	{
		Log("error: not found dest loaderCpp");
		return;
	}
	if (!file_exist(dest_memoryMappedFileH.c_str()))
	{
		Log("error: not found dest memoryMapped head file");
		return;
	}
	if (!file_exist(dest_memoryMappedFileCpp.c_str()))
	{
		Log("error: not found dest memoryMapped source file");
		return;
	}

	CopyTo(new_loaderCpp.c_str(), dest_loaderCpp.c_str());
	CopyTo(new_memoryMappedFileCpp.c_str(), dest_memoryMappedFileCpp.c_str());
	CopyTo(new_memoryMappedFileH.c_str(), dest_memoryMappedFileH.c_str());

	if (workpath != NULL) {
		free(workpath);
		workpath = NULL;
	}

	Log("override Loader code complete.");
}

void EncryptionCode(char* export_android_path)
{
	//wait input
	string global_metadata_path = export_android_path;
	if (NULL == strstr(global_metadata_path.c_str(), "global-metadata.dat")) {
		global_metadata_path += "\\unityLibrary\\src\\main\\assets\\bin\\Data\\Managed\\Metadata\\global-metadata.dat";
	}

	//cheack file vaild
	if (!file_exist(global_metadata_path.c_str())) {
		Log("file not found: %s\n", global_metadata_path);
		return;
	}

	Log("EasyObfuscation version: %s", version);

	//load file
	ifstream infile(global_metadata_path, ios::in | ios::binary | ios::ate);
	size_t size = infile.tellg();
	infile.seekg(0, ios::beg);
	char* buffer = new char[size];
	infile.read(buffer, size);
	infile.close();

	//encrtypt
	size_t srcsize = size;
	char* encbuffer = encrtypt_file(buffer, size);
	ofstream outfile(global_metadata_path, ios::out | ios::binary | ios::ate);
	if (!outfile) {
		Log("open file fail: %s\n", global_metadata_path);
		return;
	}

	//log
	unsigned int* hex_buffer = (unsigned int*)buffer;
	unsigned int* hex_encbuffer = (unsigned int*)encbuffer;
	unsigned int src_value = get_little_endian(*hex_buffer);
	unsigned int enc_value = get_little_endian(*hex_encbuffer);
	Log("src: %x\tsrc buffer size: %ld\nenc: %x\tenc buffer size: %ld", src_value, srcsize, enc_value, size);

	outfile.write(encbuffer, size);
	outfile.close();
	delete[] buffer;
	free(encbuffer);

	Log("call cpp complete.");
}

void SetDisplayLog(LogCallback callback)
{
	s_logCallback = callback;
}

