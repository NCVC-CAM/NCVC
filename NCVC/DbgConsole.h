#pragma once

class DbgConsole
{
public:
	DbgConsole() {
		FILE*	fp;
		AllocConsole();
		freopen_s(&fp, "CONOUT$", "w", stdout);	// 標準出力を新しいコンソールに
		freopen_s(&fp, "CONOUT$", "w", stderr);	// 標準エラーを新しいコンソールに
	}
	~DbgConsole() {
		FreeConsole();
	}
};
