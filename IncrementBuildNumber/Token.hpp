/******************************************************************************
	created:	2003/03/18
	author:		Hanmac software
	modify:		peter (purgatorium@gmail.com)

	purpose:	매번 strtok()로 token구하기 귀찮아서 만든 클래스
				한맥의 것이 잘 되어 있어 기존코드 날리고 이걸로 변경

	version:	1. 2003/03/18 peter
					1) 초기버전 간단하게 token구하지만 [20][40]으로 제한하였음

				2. 2005/06/19 peter
					1) 한맥의 토큰이 맘에 들어 기존 소스 삭제하고 이걸로 사용.

				3. 2006/08/03 peter
					1) <string.h> 헤더 종속성 삭제 -> windows.h 의 lstrcpy대신 사용
					2) char -> TCHAR 변경

				4. 2007/06/22 peter
					1) TCHAR전용 -> template 파라미터로 수정하여 build 환경과 상관없이 사용하게 함


******************************************************************************/


#ifndef PETER__TOKEN_HPP__
#define PETER__TOKEN_HPP__


#if _MSC_VER > 1000
#	pragma once
#endif


#include <string.h>
#include <memory.h>


namespace peter
{


const int MAXIMUM_TOKEN_COUNT = 100;				//최대 토큰

template <typename datatype = TCHAR>
class Token  
{
public:
	Token() : m_count(0)
	{
		if (sizeof(char) == sizeof(datatype))
		{
			stringlength = (StrLenFn)strlen;
			stringcopy = (StrCpyFn)strcpy;
		}
		else
		{
			stringlength = (StrLenFn)wcslen;
			stringcopy = (StrCpyFn)wcscpy;
		}

		datatype seperator[2] = {0x7C, 0x00};		// 기본 토큰분리 문자는 '|'
		stringcopy(m_seperator, seperator);
	}
	Token(const datatype *seperator) : m_count(0)
	{
		if (sizeof(char) == sizeof(datatype))
		{
			stringlength = (StrLenFn)strlen;
			stringcopy = (StrCpyFn)strcpy;
		}
		else
		{
			stringlength = (StrLenFn)wcslen;
			stringcopy = (StrCpyFn)wcscpy;
		}

		stringcopy(m_seperator, seperator);
	}
	virtual ~Token()								{ReleaseAllToken();}

public:
	int GetCount()									{return m_count;}
	void SetSeperator(const datatype *seperator)	{stringcopy(m_seperator, seperator);}

	const datatype* GetToken(int idx)
	{
		if (idx >= m_count)
			return NULL;

		return m_token[idx];
	}

	void ReleaseAllToken()
	{
		for (int i=0; i<m_count; ++i)
		{
			if (NULL != m_token[i])
				delete [] m_token[i];
		}

		m_count = 0;
	}

	void Tokenize(const datatype *tokenset)
	{
		ReleaseAllToken();

		int seperatorlen = stringlength(m_seperator);
		int pos = 0, token = 0;

		while (true)
		{
			int remainlen = FindString(tokenset+pos, m_seperator);

			// 토큰 있다
			if (-1 != remainlen)
			{
				int tokenlen = remainlen;

				m_token[token] = new datatype[tokenlen+1];
				memcpy(m_token[token], tokenset+pos, sizeof(datatype)*tokenlen);
				m_token[token++][tokenlen] = NULL;
				pos += (seperatorlen + remainlen);
			}
			else
			{
				int tokenlen = stringlength(tokenset+pos);

				// 실제 값이 없다.
				if (0 == tokenlen)
					break;

				m_token[token] = new datatype[tokenlen+1];
				memcpy(m_token[token], tokenset+pos, sizeof(datatype)*tokenlen);
				m_token[token++][tokenlen] = NULL;
				break;
			}
		}
		m_count = token;
	}

protected:
	int FindString(const datatype *tokenset, const datatype *seperator)
	{
		datatype *pos = const_cast<datatype*>(tokenset);
		int seperatorlen = stringlength(seperator);

		while (0 != memcmp(pos, seperator, seperatorlen*sizeof(datatype)))
		{
			if(NULL == *(pos++)) return -1;
		}

		return (int)(pos - tokenset);
	}

protected:
	typedef size_t		(*StrLenFn)(const datatype*);
	typedef datatype*	(*StrCpyFn)(datatype*, const datatype*);

	int			m_count;						// 토큰의 갯수
	datatype	m_seperator[20];				// 구분자
	datatype*	m_token[MAXIMUM_TOKEN_COUNT];	// 잘라진 토큰들의 포인터
	StrLenFn	stringlength;					// 상황에 따라 lstrlenA, lstrlenW가 bind된다.
	StrCpyFn	stringcopy;						// 상황에 따라 lstrcpyA, lstrcpyW가 bind된다.

	typedef int		CANNOT_USE_IMPLICTCAST_POINTER_TO_INT		[sizeof(datatype*) == sizeof(int) ? 1 : -1];
};


} // end namespace peter


#endif

