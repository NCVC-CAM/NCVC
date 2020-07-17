// CustomControl.h : �w�b�_�[ �t�@�C��
//

#pragma once

/*
	����ڽ���s
	VisualC++5 �����øƯ���S�W
	Scott Stanfield/Ralph Arvesen ��
	�H�R �� �Ė�
*/

/////////////////////////////////////////////////////////////////////////////
// CIntEdit �E�B���h�E

class CIntEdit : public CEdit
{
// �R���X�g���N�V����
public:
	CIntEdit();

// �A�g���r���[�g
public:

// �I�y���[�V����
public:
	operator int();
	CIntEdit& operator =(int);

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CIntEdit)
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
public:
	virtual ~CIntEdit();

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
protected:
	//{{AFX_MSG(CIntEdit)
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CFloatEdit �E�B���h�E

class CFloatEdit : public CEdit
{
	BOOL	m_bIntFormat;	// �����_�ȉ����Ȃ��Ƃ��C����̫�ϯ�

// �R���X�g���N�V����
public:
	CFloatEdit(BOOL = FALSE);

// �A�g���r���[�g
public:

// �I�y���[�V����
public:
	operator float();
	CFloatEdit& operator =(float);

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CFloatEdit)
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
public:
	virtual ~CFloatEdit();

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
protected:
	//{{AFX_MSG(CFloatEdit)
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CColComboBox �E�B���h�E

class CColComboBox : public CComboBox
{
// �R���X�g���N�V����
public:
	CColComboBox();

// �A�g���r���[�g
public:

// �I�y���[�V����
public:

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CColComboBox)
	public:
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
public:
	virtual ~CColComboBox();

	// �������ꂽ���b�Z�[�W �}�b�v�֐�
protected:
	//{{AFX_MSG(CColComboBox)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

COLORREF	ConvertSTRtoRGB(LPCTSTR);	// �������F���ɕϊ�
CString		ConvertRGBtoSTR(COLORREF);	// ���̋t
