/******************************************************************************
	created:	2003/03/18
	author:		Hanmac software
	modify:		peter (purgatorium@gmail.com)

	purpose:	�Ź� strtok()�� token���ϱ� �����Ƽ� ���� Ŭ����
				�Ѹ��� ���� �� �Ǿ� �־� �����ڵ� ������ �̰ɷ� ����

	version:	1. 2003/03/18 peter
					1) �ʱ���� �����ϰ� token�������� [20][40]���� �����Ͽ���

				2. 2005/06/19 peter
					1) �Ѹ��� ��ū�� ���� ��� ���� �ҽ� �����ϰ� �̰ɷ� ���.

				3. 2006/08/03 peter
					1) <string.h> ��� ���Ӽ� ���� -> windows.h �� lstrcpy��� ���
					2) char -> TCHAR ����

				4. 2007/06/22 peter
					1) TCHAR���� -> template �Ķ���ͷ� �����Ͽ� build ȯ��� ������� ����ϰ� ��


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


const int MAXIMUM_TOKEN_COUNT = 100;				//�ִ� ��ū

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

		datatype seperator[2] = {0x7C, 0x00};		// �⺻ ��ū�и� ���ڴ� '|'
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

			// ��ū �ִ�
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

				// ���� ���� ����.
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

	int			m_count;						// ��ū�� ����
	datatype	m_seperator[20];				// ������
	datatype*	m_token[MAXIMUM_TOKEN_COUNT];	// �߶��� ��ū���� ������
	StrLenFn	stringlength;					// ��Ȳ�� ���� lstrlenA, lstrlenW�� bind�ȴ�.
	StrCpyFn	stringcopy;						// ��Ȳ�� ���� lstrcpyA, lstrcpyW�� bind�ȴ�.

	typedef int		CANNOT_USE_IMPLICTCAST_POINTER_TO_INT		[sizeof(datatype*) == sizeof(int) ? 1 : -1];
};


} // end namespace peter


#endif

