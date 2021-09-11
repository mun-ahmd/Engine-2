#pragma once
#include <string>
#include <vector>
#include <fstream>

class BinaryFileUtility
{
private:
	std::fstream file;
public:
	BinaryFileUtility() = default;
	BinaryFileUtility(const char* filedir)
	{
		file.open(filedir, std::ios::in | std::ios::out | std::ios::binary);
	}
	inline void open(std::string filedir)
	{
		file.open(filedir, std::ios::in | std::ios::out | std::ios::binary);
	}
	inline void close()
	{
		file.close();
	}
	//no need for destructor, fstream closes in its own destructor
	template<class T>
	inline void read(T* dest)
	{
		file.read((char*)dest, sizeof(T));
	}
	template<class T>
	inline void read(T* dest, size_t count)
	{
		file.read((char*)dest, sizeof(T) * count);
	}
	template <class T>
	void read(std::vector<T>& dest, size_t count)
	{
		T* data = new T[count];
		file.read((char*)data, sizeof(T) * count);
		dest.insert(data, count);
		delete[] data;
	}
	template<class T>
	inline void write(T source)
	{
		file.write((const char*)&source, sizeof(T));
	}
	template <class T>
	inline void write(T* source, size_t count)
	{
		file.write((const char*)source, sizeof(T) * count);
	}
	template <class T>
	void write(std::vector<T>& source, size_t count)
	{
		file.write((const char*)source.data(), sizeof(T) * count);
	}
};