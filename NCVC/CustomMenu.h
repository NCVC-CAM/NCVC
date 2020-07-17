// CustomMenu.h: CCustomMenu �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////
// CCustomMenu �N���X�̃C���^�[�t�F�C�X

class CCustomMenu : public CMenu  
{
#ifdef _DEBUG
	void	MAP_IMAGE_PRINT() const;
	void	VEC_MNEMONIC_PRINT() const;
#endif

protected:
	CFont			m_fontMenu;		// �����̑傫���v�Z�p
	CImageList		m_ilToolBar;	// °��ް�����ݲҰ��
	CDWordArray		m_arrayImage;	// �ƭ����� ID - �Ұ�ޏ��Ή�ϯ��
	CStringArray	m_arrayString;	// �ƭ�������

	int		FindString(const CString&);
	int		FindItemID(DWORD);
	CPoint	GetIconPoint(const CRect&);

	static	HICON	m_hCheckIcon;			// �����ƭ��̱��������
	static	int		m_nIconFrameX;			// �����ڰѻ���
	static	int		m_nIconFrameY;

public:
	CCustomMenu();

// �A�g���r���[�g
public:

// �I�y���[�V����
public:
	BOOL	LoadToolBar(LPCTSTR lpszResourceName);
	BOOL	LoadToolBar(UINT nIDResource) {
		return LoadToolBar(MAKEINTRESOURCE(nIDResource));
	}
	void	RemoveMenuString(const CStringArray&);

protected:
	BOOL	LoadToolBarItem(LPCTSTR lpszResourceName);
	BOOL	LoadToolBarImage(LPCTSTR lpszResourceName);
	INT_PTR	SetMenuString(CMenu* pMenu, UINT nItem);
	void	DrawItemIconFrame(CDC* pDC, LPDRAWITEMSTRUCT lpDIS);
	void	DrawItemString(CDC* pDC, LPDRAWITEMSTRUCT lpDIS, BOOL bIcon);
	void	DrawItemStringText(CDC* pDC, CString strText, CRect rcText);
	void	DrawItemSeparator(CDC* pDC, LPDRAWITEMSTRUCT lpDIS);
	// �����Ăяo����CCustomMenuEx::DrawItemIcon()���Ăяo�����邽�� virtual�w��
	virtual	BOOL	DrawItemIcon(CDC* pDC, LPDRAWITEMSTRUCT lpDIS);

// �I�[�o�[���C�h

// �C���v�������e�[�V����
public:
	virtual ~CCustomMenu();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// ү����ϯ��(�̑���)
	void	OnSysColorChange(int nSize, UINT nIDResource[]);
	void	OnInitMenuPopup(CMenu* pMenu);
	void	OnDrawItem(LPDRAWITEMSTRUCT lpDIS);
	void	OnMeasureItem(LPMEASUREITEMSTRUCT lpMIS);
};

//////////////////////////////////////////////////////////////////////
// CCustomMenuEx �N���X�̃C���^�[�t�F�C�X

//	�����ƭ��̲Ұ�ޏ��
typedef	struct	tagCUSTMENUINFO {
	WORD	nImage;		// �Ұ��ؽĔz��
	WORD	nIndex;		// �Ұ��ؽē��̲Ұ�އ�
} CUSTMENUINFO, *LPCUSTMENUINFO;

class CCustomMenuEx : public CCustomMenu
{
	// �Ұ��ؽĂ��߲���z��
	CTypedPtrArrayEx<CObArray, CImageList*>	m_pilEnable, m_pilDisable;
	// �ƭ������ID�𷰂ɂ����Ұ�ޏ��
	CTypedPtrMap<CMapWordToPtr, WORD, LPCUSTMENUINFO>	m_mpImage;

protected:
	// ���ѲҰ�����݂�`�悷�邽�߂̵��ްײ��
	BOOL	DrawItemIcon(CDC* pDC, LPDRAWITEMSTRUCT lpDIS);

public:
	virtual ~CCustomMenuEx();

	void	SetMapImageID(WORD, WORD, WORD);
	void	RemoveCustomImageMap(size_t, LPWORD);

	void	AddCustomEnableImageList(CImageList* pilEnable) {
		m_pilEnable.Add( pilEnable );
	}
	void	AddCustomDisableImageList(CImageList* pilDisable) {
		m_pilDisable.Add( pilDisable );
	}
};
