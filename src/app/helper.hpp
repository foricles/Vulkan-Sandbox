#pragma once
#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>

namespace helpers
{
	inline std::vector<uint8_t> sb_read_file(const std::filesystem::path& filepath, bool isBin = false)
	{
		std::streampos fileSize;
		std::vector<uint8_t> arFileData;
		std::ifstream sfile(filepath, isBin ? std::ios::binary : 0);

		if (sfile.is_open())
		{
			sfile.seekg(0, std::ios::end);
			fileSize = sfile.tellg();
			sfile.seekg(0, std::ios::beg);

			arFileData.resize(static_cast<uint32_t>(fileSize) + static_cast<int>(!isBin));
			sfile.read(reinterpret_cast<char*>(&arFileData[0]), fileSize);
			if (!isBin)
			{
				arFileData.back() = '\0';
			}
		}
		else
		{
			std::cout << "\tError: Could not open file: " << filepath << std::endl;
		}

		return arFileData;
	}
}

#define BIT(x) 1 << x