#include "ByteCodeDec.h"


int main(int _argc, char **_argv)
{
	lua_State *L = lua_open();

	switch (_argc)
	{
	case 2:
	{
		if (!std::filesystem::exists(_argv[1]))
		{
			std::cout << "Invalid input path." << std::endl;
			break;
		}
		if (std::filesystem::is_directory(_argv[1]))
		{
			std::filesystem::path outdir(_argv[1]);
			outdir.append("dec");
			DecDirectory(L, _argv[1], outdir.string().c_str());
		}
		else
		{
			std::filesystem::path outdir(_argv[1]);
			outdir = outdir.parent_path().append("dec");
			DecSingle(L, _argv[1], outdir.string().c_str());
		}

		break;
	}
	case 3:
	{
		if (!std::filesystem::exists(_argv[1]))
		{
			std::cout << "Invalid input path." << std::endl;
			break;
		}

		if (std::filesystem::is_directory(_argv[1]))
		{
			DecDirectory(L, _argv[1], _argv[2]);
		}
		else
		{
			DecSingle(L, _argv[1], _argv[2]);
		}
		break;
	}

	default:
	{
		std::cout << R"(Usage: bcDec "InputFilePath/InputDir" ["OutputDir"])" << std::endl;
	}
	break;
	}

	lua_close(L);
	return 0;
}