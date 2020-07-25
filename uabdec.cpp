#include "AssetDecoder.h"
#include <iostream>
#include <filesystem>
#include <string>
#include <fstream>


void DoDecode(const char* _InputFileName, const char* _OutputFileName)
{
	std::ifstream ifs;
	ifs.open(_InputFileName, std::ios::binary);
	ifs.seekg(0, std::ios::end);
	size_t sz = (size_t)ifs.tellg();
	ifs.seekg(0, std::ios::beg);

	FByteArray* gcbuffer = (FByteArray*)NewSpecific(sizeof(char), sz);
	ifs.read(gcbuffer->m_Items, sz);
	ifs.close();

	FByteArray* out_assert = DigitalSea_Scipio(gcbuffer);

	std::ofstream ofs;
	ofs.open(_OutputFileName, std::ios::binary | std::ios::trunc);
	ofs.write(out_assert->m_Items, out_assert->max_length);
	ofs.close();
}


int main(int _argc, char **_argv)
{
	switch (_argc)
	{
	case 2:
	{
		std::filesystem::path InputFilePath(_argv[1]);
		std::string filename = InputFilePath.filename().stem().string();
		filename.append("_dec");
		filename.append(InputFilePath.filename().extension().string());

		std::string OutPath = InputFilePath.parent_path().append(filename).string().c_str();
		DoDecode(_argv[1], OutPath.c_str());
		break;
	}
	case 3:
	{
		DoDecode(_argv[1], _argv[2]);
		break;
	}
	default:
	{
		std::cout << R"(Usage: uabDec "input_file_path" ["output_file_path"] )" << std::endl;
		break;
	}
	}

	return 0;
}
