#pragma once

class DbgConsole
{
public:
	DbgConsole() {
		FILE*	fp;
		AllocConsole();
		freopen_s(&fp, "CONOUT$", "w", stdout);	// �W���o�͂�V�����R���\�[����
		freopen_s(&fp, "CONOUT$", "w", stderr);	// �W���G���[��V�����R���\�[����
	}
	~DbgConsole() {
		FreeConsole();
	}
};
