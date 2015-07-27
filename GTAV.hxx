/*
	Copyright (c) 2015 - Rafa³ 'grasmanek94/Gamer_Z' Grasman

	License: MIT

	grasmanek94 at gmail dot com
*/

#include <Windows.h>
#include <Psapi.h>
#include <vector>

#include "..\..\inc\natives.h"
#include "..\..\inc\types.h"
#include "..\..\inc\enums.h"

#include "..\..\inc\main.h"

#pragma comment(lib, "Psapi.lib")

namespace GTA_V_Scripting_Extensions
{
	#define MACRO_COMBINE_INTERNAL(X,Y) X##Y
	#define MACRO_COMBINE(X,Y) MACRO_COMBINE_INTERNAL(X,Y)

	#ifdef __COUNTER__
	#define PAD(bytes) char MACRO_COMBINE(MACRO_COMBINE(__PAD__,__COUNTER__),[(bytes)])
	#elif defined __LINE__
	#define PAD(bytes) char MACRO_COMBINE(MACRO_COMBINE(__PAD__,__LINE__),[(bytes)])
	#else
	#error Your Compiler Sucks
	#endif

	class PatternScanner
	{
	public:
		static bool memory_compare(const BYTE *data, const BYTE *pattern, const char *mask)
		{
			for (; *mask; ++mask, ++data, ++pattern)
			{
				if (*mask == 'x' && *data != *pattern)
				{
					return false;
				}
			}
			return (*mask) == NULL;
		}

		static size_t FindPattern(std::vector<unsigned short> pattern)
		{
			size_t i;
			size_t size;
			size_t address;

			MODULEINFO info = { 0 };

			address = (size_t)GetModuleHandle(NULL);
			GetModuleInformation(GetCurrentProcess(), GetModuleHandle(NULL), &info, sizeof(MODULEINFO));
			size = (size_t)info.SizeOfImage;

			std::vector<unsigned char> search;
			std::vector<char> mask;

			for (auto elem : pattern)
			{
				if (elem != 0xFFFF)
				{
					search.push_back((unsigned char)elem);
					mask.push_back('x');
				}
				else
				{
					search.push_back(0);
					mask.push_back('?');
				}
			}

			for (i = 0; i < size; ++i)
			{
				if (memory_compare((BYTE *)(address + i), (BYTE *)search.data(), mask.data()))
				{
					return (UINT64)(address + i);
				}
			}
			return 0;
		}
	};

	class CustomHelpText
	{
		const unsigned short bytes[3] =
		{
			0x4B75,
			0x1D75,
			0x9090
		};

		char* source_string;

		unsigned short* jumps[2];
		DWORD protections[2];

		size_t timer;

		bool Drawing()
		{
			return ((char*)jumps[0])[0] == 0x90;
		}

		std::string TAG;
	public:
		CustomHelpText(const std::string& TAG)
			: TAG(TAG)
		{
			timer = 0;

			size_t offset_addr = PatternScanner::FindPattern(std::vector<unsigned short>(
			{
				0x41, 0xF6, 0xC0, 0x01, 
				0x75, 0x43, 0x83, 0xF9, 
				0x03, 0x7D, 0x3E
			}));

			size_t offset = *((unsigned long*)(offset_addr + 0x23));
			source_string = (char*)((size_t)GetModuleHandle(NULL) + offset);

			size_t begin_addr = PatternScanner::FindPattern(std::vector<unsigned short>(
			{
				0x44, 0x8B, 0xC7, 0x48, 
				0x8B, 0xD6, 0x49, 0x8B, 
				0xCB, 0x89, 0x44
			}));

			jumps[0] = (unsigned short*)(begin_addr + 0x28);
			jumps[1] = (unsigned short*)(begin_addr + 0x56);

			VirtualProtect((LPVOID)jumps[0], 2, PAGE_EXECUTE_READWRITE, &protections[0]);
			VirtualProtect((LPVOID)jumps[1], 2, PAGE_EXECUTE_READWRITE, &protections[1]);
		}

		~CustomHelpText()
		{
			unsigned long dwProtect;
		
			*jumps[0] = bytes[0];
			*jumps[1] = bytes[1];

			VirtualProtect((LPVOID)jumps[0], 2, protections[0], &dwProtect);
			VirtualProtect((LPVOID)jumps[1], 2, protections[1], &dwProtect);
		}

		void Begin()
		{
			if (!Drawing())
			{
				*jumps[0] = bytes[2];
				*jumps[1] = bytes[2];

				timer = 0;
			}
		}

		void SetText(const std::string& text)//char limit is ~599
		{
			std::string combined(TAG + text);
			memcpy(source_string, combined.c_str(), combined.size() + 1);
		}

		void ShowThisFrame()
		{
			UI::DISPLAY_HELP_TEXT_THIS_FRAME("LETTERS_HELP2", false);
		}

		void End()
		{
			if (Drawing())
			{
				source_string[0] = 0;

				*jumps[0] = bytes[0];
				*jumps[1] = bytes[1];

				UI::HIDE_HELP_TEXT_THIS_FRAME();

				timer = 0;
			}
		}

		void Tick()
		{
			if (timer)
			{
				size_t time_now = GetTickCount();
				if (time_now < timer)
				{
					ShowThisFrame();
				}
				else
				{
					End();
				}
			}
		}

		void ShowTimedText(const std::string& text, size_t how_many_ms)
		{
			Begin();
			SetText(text);
			timer = (size_t)GetTickCount() + how_many_ms;
		}
	};

	class CustomKeyboardText
	{
		wchar_t* title;
		char bytes[5];
		unsigned long dwProtect;
		char* calladdr;
	public:

		CustomKeyboardText()
		{
			size_t begin_addr = PatternScanner::FindPattern(std::vector<unsigned short>(
			{
				0x41, 0xB8, 0x40, 0x00,
				0x00, 0x00, 0x48, 0x8B,
				0xD0, 0x48, 0x8B, 0xCF
			}));

			size_t offset = *((unsigned long*)(begin_addr - 0x04));
			calladdr = (char*)((unsigned long*)(begin_addr + 0x0C));

			title = (wchar_t*)(begin_addr + offset);

			VirtualProtect((LPVOID)calladdr, 5, PAGE_EXECUTE_READWRITE, &dwProtect);

			memcpy(bytes, calladdr, 5);
		}

		~CustomKeyboardText()
		{
			unsigned long unused;	

			VirtualProtect((LPVOID)calladdr, 5, dwProtect, &unused);
		}

		void DISPLAY_ONSCREEN_KEYBOARD(BOOL p0, const std::wstring& windowTitle, char* p2, char* defaultText, char* defaultConcat1, char* defaultConcat2, char* defaultConcat3, int maxInputLength)
		{
			memset(calladdr, 0x90, 5);
			memcpy(title, windowTitle.c_str(), windowTitle.size() * sizeof(wchar_t) + sizeof(wchar_t));
			GAMEPLAY::DISPLAY_ONSCREEN_KEYBOARD(0, "CELL_EMAIL_BOD", "", "", "", "", "", 500);
			memcpy(calladdr, bytes, 5);
		}
	};
};