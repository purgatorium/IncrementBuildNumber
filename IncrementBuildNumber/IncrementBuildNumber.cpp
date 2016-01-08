#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <shellapi.h>
#include <io.h>
#include "StdString.h"
#include "Token.hpp"



class ArgvHelper
{
private:
	int argc;
	wchar_t **argv;

public:
	ArgvHelper(wchar_t *cmdline)
	{
		argv = ::CommandLineToArgvW(cmdline, &argc);
	}

	~ArgvHelper()
	{
		::LocalFree(argv);
	}

	int Count()
	{
		return argc;
	}

	CStdStringW Get(size_t idx)
	{
		if (idx <= static_cast<size_t>(argc))
		{
			return argv[idx];
		}

		return L"";
	}
};

class AutoCloseHandle
{
private:
	HANDLE handle;

public:
	AutoCloseHandle(HANDLE h)
	{
		handle = h;
	}

	~AutoCloseHandle()
	{
		if (NULL != handle && INVALID_HANDLE_VALUE != handle)
			CloseHandle(handle);
	}

	HANDLE Get()
	{
		return handle;
	}
};



CStdStringA UnicodeToUTF8(const wchar_t* source, long source_len = -1)
{
	int len = WideCharToMultiByte(CP_UTF8, 0, source, source_len, NULL, 0, NULL, NULL);
	if (len == 0)
		return "";

	CStdStringA result;
	WideCharToMultiByte(CP_UTF8, 0, source, source_len, result.GetBufferSetLength(len+1), len, NULL, NULL);
	result.at(len) = '\0';
	result.ReleaseBuffer();
	return result;
}

CStdStringW UTF8ToUnicode(const char* source, long source_len = -1)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, source, source_len, NULL, 0);
	if (0 == len)
		return L"";

	CStdStringW result;
	MultiByteToWideChar(CP_UTF8, 0, source, source_len, result.GetBufferSetLength(len+1), len);
	result.at(len) = L'\0';
	result.ReleaseBuffer();
	return result;
}




// "1.0.0.1" �� ó��
void _ReplaceVersionString(CStdStringW &line, int begin)
{
	// a,b,c,d")]
	CStdStringW replace_src = line.substr(begin);

	// d
	CStdStringW build_number_str = replace_src.substr(replace_src.ReverseFind(L".") + 1);
	build_number_str.Replace(L"\")]", NULL);
	//::MessageBox(NULL, _T("[") + CStdString(build_number_str) + _T("]"), _T("�ȳ�"), MB_ICONINFORMATION);

	// a,b,c,
	CStdStringW replace_dst = replace_src.substr(0, replace_src.ReverseFind(L".") + 1);
	//::MessageBox(NULL, _T("[") + CStdString(replace_dst) + _T("]"), _T("�ȳ�"), MB_ICONINFORMATION);

	// d + 1
	int build_number = _wtoi(build_number_str) + 1;

	// a,b,c,d+1
	wchar_t tmp[20];
	if (0 != _itow_s(build_number, tmp, 20, 10))
		return;

	replace_dst += tmp;
	replace_dst += L"\")]";
	//::MessageBox(NULL, _T("[") + CStdString(replace_src) + _T("]"), _T("�ȳ�"), MB_ICONINFORMATION);
	//::MessageBox(NULL, _T("[") + CStdString(replace_dst) + _T("]"), _T("�ȳ�"), MB_ICONINFORMATION);
	line.Replace(replace_src, replace_dst);
}

