
#pragma once

// SDKDDKVer.h ���C���N���[�h����ƁA���p�ł���ł���ʂ� Windows �v���b�g�t�H�[������`����܂��B

// �ȑO�� Windows �v���b�g�t�H�[���p�ɃA�v���P�[�V�������r���h����ꍇ�́AWinSDKVer.h ���C���N���[�h���A
// SDKDDKVer.h ���C���N���[�h����O�ɁA�T�|�[�g�ΏۂƂ���v���b�g�t�H�[���������悤�� _WIN32_WINNT �}�N����ݒ肵�܂��B

//#include <SDKDDKVer.h>	// ���ꂾ��XP�� SystemParametersInfo() �̒l���������Ȃ�
#define _WIN32_WINNT 0x0501		// Windows XP �`
#include <WinSDKVer.h>

/*
// �ȉ��̃}�N���́A�Œ���K�v�ȃv���b�g�t�H�[�����`���܂��B�Œ���K�v�ȃv���b�g�t�H�[���́A
// �A�v���P�[�V���������s����̂ɕK�v�ȍŏ��o�[�W������ Windows �� Internet Explorer �Ȃǂł��B
// ���̃}�N���́A���p�\�ȃv���b�g�t�H�[���̃o�[�W�����Ŏ��s�ł��邷�ׂĂ̋@�\��L���ɂ��܂��B
// �v���b�g�t�H�[���̃o�[�W�������w�肷�邱�Ƃ��ł��܂��B

// ���Ŏw�肳�ꂽ��`�̑O�ɑΏۃv���b�g�t�H�[�����w�肵�Ȃ���΂Ȃ�Ȃ��ꍇ�A�ȉ��̒�`��ύX���Ă��������B
// �قȂ�v���b�g�t�H�[���ɑΉ�����l�Ɋւ���ŐV���ɂ��ẮAMSDN ���Q�Ƃ��Ă��������B
#ifndef WINVER
//#define WINVER 0x0500			// Windows 2000 �`
#define WINVER 0x0501			// Windows XP �`
#endif

#ifndef _WIN32_WINNT
//#define _WIN32_WINNT 0x0500	// Windows 2000 �`
#define _WIN32_WINNT 0x0501		// Windows XP �`
#endif

#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0410	// Windows Me �`
#endif

#ifndef _WIN32_IE
#define _WIN32_IE 0x0700		// ����� IE �̑��̃o�[�W���������ɓK�؂Ȓl�ɕύX���Ă��������B
//#define _WIN32_IE 0x0600
#endif
*/
