
#pragma once

// �ȉ��̃}�N���́A�Œ���K�v�ȃv���b�g�t�H�[�����`���܂��B�Œ���K�v�ȃv���b�g�t�H�[���́A
// �A�v���P�[�V���������s����̂ɕK�v�ȍŏ��o�[�W������ Windows �� Internet Explorer �Ȃǂł��B
// ���̃}�N���́A���p�\�ȃv���b�g�t�H�[���̃o�[�W�����Ŏ��s�ł��邷�ׂĂ̋@�\��L���ɂ��܂��B
// �v���b�g�t�H�[���̃o�[�W�������w�肷�邱�Ƃ��ł��܂��B

// ���Ŏw�肳�ꂽ��`�̑O�ɑΏۃv���b�g�t�H�[�����w�肵�Ȃ���΂Ȃ�Ȃ��ꍇ�A�ȉ��̒�`��ύX���Ă��������B
// �قȂ�v���b�g�t�H�[���ɑΉ�����l�Ɋւ���ŐV���ɂ��ẮAMSDN ���Q�Ƃ��Ă��������B
#ifndef WINVER						// �Œ���K�v�ȃv���b�g�t�H�[���Ƃ��� Windows Vista ���w�肳��Ă��܂��B
//#define WINVER 0x0500			// Windows 2000 �`
#define WINVER 0x0501			// Windows XP �`
#endif

#ifndef _WIN32_WINNT				// �Œ���K�v�ȃv���b�g�t�H�[���Ƃ��� Windows Vista ���w�肳��Ă��܂��B
//#define _WIN32_WINNT 0x0500	// Windows 2000 �`
#define _WIN32_WINNT 0x0501		// Windows XP �`
#endif

#ifndef _WIN32_WINDOWS				// �Œ���K�v�ȃv���b�g�t�H�[���Ƃ��� Windows 98 ���w�肳��Ă��܂��B
#define _WIN32_WINDOWS 0x0410	// Windows Me �`
#endif

#ifndef _WIN32_IE					// �Œ���K�v�ȃv���b�g�t�H�[���Ƃ��� Internet Explorer 7.0 ���w�肳��Ă��܂��B
//#define _WIN32_IE 0x0700		// ����� IE �̑��̃o�[�W���������ɓK�؂Ȓl�ɕύX���Ă��������B
#define _WIN32_IE 0x0600
#endif