// Properties ���丮�� AssemblyInfo.cs ���Ͽ��� ���������� �����Ѵ�.
void ProcessCSharpProject(CStdString &csharp_target)
{
	using namespace peter;

	// ���� �����
	AutoCloseHandle fp(CreateFile(csharp_target, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
	if (INVALID_HANDLE_VALUE == fp.Get())
	{
		::MessageBox(NULL, _T("���� ���� ����\r\n") + csharp_target, _T("�ȳ�"), MB_ICONINFORMATION);
		return;
	}

	// ���� ��°�� �а�
	CStdStringA real_file_buffer;
	DWORD size = 0;
	if (FALSE == ReadFile(fp.Get(), real_file_buffer.GetBufferSetLength(10240), 10240, &size, NULL))
		return;


	// 151223 peter: BOM�� ��� �߰��Ǵ°� ���ݲ� �𸣰� �־���...
	// ���� ��ȣ�� 1200�� ������Ʈ�� �ִµ� AssemblyInfo.cs ������ �̻��ϰ� �뷮�� Ŀ�� Ȯ���غ��� �̷� ������ �־���
	// EFBBBFEFBBBFEFBBBFEFBBBFEFBBBFEFBBBFEFBBBFEFBBBFEFBBBFEFBBBFEFBBBFEFBBBFEFBBBFEFBBBFEFBBBFEFBBBFEFBBBF...
	char BOM[3] = { 0xEF, 0xBB, 0xBF };
	// BOM�� ���� �����Ѵ�.
	while (0 == memcmp(real_file_buffer.GetBuffer(), BOM, 3))
	{
		memmove_s(real_file_buffer.GetBuffer(), 10240, real_file_buffer.GetBuffer() + 3, 10240 - 3);
	}
	real_file_buffer.ReleaseBuffer();


	// ������ UTF8�̹Ƿ� UTF16���� �����ϰ� �۾�
	CStdStringW buffer(UTF8ToUnicode(real_file_buffer.c_str(), real_file_buffer.size()));

	// 160106 peter: ���� ��������� ���̴� ���� ���� ����� ������ �����ش޶�� ��û�� �־� �ΰ��� �������� ��� �����ϰ� ����
	Token<wchar_t> token(L"\r\n");
	token.Tokenize(buffer);
	buffer.clear();

	// ���پ� ó���Ѵ�.
	for (int i = 0; i < token.GetCount(); ++i)
	{
		CStdStringW line(token.GetToken(i));
		// ��// [assembly: AssemblyVersion("1.0.*")]�� �κ��� �Ѿ�� ����
		if (0 == wcsncmp(line, L"//", 2))
		{
			// ���� �״�� ����
			buffer += line + "\r\n";
			continue;
		}

		// [assembly: AssemblyVersion("a.b.c.d")] ����
		int begin = line.Find(L"AssemblyVersion(\"");
		if (0 <= begin)
		{
			_ReplaceVersionString(line, begin);

			// ������ ���� ����
			buffer += line + "\r\n";
			continue;
		}

		// [assembly: AssemblyFileVersion("a.b.c.d")] ����
		begin = line.Find(L"AssemblyFileVersion(\"");
		if (0 <= begin)
		{
			_ReplaceVersionString(line, begin);

			// ������ ���� ����
			buffer += line + "\r\n";
			continue;
		}

		// ���� �״�� ����
		buffer += line + "\r\n";
	}


	// ���� ���� �����Ҳ���. seek �ȵǸ� �۾� �Ұ�
	if (INVALID_SET_FILE_POINTER == SetFilePointer(fp.Get(), 0, NULL, FILE_BEGIN))
		return;

	// �ϴ� UTF8 ��� ����
	WriteFile(fp.Get(), BOM, 3, &size, NULL);

	real_file_buffer = UnicodeToUTF8(buffer.c_str(), buffer.size());
	WriteFile(fp.Get(), real_file_buffer.c_str(), real_file_buffer.size(), &size, NULL);

	SetEndOfFile(fp.Get());
}



// ���ҽ������� �Ѱ��� �ִ°��� Ȱ��, ���������� �����Ѵ�.
class CPlusPlusProject
{
private:
	CStdString target_directory;
	CStdString target_file;
	CStdStringA bom;

public:
	bool FindResourceFile()
	{
		target_file = target_directory;
		CStdString find_string = target_directory + _T("*.rc");
		struct _tfinddatai64_t filedata;
		__int64 finddata = _tfindfirsti64(find_string, &filedata);

		if (-1 != finddata)
		{
			do 
			{
				if (_A_SUBDIR == (filedata.attrib & _A_SUBDIR))
				{
					continue;
				}
				else
				{
					target_file += filedata.name;
					_findclose(finddata);
					return true;
				}
			} while (-1 != _tfindnexti64(finddata, &filedata));
		}

		_findclose(finddata);
		return false;
	}

	void ProcessCPlusPlusProject(CStdString cplusplus_target)
	{
		target_directory = cplusplus_target;
		if (!FindResourceFile())
		{
			::MessageBox(NULL, _T("C++ ������Ʈ�� �ƴմϴ�.\r\n") + target_file, _T("�ȳ�"), MB_ICONINFORMATION);
			return;
		}

		// ���� �����
		AutoCloseHandle fp(CreateFile(target_file, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
		if (INVALID_HANDLE_VALUE == fp.Get())
		{
			::MessageBox(NULL, _T("���� ���� ����\r\n") + target_file, _T("�ȳ�"), MB_ICONINFORMATION);
			return;
		}

		// BOM�� �о �Ϲ��������� UTF16�������� Ȯ���Ѵ�.
		DWORD size = 0;
		if (FALSE == ReadFile(fp.Get(), bom.GetBufferSetLength(2), 2, &size, NULL))
		{
			::MessageBox(NULL, _T("���� �б� ����\r\n") + target_file, _T("�ȳ�"), MB_ICONINFORMATION);
			return;
		}

		if ( ((char)0xFF == bom.at(0) && (char)0xFE == bom.at(1)) || ((char)0xFE == bom.at(0) && (char)0xFF == bom.at(1)) )
		{
			// UTF16 ������ ��� ó��
			UTF16(fp.Get());
		}
		else
		{
			// MBCS ������ ��� ó��
			MBCS(fp.Get());
		}
	}

	void UTF16(HANDLE fp)
	{
		// UTF16: ���� ��°�� �а�
		CStdStringW buffer;
		DWORD size = 0;
		if (FALSE == ReadFile(fp, buffer.GetBufferSetLength(10240), 10240, &size, NULL))
			return;
		// 160106 peter: ���⵵ ���������� FEFFFEFFFEFFFEFFFEFFFEFF......... �� ó���ɵ�. �׷��� ����
		while (0 == memcmp(buffer.GetBuffer(), bom.c_str(), 2))
		{
			memmove_s(buffer.GetBuffer(), 10240, buffer.GetBuffer() + 2, 10240 - 2);
		}
		buffer.ReleaseBuffer();

		// �������� ���� ��
		if (!ReplaceVersionNumber(buffer, L"FILEVERSION"))
			return;
		if (!ReplaceVersionNumber(buffer, L"PRODUCTVERSION"))
			return;
		if (!ReplaceVersionString(buffer, L"\"FileVersion\""))
			return;
		if (!ReplaceVersionString(buffer, L"\"ProductVersion\""))
			return;

		//::MessageBox(NULL, CStdString(buffer), _T("�ȳ�"), MB_ICONINFORMATION);
		if (INVALID_SET_FILE_POINTER != SetFilePointer(fp, 0, NULL, FILE_BEGIN))
		{
			// UTF16: ���� ��°�� �����Ѵ�.
			WriteFile(fp, bom.c_str(), bom.size(), &size, NULL); // BOM �����ؾ���
			WriteFile(fp, buffer.c_str(), buffer.size() * sizeof(wchar_t), &size, NULL);
			SetEndOfFile(fp);
		}
	}

	void MBCS(HANDLE fp)
	{
		// MBCS: ���� ��°�� �а�
		CStdStringA buffer;
		DWORD size = 0;
		if (FALSE == ReadFile(fp, buffer.GetBufferSetLength(10240), 10240, &size, NULL))
			return;
		buffer.ReleaseBuffer();

		// �������� ���� ��
		if (!ReplaceVersionNumber(buffer, "FILEVERSION"))
			return;
		if (!ReplaceVersionNumber(buffer, "PRODUCTVERSION"))
			return;
		if (!ReplaceVersionString(buffer, "\"FileVersion\""))
			return;
		if (!ReplaceVersionString(buffer, "\"ProductVersion\""))
			return;

		//::MessageBox(NULL, CStdString(buffer), _T("�ȳ�"), MB_ICONINFORMATION);
		if (INVALID_SET_FILE_POINTER != SetFilePointer(fp, 0, NULL, FILE_BEGIN))
		{
			// MBCS: ���� ��°�� �����Ѵ�.
			WriteFile(fp, buffer.c_str(), buffer.size(), &size, NULL);
			SetEndOfFile(fp);
		}
	}


	////////////////////////////////////////////////////////////////////////////////
	// �Ʒ��� ���� ������ ���
	// FILEVERSION 1,0,0,1
	// PRODUCTVERSION 1,0,0,1
	bool ReplaceVersionNumber(CStdStringW &buffer, const wchar_t* findstr)
	{
		int begin = buffer.Find(findstr);
		if (-1 == begin)
			return false;
		int end = buffer.Find(L"\r\n", begin);
		if (-1 == end || begin >= end)
			return false;

		// a,b,c,d
		CStdStringW replace_src = buffer.substr(begin, end-begin);

		// d
		CStdStringW build_number_str = replace_src.substr(replace_src.ReverseFind(L",") + 1);
		//::MessageBox(NULL, _T("[") + CStdString(build_number_str) + _T("]"), _T("�ȳ�"), MB_ICONINFORMATION);

		// a,b,c,
		CStdStringW replace_dst = replace_src.substr(0, replace_src.ReverseFind(L",") + 1);
		//::MessageBox(NULL, _T("[") + CStdString(replace_dst) + _T("]"), _T("�ȳ�"), MB_ICONINFORMATION);

		// d + 1
		int build_number = _wtoi(build_number_str) + 1;

		// a,b,c,d+1
		wchar_t tmp[20];
		if (0 != _itow_s(build_number, tmp, 20, 10))
			return false;

		replace_dst += tmp;
		//::MessageBox(NULL, _T("[") + CStdString(replace_src) + _T("]"), _T("�ȳ�"), MB_ICONINFORMATION);
		//::MessageBox(NULL, _T("[") + CStdString(replace_dst) + _T("]"), _T("�ȳ�"), MB_ICONINFORMATION);
		buffer.Replace(replace_src, replace_dst);
		return true;
	}

	// �Ʒ��� ���� ������ ���
	// VALUE "FileVersion", "1.0.0.1"
	// VALUE "ProductVersion", "1.0.0.1"
	bool ReplaceVersionString(CStdStringW &buffer, const wchar_t* findstr)
	{
		int begin = buffer.Find(findstr);
		if (-1 == begin)
			return false;
		int end = buffer.Find(L"\r\n", begin);
		if (-1 == end || begin >= end)
			return false;

		// "a.b.c.d"
		CStdStringW replace_src = buffer.substr(begin, end-begin);

		// d
		CStdStringW build_number_str = replace_src.substr(replace_src.ReverseFind(L".") + 1);
		build_number_str.Remove(L'"');
		//::MessageBox(NULL, _T("[") + CStdString(build_number_str) + _T("]"), _T("�ȳ�"), MB_ICONINFORMATION);

		// "a.b.c.
		CStdStringW replace_dst = replace_src.substr(0, replace_src.ReverseFind(L".") + 1);
		//::MessageBox(NULL, _T("[") + CStdString(replace_dst) + _T("]"), _T("�ȳ�"), MB_ICONINFORMATION);

		// d + 1
		int build_number = _wtoi(build_number_str) + 1;

		// "a.b.c.d+1"
		wchar_t tmp[20];
		if (0 != _itow_s(build_number, tmp, 20, 10))
			return false;

		replace_dst += tmp;
		replace_dst += '"';
		//::MessageBox(NULL, _T("[") + CStdString(replace_src) + _T("]"), _T("�ȳ�"), MB_ICONINFORMATION);
		//::MessageBox(NULL, _T("[") + CStdString(replace_dst) + _T("]"), _T("�ȳ�"), MB_ICONINFORMATION);
		buffer.Replace(replace_src, replace_dst);
	}



	////////////////////////////////////////////////////////////////////////////////
	// �Ʒ��� ���� ������ ���
	// FILEVERSION 1,0,0,1
	// PRODUCTVERSION 1,0,0,1
	bool ReplaceVersionNumber(CStdStringA &buffer, const char* findstr)
	{
		int begin = buffer.Find(findstr);
		if (-1 == begin)
			return false;
		int end = buffer.Find("\r\n", begin);
		if (-1 == end || begin >= end)
			return false;

		// a,b,c,d
		CStdStringA replace_src = buffer.substr(begin, end-begin);

		// d
		CStdStringA build_number_str = replace_src.substr(replace_src.ReverseFind(",") + 1);
		//::MessageBox(NULL, _T("[") + CStdString(build_number_str) + _T("]"), _T("�ȳ�"), MB_ICONINFORMATION);

		// a,b,c,
		CStdStringA replace_dst = replace_src.substr(0, replace_src.ReverseFind(",") + 1);
		//::MessageBox(NULL, _T("[") + CStdString(replace_dst) + _T("]"), _T("�ȳ�"), MB_ICONINFORMATION);

		// d + 1
		int build_number = atoi(build_number_str) + 1;

		// a,b,c,d+1
		char tmp[20];
		if (0 != _itoa_s(build_number, tmp, 20, 10))
			return false;

		replace_dst += tmp;
		//::MessageBox(NULL, _T("[") + CStdString(replace_src) + _T("]"), _T("�ȳ�"), MB_ICONINFORMATION);
		//::MessageBox(NULL, _T("[") + CStdString(replace_dst) + _T("]"), _T("�ȳ�"), MB_ICONINFORMATION);
		buffer.Replace(replace_src, replace_dst);
		return true;
	}
	
	// �Ʒ��� ���� ������ ���
	// VALUE "FileVersion", "1, 0, 0, 1\0"
	// VALUE "ProductVersion", "1, 0, 0, 1\0"
	bool ReplaceVersionString(CStdStringA &buffer, const char* findstr)
	{
		int begin = buffer.Find(findstr);
		if (-1 == begin)
			return false;
		int end = buffer.Find("\r\n", begin);
		if (-1 == end || begin >= end)
			return false;

		// "a, b, c, d/0"
		CStdStringA replace_src = buffer.substr(begin, end-begin);

		// d
		CStdStringA build_number_str = replace_src.substr(replace_src.ReverseFind(",") + 2);
		build_number_str.Replace("\\0\"", NULL);
		//::MessageBox(NULL, _T("[") + CStdString(build_number_str) + _T("]"), _T("�ȳ�"), MB_ICONINFORMATION);

		// "a, b, c,
		CStdStringA replace_dst = replace_src.substr(0, replace_src.ReverseFind(",") + 1);
		//::MessageBox(NULL, _T("[") + CStdString(replace_dst) + _T("]"), _T("�ȳ�"), MB_ICONINFORMATION);

		// d + 1
		int build_number = atoi(build_number_str) + 1;

		// "a, b, c, d+1/0"
		char tmp[20];
		if (0 != _itoa_s(build_number, tmp, 20, 10))
			return false;

		replace_dst += ' ';
		replace_dst += tmp;
		replace_dst += "\\0\"";
		//::MessageBox(NULL, _T("[") + CStdString(replace_src) + _T("]"), _T("�ȳ�"), MB_ICONINFORMATION);
		//::MessageBox(NULL, _T("[") + CStdString(replace_dst) + _T("]"), _T("�ȳ�"), MB_ICONINFORMATION);
		buffer.Replace(replace_src, replace_dst);
	}
};





int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);

	ArgvHelper argv(lpCmdLine);

	if (NULL == lpCmdLine || NULL == lpCmdLine[0] || 1 > argv.Count())
	{
		::MessageBox(NULL, _T("������Ʈ ��θ� �����ϼ���.\r\nC++ ������Ʈ �� C# ������Ʈ�� �����մϴ�.\r\n\r\nPre Build Event�� �Ʒ� �׸��� �߰��մϴ�.\r\nIncrementBuildNumber \"$(ProjectDir)\""), _T("�ȳ�"), MB_ICONINFORMATION);
		return -1;
	}

    // ./Properties ���丮�� AssemblyInfo.cs ������ ������ C# ������Ʈ
    // ./ ���丮�� *.rc ������ ������ C++ ������Ʈ
	CStdString target = argv.Get(0);
	target.Replace(_T("\""), _T(""));

	if (_T('\\') != target.at(target.size() - 1))
        target += _T("\\");

	CStdString csharp_target = target + _T("Properties\\AssemblyInfo.cs");
	if (-1 != _taccess(csharp_target, 0))
	{
		ProcessCSharpProject(csharp_target);
	}
	else
	{
		CPlusPlusProject cpp;
		cpp.ProcessCPlusPlusProject(target);
	}
	return 0;
}

