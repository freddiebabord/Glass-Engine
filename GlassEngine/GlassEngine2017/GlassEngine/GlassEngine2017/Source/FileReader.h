#pragma once

namespace GlassEngine
{
	static std::vector<char> ReadFileToCharVector(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open())
			throw std::runtime_error("Failed to open file!");

		size_t fileSize = size_t(file.tellg());
		std::vector<char> buffer(fileSize);
		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}

	static const char *mmap_file(size_t *len, const char* filename)
	{
		(*len) = 0;
		HANDLE file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
		assert(file != INVALID_HANDLE_VALUE);

		HANDLE fileMapping = CreateFileMapping(file, NULL, PAGE_READONLY, 0, 0, NULL);
		assert(fileMapping != INVALID_HANDLE_VALUE);

		LPVOID fileMapView = MapViewOfFile(fileMapping, FILE_MAP_READ, 0, 0, 0);
		auto fileMapViewChar = (const char*)fileMapView;
		assert(fileMapView != NULL);

		LARGE_INTEGER fileSize;
		fileSize.QuadPart = 0;
		GetFileSizeEx(file, &fileSize);

		(*len) = static_cast<size_t>(fileSize.QuadPart);
		return fileMapViewChar;

	}

	static const char* GetFileDataAndLength(size_t *len, const char* filename)
	{

		const char *ext = strrchr(filename, '.');

		size_t data_len = 0;
		const char* data = mmap_file(&data_len, filename);

		(*len) = data_len;
		return data;
	}

}
