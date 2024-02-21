#include <fstream>
#include <string>

namespace File {

	bool Contain(const char* path, const char* dest) {
		std::ifstream infile(path, std::ios::in | std::ios::binary | std::ios::ate);
		size_t size = infile.tellg();
		infile.seekg(0, std::ios::beg);
		char* buffer = new char[size];
		infile.read(buffer, size);
		infile.close();
		std::string alltext(buffer, size);
		delete[] buffer;
		return alltext.find(dest) != std::string::npos;
	}

	char* ReadBytes(const char* path) {
		std::ifstream infile(path, std::ios::in | std::ios::binary | std::ios::ate);
		size_t size = infile.tellg();
		infile.seekg(0, std::ios::beg);
		char* buffer = new char[size];
		infile.read(buffer, size);
		infile.close();
		return buffer;
	}

	std::string Extract(const char* path, size_t offset, size_t length) {
		char* buffer = ReadBytes(path);
		if (buffer) {
			std::string a(buffer + offset, length);
			delete[] buffer;
			return a;
		}
		delete[] buffer;
		return nullptr;
	}

}