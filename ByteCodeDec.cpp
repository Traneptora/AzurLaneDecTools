#include "ByteCodeDec.h"

int writer_buf(lua_State *L, const void *p, size_t size, void *sb)
{
	lj_buf_putmem((SBuf *)sb, p, (MSize)size);
	UNUSED(L);
	return 0;
}

bool DecodeByteCode(lua_State* L, const char* _BCFilePath, const char* _OutputFilePath)
{
	bool bSuccess = false;
	size_t sz = 0;
	const char* decbuf = nullptr;
	std::ofstream ofs;

	luaL_loadfile(L, _BCFilePath);

	GCfunc *fn = lj_lib_checkfunc(L, 1);
	static int strip = 1;
	SBuf *sb = lj_buf_tmp_(L);
	L->top = L->base + 1;
	if (!isluafunc(fn) || lj_bcwrite(L, funcproto(fn), writer_buf, sb, strip))
	{
		lj_err_caller(L, LJ_ERR_STRDUMP);
		bSuccess = false;
		goto exit;
	}

	setstrV(L, L->top - 1, lj_buf_str(L, sb));
	lj_gc_check(L);

	decbuf = lua_tolstring(L, -1, &sz);
	ofs.open(_OutputFilePath, std::ios::binary | std::ios::trunc);
	ofs.write(decbuf, sz);
	ofs.close();
	bSuccess = true;

exit:
	lua_settop(L, 0);
	return bSuccess;
}

void DecFileTo_Impl(lua_State* L, const std::filesystem::path& _Inpath, const char* _OutputDir)
{
	std::filesystem::path OutFileName(_OutputDir);
	OutFileName.append(_Inpath.filename().stem().string());
	std::string OutPath(OutFileName.string().c_str());
	OutPath.append(".lj");
	//std::cout << _FilePath.string() << std::endl;
	//std::cout << OutPath << std::endl;
	DecodeByteCode(L, _Inpath.string().c_str(), OutPath.c_str());
}

void DecSingle(lua_State* L, const char* _InputFilePath, const char* _OutputDir)
{
	std::filesystem::path inpath(_InputFilePath);
	std::filesystem::create_directories(_OutputDir);
	DecFileTo_Impl(L, inpath, _OutputDir);
}

void DecDirectory(lua_State* L, const char* _InputDir, const char* _OutputDir)
{
	std::filesystem::create_directories(_OutputDir);
	std::filesystem::directory_iterator dir(_InputDir);
	std::filesystem::directory_iterator dir_end;

	for (; dir != dir_end; ++dir)
	{
		if (dir->status().type() == std::filesystem::file_type::regular)
		{
			DecFileTo_Impl(L, dir->path(), _OutputDir);
		}
	}
}


