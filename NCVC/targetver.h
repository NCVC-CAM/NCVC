
#pragma once

// 以下のマクロは、最低限必要なプラットフォームを定義します。最低限必要なプラットフォームは、
// アプリケーションを実行するのに必要な最小バージョンの Windows や Internet Explorer などです。
// このマクロは、利用可能なプラットフォームのバージョンで実行できるすべての機能を有効にします。
// プラットフォームのバージョンを指定することもできます。

// 下で指定された定義の前に対象プラットフォームを指定しなければならない場合、以下の定義を変更してください。
// 異なるプラットフォームに対応する値に関する最新情報については、MSDN を参照してください。
#ifndef WINVER						// 最低限必要なプラットフォームとして Windows Vista が指定されています。
//#define WINVER 0x0500			// Windows 2000 ～
#define WINVER 0x0501			// Windows XP ～
#endif

#ifndef _WIN32_WINNT				// 最低限必要なプラットフォームとして Windows Vista が指定されています。
//#define _WIN32_WINNT 0x0500	// Windows 2000 ～
#define _WIN32_WINNT 0x0501		// Windows XP ～
#endif

#ifndef _WIN32_WINDOWS				// 最低限必要なプラットフォームとして Windows 98 が指定されています。
#define _WIN32_WINDOWS 0x0410	// Windows Me ～
#endif

#ifndef _WIN32_IE					// 最低限必要なプラットフォームとして Internet Explorer 7.0 が指定されています。
//#define _WIN32_IE 0x0700		// これを IE の他のバージョン向けに適切な値に変更してください。
#define _WIN32_IE 0x0600
#endif

