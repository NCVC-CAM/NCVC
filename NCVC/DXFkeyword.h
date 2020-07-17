// DXFkeyword.h : ヘッダー ファイル
//		ReadDXF.cpp, DXFMakeClass.cpp で使用する
//		DXFｷｰﾜｰﾄﾞﾃｰﾌﾞﾙのｱｸｾｽ定義

#pragma once

#define	SEC_NOSECTION		-1
#define	SEC_SECTION			0
#define	SEC_ENDSEC			1
#define	SEC_EOF				2
#define	SEC_HEADER			0
#define	SEC_TABLES			1
#define	SEC_BLOCKS			2
#define	SEC_ENTITIES		3
//
#define	GROUP0				0
#define	GROUP1				1
#define	GROUP2				2
#define	GROUP3				3
#define	GROUP6				4
#define	GROUP8				5
#define	GROUP9				6
#define	GROUP70				7
//
#define	DXFMAXVALUESIZE		12
#define	VALUE10				0
#define	VALUE20				1
#define	VALUE11				2
#define	VALUE21				3
#define	VALUE40				4
#define	VALUE41				5
#define	VALUE42				6
#define	VALUE50				7
#define	VALUE51				8
#define	VALUE210			9
#define	VALUE220			10
#define	VALUE230			11
#define	VALFLG10			0x0001
#define	VALFLG20			0x0002
#define	VALFLG11			0x0004
#define	VALFLG21			0x0008
#define	VALFLG40			0x0010
#define	VALFLG41			0x0020
#define	VALFLG42			0x0040
#define	VALFLG50			0x0100
#define	VALFLG51			0x0200
#define	VALFLG210			0x1000
#define	VALFLG220			0x2000
#define	VALFLG230			0x4000
#define	VALFLG_START		(VALFLG10|VALFLG20)
#define	VALFLG_END			(VALFLG11|VALFLG21)
#define	VALFLG_POINT		(VALFLG_START)
#define	VALFLG_LINE			(VALFLG_START|VALFLG_END)
#define	VALFLG_CIRCLE		(VALFLG_POINT|VALFLG40)
#define	VALFLG_ARC			(VALFLG_CIRCLE|VALFLG50|VALFLG51)
#define	VALFLG_ELLIPSE		(VALFLG_LINE|VALFLG40|VALFLG41|VALFLG42)
#define	VALFLG_TEXT			(VALFLG_POINT)
#define	VALFLG_PLANE		(VALFLG210|VALFLG220|VALFLG230)
//
#define	HEAD_ACADVER		0
#define	HEAD_EXTMIN			1
#define	HEAD_EXTMAX			2
#define	HEAD_LIMMIN			3
#define	HEAD_LIMMAX			4
//
#define	TABLES_TABLE		0
#define	TABLES_ENDTAB		1
#define	TABLEKEY_LTYPE		0
#define	TABLEKEY_LAYER		1
#define	TABLEKEY_VPORT		2
//
#define	LTYPE_CONTINUOUS	0
#define	LTYPE_DASHED		1
#define	LTYPE_DOT			2
#define	LTYPE_DASHDOT		3
#define	LTYPE_DIVDE			4
//
#define	TYPE_POINT			0
#define	TYPE_LINE			1
#define	TYPE_CIRCLE			2
#define	TYPE_ARC			3
#define	TYPE_ELLIPSE		4
#define	TYPE_POLYLINE		5
#define	TYPE_TEXT			6
#define	TYPE_INSERT			7
#define	TYPE_LWPOLYLINE		8
#define	TYPE_VIEWPORT		9
