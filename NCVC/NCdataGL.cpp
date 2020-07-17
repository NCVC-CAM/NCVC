// NCdataGL.cpp: CNCdata ÉNÉâÉXÇÃÉCÉìÉvÉäÉÅÉìÉeÅ[ÉVÉáÉì
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NCVC.h"
#include "NCdata.h"
#include "ViewOption.h"

#include "MagaDbgMac.h"
#ifdef _DEBUG
#define new DEBUG_NEW
extern	CMagaDbg	g_dbg;
#endif

using std::vector;
using namespace boost;
extern	const	PENSTYLE	g_penStyle[];	// ViewOption.cpp
extern	double	_TABLECOS[ARCCOUNT],		// NCVC.cpp
				_TABLESIN[ARCCOUNT];

// â~ï`âÊ(GL_TRIANGLE_FAN)ÇÃÇΩÇﬂÇÃí∏ì_≤›√ﬁØ∏Ω
static	const GLushort	GLFanElement[][ARCCOUNT+2] = {
	{  0,	// íÜêSç¿ïW
	   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,
	  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,
	  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
	  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,
	   1 },	// â~ÇPé¸
	{ 65,
	  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,
	  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,
	  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113,
	 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129,
	  66 }
};
// â~ï`âÊópÇÃç¿ïWÇ ﬂ≤ÃﬂèÛÇ…Ç¬Ç»ÇÆÇΩÇﬂÇÃí∏ì_≤›√ﬁØ∏Ω
static	const GLushort	GLFanStripElement[] = {
	  1,  66,   2,  67,   3,  68,   4,  69,   5,  70,   6,  71,   7,  72,   8,  73,
	  9,  74,  10,  75,  11,  76,  12,  77,  13,  78,  14,  79,  15,  80,  16,  81,
	 17,  82,  18,  83,  19,  84,  20,  85,  21,  86,  22,  87,  23,  88,  24,  89,
	 25,  90,  26,  91,  27,  92,  28,  93,  29,  94,  30,  95,  31,  96,  32,  97,
	 33,  98,  34,  99,  35, 100,  36, 101,  37, 102,  38, 103,  39, 104,  40, 105,
	 41, 106,  42, 107,  43, 108,  44, 109,  45, 110,  46, 111,  47, 112,  48, 113,
	 49, 114,  50, 115,  51, 116,  52, 117,  53, 118,  54, 119,  55, 120,  56, 121,
	 57, 122,  58, 123,  59, 124,  60, 125,  61, 126,  62, 127,  63, 128,  64, 129,
	  1,  66
};
// îºãÖÇï`âÊÇ∑ÇÈÇΩÇﬂÇÃí∏ì_≤›√ﬁØ∏Ω
static	const GLushort	GLSphereElement[][ARCCOUNT*2+2] = {
	// 0
	{  0,  64,   1,  65,   2,  66,   3,  67,   4,  68,   5,  69,   6,  70,   7,  71,
	   8,  72,   9,  73,  10,  74,  11,  75,  12,  76,  13,  77,  14,  78,  15,  79,
	  16,  80,  17,  81,  18,  82,  19,  83,  20,  84,  21,  85,  22,  86,  23,  87,
	  24,  88,  25,  89,  26,  90,  27,  91,  28,  92,  29,  93,  30,  94,  31,  95,
	  32,  96,  33,  97,  34,  98,  35,  99,  36, 100,  37, 101,  38, 102,  39, 103,
	  40, 104,  41, 105,  42, 106,  43, 107,  44, 108,  45, 109,  46, 110,  47, 111,
	  48, 112,  49, 113,  50, 114,  51, 115,  52, 116,  53, 117,  54, 118,  55, 119,
	  56, 120,  57, 121,  58, 122,  59, 123,  60, 124,  61, 125,  62, 126,  63, 127,   0,  64},
	// 1
	{ 64, 128,  65, 129,  66, 130,  67, 131,  68, 132,  69, 133,  70, 134,  71, 135,
	  72, 136,  73, 137,  74, 138,  75, 139,  76, 140,  77, 141,  78, 142,  79, 143,
	  80, 144,  81, 145,  82, 146,  83, 147,  84, 148,  85, 149,  86, 150,  87, 151,
	  88, 152,  89, 153,  90, 154,  91, 155,  92, 156,  93, 157,  94, 158,  95, 159,
	  96, 160,  97, 161,  98, 162,  99, 163, 100, 164, 101, 165, 102, 166, 103, 167,
	 104, 168, 105, 169, 106, 170, 107, 171, 108, 172, 109, 173, 110, 174, 111, 175,
	 112, 176, 113, 177, 114, 178, 115, 179, 116, 180, 117, 181, 118, 182, 119, 183,
	 120, 184, 121, 185, 122, 186, 123, 187, 124, 188, 125, 189, 126, 190, 127, 191,  64, 128},
	// 2
	{128, 192, 129, 193, 130, 194, 131, 195, 132, 196, 133, 197, 134, 198, 135, 199,
	 136, 200, 137, 201, 138, 202, 139, 203, 140, 204, 141, 205, 142, 206, 143, 207,
	 144, 208, 145, 209, 146, 210, 147, 211, 148, 212, 149, 213, 150, 214, 151, 215,
	 152, 216, 153, 217, 154, 218, 155, 219, 156, 220, 157, 221, 158, 222, 159, 223,
	 160, 224, 161, 225, 162, 226, 163, 227, 164, 228, 165, 229, 166, 230, 167, 231,
	 168, 232, 169, 233, 170, 234, 171, 235, 172, 236, 173, 237, 174, 238, 175, 239,
	 176, 240, 177, 241, 178, 242, 179, 243, 180, 244, 181, 245, 182, 246, 183, 247,
	 184, 248, 185, 249, 186, 250, 187, 251, 188, 252, 189, 253, 190, 254, 191, 255, 128, 192},
	// 3
	{192, 256, 193, 257, 194, 258, 195, 259, 196, 260, 197, 261, 198, 262, 199, 263,
	 200, 264, 201, 265, 202, 266, 203, 267, 204, 268, 205, 269, 206, 270, 207, 271,
	 208, 272, 209, 273, 210, 274, 211, 275, 212, 276, 213, 277, 214, 278, 215, 279,
	 216, 280, 217, 281, 218, 282, 219, 283, 220, 284, 221, 285, 222, 286, 223, 287,
	 224, 288, 225, 289, 226, 290, 227, 291, 228, 292, 229, 293, 230, 294, 231, 295,
	 232, 296, 233, 297, 234, 298, 235, 299, 236, 300, 237, 301, 238, 302, 239, 303,
	 240, 304, 241, 305, 242, 306, 243, 307, 244, 308, 245, 309, 246, 310, 247, 311,
	 248, 312, 249, 313, 250, 314, 251, 315, 252, 316, 253, 317, 254, 318, 255, 319, 192, 256},
	// 4
	{256, 320, 257, 321, 258, 322, 259, 323, 260, 324, 261, 325, 262, 326, 263, 327,
	 264, 328, 265, 329, 266, 330, 267, 331, 268, 332, 269, 333, 270, 334, 271, 335,
	 272, 336, 273, 337, 274, 338, 275, 339, 276, 340, 277, 341, 278, 342, 279, 343,
	 280, 344, 281, 345, 282, 346, 283, 347, 284, 348, 285, 349, 286, 350, 287, 351,
	 288, 352, 289, 353, 290, 354, 291, 355, 292, 356, 293, 357, 294, 358, 295, 359,
	 296, 360, 297, 361, 298, 362, 299, 363, 300, 364, 301, 365, 302, 366, 303, 367,
	 304, 368, 305, 369, 306, 370, 307, 371, 308, 372, 309, 373, 310, 374, 311, 375,
	 312, 376, 313, 377, 314, 378, 315, 379, 316, 380, 317, 381, 318, 382, 319, 383, 256, 320},
	// 5
	{320, 384, 321, 385, 322, 386, 323, 387, 324, 388, 325, 389, 326, 390, 327, 391,
	 328, 392, 329, 393, 330, 394, 331, 395, 332, 396, 333, 397, 334, 398, 335, 399,
	 336, 400, 337, 401, 338, 402, 339, 403, 340, 404, 341, 405, 342, 406, 343, 407,
	 344, 408, 345, 409, 346, 410, 347, 411, 348, 412, 349, 413, 350, 414, 351, 415,
	 352, 416, 353, 417, 354, 418, 355, 419, 356, 420, 357, 421, 358, 422, 359, 423,
	 360, 424, 361, 425, 362, 426, 363, 427, 364, 428, 365, 429, 366, 430, 367, 431,
	 368, 432, 369, 433, 370, 434, 371, 435, 372, 436, 373, 437, 374, 438, 375, 439,
	 376, 440, 377, 441, 378, 442, 379, 443, 380, 444, 381, 445, 382, 446, 383, 447, 320, 384},
	// 6
	{384, 448, 385, 449, 386, 450, 387, 451, 388, 452, 389, 453, 390, 454, 391, 455,
	 392, 456, 393, 457, 394, 458, 395, 459, 396, 460, 397, 461, 398, 462, 399, 463,
	 400, 464, 401, 465, 402, 466, 403, 467, 404, 468, 405, 469, 406, 470, 407, 471,
	 408, 472, 409, 473, 410, 474, 411, 475, 412, 476, 413, 477, 414, 478, 415, 479,
	 416, 480, 417, 481, 418, 482, 419, 483, 420, 484, 421, 485, 422, 486, 423, 487,
	 424, 488, 425, 489, 426, 490, 427, 491, 428, 492, 429, 493, 430, 494, 431, 495,
	 432, 496, 433, 497, 434, 498, 435, 499, 436, 500, 437, 501, 438, 502, 439, 503,
	 440, 504, 441, 505, 442, 506, 443, 507, 444, 508, 445, 509, 446, 510, 447, 511, 384, 448},
	// 7
	{448, 512, 449, 513, 450, 514, 451, 515, 452, 516, 453, 517, 454, 518, 455, 519,
	 456, 520, 457, 521, 458, 522, 459, 523, 460, 524, 461, 525, 462, 526, 463, 527,
	 464, 528, 465, 529, 466, 530, 467, 531, 468, 532, 469, 533, 470, 534, 471, 535,
	 472, 536, 473, 537, 474, 538, 475, 539, 476, 540, 477, 541, 478, 542, 479, 543,
	 480, 544, 481, 545, 482, 546, 483, 547, 484, 548, 485, 549, 486, 550, 487, 551,
	 488, 552, 489, 553, 490, 554, 491, 555, 492, 556, 493, 557, 494, 558, 495, 559,
	 496, 560, 497, 561, 498, 562, 499, 563, 500, 564, 501, 565, 502, 566, 503, 567,
	 504, 568, 505, 569, 506, 570, 507, 571, 508, 572, 509, 573, 510, 574, 511, 575, 448, 512},
	// 8
	{512, 576, 513, 577, 514, 578, 515, 579, 516, 580, 517, 581, 518, 582, 519, 583,
	 520, 584, 521, 585, 522, 586, 523, 587, 524, 588, 525, 589, 526, 590, 527, 591,
	 528, 592, 529, 593, 530, 594, 531, 595, 532, 596, 533, 597, 534, 598, 535, 599,
	 536, 600, 537, 601, 538, 602, 539, 603, 540, 604, 541, 605, 542, 606, 543, 607,
	 544, 608, 545, 609, 546, 610, 547, 611, 548, 612, 549, 613, 550, 614, 551, 615,
	 552, 616, 553, 617, 554, 618, 555, 619, 556, 620, 557, 621, 558, 622, 559, 623,
	 560, 624, 561, 625, 562, 626, 563, 627, 564, 628, 565, 629, 566, 630, 567, 631,
	 568, 632, 569, 633, 570, 634, 571, 635, 572, 636, 573, 637, 574, 638, 575, 639, 512, 576},
	// 9
	{576, 640, 577, 641, 578, 642, 579, 643, 580, 644, 581, 645, 582, 646, 583, 647,
	 584, 648, 585, 649, 586, 650, 587, 651, 588, 652, 589, 653, 590, 654, 591, 655,
	 592, 656, 593, 657, 594, 658, 595, 659, 596, 660, 597, 661, 598, 662, 599, 663,
	 600, 664, 601, 665, 602, 666, 603, 667, 604, 668, 605, 669, 606, 670, 607, 671,
	 608, 672, 609, 673, 610, 674, 611, 675, 612, 676, 613, 677, 614, 678, 615, 679,
	 616, 680, 617, 681, 618, 682, 619, 683, 620, 684, 621, 685, 622, 686, 623, 687,
	 624, 688, 625, 689, 626, 690, 627, 691, 628, 692, 629, 693, 630, 694, 631, 695,
	 632, 696, 633, 697, 634, 698, 635, 699, 636, 700, 637, 701, 638, 702, 639, 703, 576, 640},
	// 10
	{640, 704, 641, 705, 642, 706, 643, 707, 644, 708, 645, 709, 646, 710, 647, 711,
	 648, 712, 649, 713, 650, 714, 651, 715, 652, 716, 653, 717, 654, 718, 655, 719,
	 656, 720, 657, 721, 658, 722, 659, 723, 660, 724, 661, 725, 662, 726, 663, 727,
	 664, 728, 665, 729, 666, 730, 667, 731, 668, 732, 669, 733, 670, 734, 671, 735,
	 672, 736, 673, 737, 674, 738, 675, 739, 676, 740, 677, 741, 678, 742, 679, 743,
	 680, 744, 681, 745, 682, 746, 683, 747, 684, 748, 685, 749, 686, 750, 687, 751,
	 688, 752, 689, 753, 690, 754, 691, 755, 692, 756, 693, 757, 694, 758, 695, 759,
	 696, 760, 697, 761, 698, 762, 699, 763, 700, 764, 701, 765, 702, 766, 703, 767, 640, 704},
	// 11
	{704, 768, 705, 769, 706, 770, 707, 771, 708, 772, 709, 773, 710, 774, 711, 775,
	 712, 776, 713, 777, 714, 778, 715, 779, 716, 780, 717, 781, 718, 782, 719, 783,
	 720, 784, 721, 785, 722, 786, 723, 787, 724, 788, 725, 789, 726, 790, 727, 791,
	 728, 792, 729, 793, 730, 794, 731, 795, 732, 796, 733, 797, 734, 798, 735, 799,
	 736, 800, 737, 801, 738, 802, 739, 803, 740, 804, 741, 805, 742, 806, 743, 807,
	 744, 808, 745, 809, 746, 810, 747, 811, 748, 812, 749, 813, 750, 814, 751, 815,
	 752, 816, 753, 817, 754, 818, 755, 819, 756, 820, 757, 821, 758, 822, 759, 823,
	 760, 824, 761, 825, 762, 826, 763, 827, 764, 828, 765, 829, 766, 830, 767, 831, 704, 768},
	// 12
	{768, 832, 769, 833, 770, 834, 771, 835, 772, 836, 773, 837, 774, 838, 775, 839,
	 776, 840, 777, 841, 778, 842, 779, 843, 780, 844, 781, 845, 782, 846, 783, 847,
	 784, 848, 785, 849, 786, 850, 787, 851, 788, 852, 789, 853, 790, 854, 791, 855,
	 792, 856, 793, 857, 794, 858, 795, 859, 796, 860, 797, 861, 798, 862, 799, 863,
	 800, 864, 801, 865, 802, 866, 803, 867, 804, 868, 805, 869, 806, 870, 807, 871,
	 808, 872, 809, 873, 810, 874, 811, 875, 812, 876, 813, 877, 814, 878, 815, 879,
	 816, 880, 817, 881, 818, 882, 819, 883, 820, 884, 821, 885, 822, 886, 823, 887,
	 824, 888, 825, 889, 826, 890, 827, 891, 828, 892, 829, 893, 830, 894, 831, 895, 768, 832},
	// 13
	{832, 896, 833, 897, 834, 898, 835, 899, 836, 900, 837, 901, 838, 902, 839, 903,
	 840, 904, 841, 905, 842, 906, 843, 907, 844, 908, 845, 909, 846, 910, 847, 911,
	 848, 912, 849, 913, 850, 914, 851, 915, 852, 916, 853, 917, 854, 918, 855, 919,
	 856, 920, 857, 921, 858, 922, 859, 923, 860, 924, 861, 925, 862, 926, 863, 927,
	 864, 928, 865, 929, 866, 930, 867, 931, 868, 932, 869, 933, 870, 934, 871, 935,
	 872, 936, 873, 937, 874, 938, 875, 939, 876, 940, 877, 941, 878, 942, 879, 943,
	 880, 944, 881, 945, 882, 946, 883, 947, 884, 948, 885, 949, 886, 950, 887, 951,
	 888, 952, 889, 953, 890, 954, 891, 955, 892, 956, 893, 957, 894, 958, 895, 959, 832, 896},
	// 14 (îºãÖí∏ì_ FANÇ≈ï`âÊ)
	{960,
	 896, 897, 898, 899, 900, 901, 902, 903, 904, 905, 906, 907, 908, 909, 910, 911,
	 912, 913, 914, 915, 916, 917, 918, 919, 920, 921, 922, 923, 924, 925, 926, 927,
	 928, 929, 930, 931, 932, 933, 934, 935, 936, 937, 938, 939, 940, 941, 942, 943,
	 944, 945, 946, 947, 948, 949, 950, 951, 952, 953, 954, 955, 956, 957, 958, 959, 896,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0}
};

//////////////////////////////////////////////////////////////////////

static inline void _SetEndmillCircle(double r, const CPoint3D& ptOrg,
	CPoint3D ptResult[ARCCOUNT])
{
	// â~ç¿ïWÇptResult[]Ç…æØƒ
	for ( int i=0; i<ARCCOUNT; ++i ) {
		ptResult[i].x = r * _TABLECOS[i] + ptOrg.x;
		ptResult[i].y = r * _TABLESIN[i] + ptOrg.y;
		ptResult[i].z = ptOrg.z;
	}
	// ç≈å„ÇÃï¬ç¿ïWÇÕï`âÊéûÇ…ê›íË
}

static inline void _SetEndmillOrgCircle(double r, const CPoint3D& ptOrg,
	CPoint3D ptResult[ARCCOUNT+1], int nOffset)
{
	int		i = 0, n = nOffset * (ARCCOUNT+1);

	// íÜêSç¿ïWÇä‹Çﬁâ~ç¿ïWÇptResult[]Ç…æØƒ
	ptResult[n++] = ptOrg;
	for ( ; i<ARCCOUNT; ++i, ++n ) {
		ptResult[n].x = r * _TABLECOS[i] + ptOrg.x;
		ptResult[n].y = r * _TABLESIN[i] + ptOrg.y;
		ptResult[n].z = ptOrg.z;
	}
}

static inline void _SetEndmillSphere(double d, const CPoint3D& ptOrg,
	CPoint3D ptResult[ARCCOUNT/4][ARCCOUNT])
{
	int			i, k;
	double		x, dz = d + ptOrg.z;
	CPoint3D	ptZ(ptOrg);

	// îºãÖç¿ïWÅAZï˚å¸Ç…ó÷êÿÇË 90Åã-1âÒï™ÇZï˚å¸Ç…åJÇËï‘Ç∑
	// 180ìxÇ©ÇÁ270ìxéËëOÇ‹Ç≈
	for ( i=ARCCOUNT/2, k=0; k<ARCCOUNT/4-1; ++i, ++k ) {
		x     = d * _TABLECOS[i];		// Ç±ÇÃZà íuÇÃîºåa
		ptZ.z = d * _TABLESIN[i] + dz;	// ê[Ç≥
		_SetEndmillCircle(x, ptZ, ptResult[k]);	// Ç±ÇÃà íuÇ…XYïΩñ ÇÃâ~ç¿ïWÇìoò^
	}
	// 270ìxí∏ì_
	ptResult[k][0] = ptOrg;	// í∏ì_ç¿ïWÇÕ[0]ÇÃÇ›égóp
}

static inline void _SetEndmillSpherePath
	(double d, double qp, const CPoint3D& ptOrg,
	CPoint3D* lptResult)	// ARCCOUNT/2+1ï™ÇæÇØégóp
{
	int			i = ARCCOUNT/2, k = 0;	// 180ìxÅ`360ìx åv33å¬
	double		x,
				cos_qp = cos(qp),
				sin_qp = sin(qp);
	CPoint3D	pt;

	// Zï˚å¸Ç÷ÇÃîºâ~å@ÇËâ∫Ç∞(180ìxÅ`360ìxÇÃéËëO)
	for ( ; i<ARCCOUNT; ++i, ++k ) {
		x    = d * _TABLECOS[i];
		pt.z = d * _TABLESIN[i] + d;
		pt.x = x * cos_qp;	// - y * sin_qp;
		pt.y = x * sin_qp;	// + y * cos_qp;
		lptResult[k] = pt + ptOrg;
	}
	// 360(0)ìxï™
	x = pt.z = d;
	pt.x = x * cos_qp;
	pt.y = x * sin_qp;
	lptResult[k] = pt + ptOrg;
}

static inline void _SetEndmillSpherePath2
	(double d, double qp, const CPoint3D& ptOrg1, const CPoint3D& ptOrg2,
	CPoint3D* lptResult)
{
	int			i = ARCCOUNT/2, k = 0;
	double		x,
				cos_qp = cos(qp),
				sin_qp = sin(qp);
	CPoint3D	pt;

	for ( ; i<ARCCOUNT; ++i ) {
		x    = d * _TABLECOS[i];
		pt.z = d * _TABLESIN[i] + d;
		pt.x = x * cos_qp;
		pt.y = x * sin_qp;
		lptResult[k++] = pt + ptOrg1;
		lptResult[k++] = pt + ptOrg2;
	}
	x = pt.z = d;
	pt.x = x * cos_qp;
	pt.y = x * sin_qp;
	lptResult[k++] = pt + ptOrg1;
	lptResult[k  ] = pt + ptOrg2;
}

static inline void _DrawBottomFaceSphere(void)
{
	int		i;

	// í∏ì_ÇèúÇ≠îºãÖï`âÊ
	for ( i=0; i<SIZEOF(GLSphereElement)-1; ++i ) {
		::glDrawRangeElements(GL_TRIANGLE_STRIP,
			GLSphereElement[i][0], GLSphereElement[i][0]+ARCCOUNT*2-1,
			SIZEOF(GLSphereElement[0]), GL_UNSIGNED_SHORT, GLSphereElement[i]);
	}
	// îºãÖí∏ì_
	::glDrawRangeElements(GL_TRIANGLE_FAN,
		896, 960, // GLSphereElement[i][1], GLSphereElement[i][1]+ARCCOUNT,
		ARCCOUNT+2, GL_UNSIGNED_SHORT, GLSphereElement[i]);
}

static inline void _DrawEndmillPipe(size_t nPipe, size_t nVertex,
	const CPoint3D ptPipe[ARCCOUNT+1][ARCCOUNT])
{
	GLuint	i, j, k, i1, i2;
	size_t	n = (nVertex+1)*2;
	GLuint*	pElement  = new GLuint[n];

	::glVertexPointer(3, GL_DOUBLE, 0, ptPipe);

	for ( i=0; i<nPipe-1; ++i ) {
		i1 = ARCCOUNT * i;
		i2 = ARCCOUNT *(i+1);
		for ( j=k=0; j<nVertex; ++j ) {
			pElement[k++] = i1 + j;
			pElement[k++] = i2 + j;
		}
		pElement[k++] = i1;
		pElement[k  ] = i2;
		::glDrawRangeElements(GL_TRIANGLE_STRIP,
			i1, i2+j-1,
			n, GL_UNSIGNED_INT, pElement);
	}

	delete[]	pElement;
}

//////////////////////////////////////////////////////////////////////
// NC√ﬁ∞¿ÇÃäÓëb√ﬁ∞¿∏◊Ω
//////////////////////////////////////////////////////////////////////

void CNCdata::DrawGLMillWire(void) const
{
	// îhê∂∏◊ΩÇ©ÇÁÇÃã§í åƒÇ—èoÇµ
	for ( int i=0; i<m_obCdata.GetSize(); ++i )
		m_obCdata[i]->DrawGLMillWire();
}

void CNCdata::DrawGLWireWire(void) const
{
}

void CNCdata::DrawGLLatheFace(void) const
{
}

void CNCdata::DrawGLBottomFace(void) const
{
	for ( int i=0; i<m_obCdata.GetSize(); ++i )
		m_obCdata[i]->DrawGLBottomFace();
}

int CNCdata::AddGLWireFirstVertex(vector<GLfloat>& vVertex, vector<GLfloat>& vNormal) const
{
	if ( !m_pWireObj )
		return -1;

	CPoint3D	pts(m_pWireObj->GetStartPoint());

	// µÃﬁºﬁ™∏ƒénì_Çìoò^
	vVertex.push_back((GLfloat)m_ptValS.x);
	vVertex.push_back((GLfloat)m_ptValS.y);
	vVertex.push_back((GLfloat)m_ptValS.z);
	vVertex.push_back((GLfloat)pts.x);
	vVertex.push_back((GLfloat)pts.y);
	vVertex.push_back((GLfloat)pts.z);

	// ñ@ê¸Õﬁ∏ƒŸ
	optional<CPointD> ptResult = CalcPerpendicularPoint(STARTPOINT, 1.0, 1);
	if ( ptResult ) {
		CPointD	pt( *ptResult );
		vNormal.push_back((GLfloat)pt.x);
		vNormal.push_back((GLfloat)pt.y);
		vNormal.push_back((GLfloat)m_ptValS.z);
		vNormal.push_back((GLfloat)pt.x);
		vNormal.push_back((GLfloat)pt.y);
		vNormal.push_back((GLfloat)pts.z);
	}
	else {
		// ï€åØ
		vNormal.insert(vNormal.end(), NCXYZ*2, 1.0f);
	}

	return 2;
}

int CNCdata::AddGLWireVertex(vector<GLfloat>&, vector<GLfloat>&) const
{
	return 0;
}

int CNCdata::AddGLWireTexture(int, double&, double, GLfloat*) const
{
	return 0;
}

//////////////////////////////////////////////////////////////////////
// CNCline ÉNÉâÉX
//////////////////////////////////////////////////////////////////////

void CNCline::DrawGLMillWire(void) const
{
	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();

	if ( m_obCdata.IsEmpty() || pOpt->GetNCViewFlg(NCVIEWFLG_DRAWREVISE) ) {
		COLORREF col = pOpt->GetNcDrawColor(
			m_obCdata.IsEmpty() ? (GetPenType()+NCCOL_G0) : NCCOL_CORRECT);
		::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(GetLineType())].nGLpattern);
		::glBegin(GL_LINES);
			::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
			::glVertex3dv((const GLdouble *)&m_ptValS);
			::glVertex3dv((const GLdouble *)&m_ptValE);
		::glEnd();
	}

	CNCdata::DrawGLMillWire();
}

void CNCline::DrawGLWireWire(void) const
{
	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	COLORREF col = pOpt->GetNcDrawColor( GetPenType()+NCCOL_G0 );
	::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(GetLineType())].nGLpattern);
	// XY
	::glBegin(GL_LINES);
		::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
		::glVertex3dv((const GLdouble *)&m_ptValS);
		::glVertex3dv((const GLdouble *)&m_ptValE);
		if ( m_pWireObj ) {
			// UV
			CPoint3D	pts(m_pWireObj->GetStartPoint()),
						pte(m_pWireObj->GetEndPoint());
			::glVertex3dv((const GLdouble *)&pts);
			::glVertex3dv((const GLdouble *)&pte);
			// XYÇ∆UVÇÃê⁄ë±
			::glVertex3dv((const GLdouble *)&m_ptValS);
			::glVertex3dv((const GLdouble *)&pts);
			::glVertex3dv((const GLdouble *)&m_ptValE);
			::glVertex3dv((const GLdouble *)&pte);
		}
	::glEnd();
}

void CNCline::DrawGLLatheFace(void) const
{
	if ( m_nc.nGcode != 1 )
		return;

	::glBegin(GL_LINES);
		::glVertex3dv((const GLdouble *)&m_ptValS);
		::glVertex3dv((const GLdouble *)&m_ptValE);
	::glEnd();
}

void CNCline::DrawGLBottomFace(void) const
{
	if ( m_nc.nGcode != 1 )
		return;
	if ( !m_obCdata.IsEmpty() ) {
		CNCdata::DrawGLBottomFace();
		return;
	}

	if ( GetEndmillType() == 0 ) {
		// Ω∏≥™±ç¿ïWåvéZ
		CPoint3D	pt[(ARCCOUNT+1)*2];	// íÜêSç¿ïWä‹Çﬁénì_èIì_
		::glVertexPointer(3, GL_DOUBLE, 0, pt);
		// énì_ÅõÇÃï`âÊ
		_SetEndmillOrgCircle(m_dEndmill, m_ptValS, pt, 0);
		::glDrawRangeElements(GL_TRIANGLE_FAN,
			0, ARCCOUNT,
			SIZEOF(GLFanElement[0]), GL_UNSIGNED_SHORT, GLFanElement[0]);
		if ( m_dMove[NCA_X]>0 || m_dMove[NCA_Y]>0 ) {
			// èIì_ÅõÇÃï`âÊ
			_SetEndmillOrgCircle(m_dEndmill, m_ptValE, pt, 1);
			::glDrawRangeElements(GL_TRIANGLE_FAN,
				65, 129, // ARCCOUNT+1, ARCCOUNT*2+1,
				SIZEOF(GLFanElement[1]), GL_UNSIGNED_SHORT, GLFanElement[1]);
			if ( m_dMove[NCA_Z]>0 ) {
				// XZ, YZ Ç‡ÇµÇ≠ÇÕ 3é≤à⁄ìÆÇÃÇ∆Ç´ÇÕ
				// pts Ç∆ pte ÇÃâ~Ç ﬂ≤ÃﬂèÛÇ…Ç¬Ç»ÇÆ
				::glDrawRangeElements(GL_TRIANGLE_STRIP,
					1, 129, // 1, ARCCOUNT*2+1,
					SIZEOF(GLFanStripElement), GL_UNSIGNED_SHORT, GLFanStripElement);
			}
			else {
				// XYïΩñ ÇÃà⁄ìÆ ﬂΩÇÕ
				// énì_èIì_ÇãÈå`Ç≈Ç¬Ç»ÇÆ
				double	q = atan2(m_ptValE.y-m_ptValS.y, m_ptValE.x-m_ptValS.x)+RAD(90.0),
						cos_q = cos(q) * m_dEndmill,
						sin_q = sin(q) * m_dEndmill;
				pt[0].SetPoint( cos_q+m_ptValS.x, sin_q+m_ptValS.y, m_ptValS.z);
				pt[1].SetPoint(-cos_q+m_ptValS.x,-sin_q+m_ptValS.y, m_ptValS.z);
				pt[2].SetPoint( cos_q+m_ptValE.x, sin_q+m_ptValE.y, m_ptValE.z);
				pt[3].SetPoint(-cos_q+m_ptValE.x,-sin_q+m_ptValE.y, m_ptValE.z);
				::glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			}
		}
	}
	else {
		// Œﬁ∞Ÿ¥›ƒﬁ–Ÿç¿ïWåvéZ
		CPoint3D	ptSphere[ARCCOUNT/4][ARCCOUNT];
		::glVertexPointer(3, GL_DOUBLE, 0, ptSphere);
		// énì_ÇÃîºãÖï`âÊ
		_SetEndmillSphere(m_dEndmill, m_ptValS, ptSphere);
		_DrawBottomFaceSphere();
		if ( m_dMove[NCA_X]>0 || m_dMove[NCA_Y]>0 ) {
			// èIì_ÇÃîºãÖï`âÊ
			_SetEndmillSphere(m_dEndmill, m_ptValE, ptSphere);
			_DrawBottomFaceSphere();
			// Œﬁ∞Ÿ¥›ƒﬁ–ŸÇÃà⁄ìÆ ﬂΩï`âÊ
			CPoint3D	pt[(ARCCOUNT/2+1)*2];
			double	q = atan2(m_ptValE.y-m_ptValS.y, m_ptValE.x-m_ptValS.x) + RAD(90.0);
			_SetEndmillSpherePath2(m_dEndmill, q, m_ptValS, m_ptValE, pt);
			::glVertexPointer(3, GL_DOUBLE, 0, pt);
			::glDrawArrays(GL_TRIANGLE_STRIP, 0, SIZEOF(pt));
		}
	}
}

int CNCline::AddGLWireVertex(vector<GLfloat>& vVertex, vector<GLfloat>& vNormal) const
{
	if ( m_nc.nGcode!=1 || !m_pWireObj )
		return -1;		// à⁄ìÆ∫∞ƒﬁ => Ãﬁ⁄≤∏

	CPoint3D	pte(m_pWireObj->GetEndPoint());

	// µÃﬁºﬁ™∏ƒèIì_Çìoò^
	vVertex.push_back((GLfloat)m_ptValE.x);
	vVertex.push_back((GLfloat)m_ptValE.y);
	vVertex.push_back((GLfloat)m_ptValE.z);
	vVertex.push_back((GLfloat)pte.x);
	vVertex.push_back((GLfloat)pte.y);
	vVertex.push_back((GLfloat)pte.z);

	// ñ@ê¸Õﬁ∏ƒŸ
	optional<CPointD> ptResult = CalcPerpendicularPoint(ENDPOINT, 1.0, 1);
	if ( ptResult ) {
		CPointD	pt( *ptResult );
		vNormal.push_back((GLfloat)pt.x);
		vNormal.push_back((GLfloat)pt.y);
		vNormal.push_back((GLfloat)m_ptValE.z);
		vNormal.push_back((GLfloat)pt.x);
		vNormal.push_back((GLfloat)pt.y);
		vNormal.push_back((GLfloat)pte.z);
	}
	else {
		// ï€åØ
		vNormal.insert(vNormal.end(), NCXYZ*2, 1.0f);
	}

	return 2;	// í∏ì_êî
}

int CNCline::AddGLWireTexture(int n, double& dAccuLength, double dAllLength, GLfloat* pfTEX) const
{
	if ( m_nc.nGcode!=1 || !m_pWireObj )
		return -1;

	dAccuLength += m_nc.dLength;
	GLfloat	f = (GLfloat)(dAccuLength / dAllLength);

	pfTEX[n++] = f;
	pfTEX[n++] = 0.0;
	pfTEX[n++] = f;
	pfTEX[n++] = 1.0;

	return 4;
}

//////////////////////////////////////////////////////////////////////
// CNCcycle ÉNÉâÉX
//////////////////////////////////////////////////////////////////////

void CNCcycle::DrawGLMillWire(void) const
{
	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	COLORREF	colG0 = pOpt->GetNcDrawColor( NCCOL_G0 ),
				colCY = pOpt->GetNcDrawColor( NCCOL_CYCLE );
	GLubyte	bG0col[3], bCYcol[3];
	bG0col[0] = GetRValue(colG0);
	bG0col[1] = GetGValue(colG0);
	bG0col[2] = GetBValue(colG0);
	bCYcol[0] = GetRValue(colCY);
	bCYcol[1] = GetGValue(colCY);
	bCYcol[2] = GetBValue(colCY);

	::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCCOLLINE_G0)].nGLpattern);
	::glBegin(GL_LINE_STRIP);
		::glColor3ubv( bG0col );
		::glVertex3dv((const GLdouble *)&m_ptValS);
		::glVertex3dv((const GLdouble *)&m_ptValI);
		::glVertex3dv((const GLdouble *)&m_ptValR);
		::glVertex3dv((const GLdouble *)&m_ptValE);
	::glEnd();
	::glBegin(GL_LINES);
	for ( int i=0; i<m_nDrawCnt; ++i ) {
		::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCCOLLINE_G0)].nGLpattern);
		::glColor3ubv( bG0col );
		::glVertex3dv((const GLdouble *)&m_Cycle3D[i].ptI);
		::glVertex3dv((const GLdouble *)&m_Cycle3D[i].ptR);
		::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCCOLLINE_CYCLE)].nGLpattern);
		::glColor3ubv( bCYcol );
		::glVertex3dv((const GLdouble *)&m_Cycle3D[i].ptR);
		::glVertex3dv((const GLdouble *)&m_Cycle3D[i].ptC);
	}
	::glEnd();
}

void CNCcycle::DrawGLWireWire(void) const
{
}

void CNCcycle::DrawGLLatheFace(void) const
{
}

void CNCcycle::DrawGLBottomFace(void) const
{
	if ( GetPlane() != XY_PLANE )
		return;

	int		i;

	if ( GetEndmillType() == 0 ) {
		CPoint3D	ptc[ARCCOUNT+1];
		::glVertexPointer(3, GL_DOUBLE, 0, ptc);
		for ( i=0; i<m_nDrawCnt; ++i ) {
			// ç¿ïWåvéZÇ∆ï`âÊ
			_SetEndmillOrgCircle(m_dEndmill, m_Cycle3D[i].ptC, ptc, 0);
			::glDrawRangeElements(GL_TRIANGLE_FAN,
				0, ARCCOUNT,
				SIZEOF(GLFanElement[0]), GL_UNSIGNED_SHORT, GLFanElement[0]);
		}
	}
	else {
		CPoint3D	ptSphere[ARCCOUNT/4][ARCCOUNT];
		::glVertexPointer(3, GL_DOUBLE, 0, ptSphere);
		for ( i=0; i<m_nDrawCnt; ++i ) {
			_SetEndmillSphere(m_dEndmill, m_Cycle3D[i].ptC, ptSphere);
			_DrawBottomFaceSphere();
		}
	}
}

int CNCcycle::AddGLWireVertex(vector<GLfloat>&, vector<GLfloat>&) const
{
	return 0;
}

int CNCcycle::AddGLWireTexture(int, double&, double, GLfloat*) const
{
	return 0;
}

//////////////////////////////////////////////////////////////////////
// CNCcircle ÉNÉâÉX
//////////////////////////////////////////////////////////////////////

void CNCcircle::DrawGLMillWire(void) const
{
	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();

	if ( m_obCdata.IsEmpty() || pOpt->GetNCViewFlg(NCVIEWFLG_DRAWREVISE) ) {
		COLORREF	col = pOpt->GetNcDrawColor(
			m_obCdata.IsEmpty() ? NCCOL_G1 : NCCOL_CORRECT);
		::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCCOLLINE_G1)].nGLpattern);
		::glBegin(GL_LINE_STRIP);
			::glColor3ub( GetRValue(col), GetGValue(col), GetBValue(col) );
			DrawGLWire();	// ç¿ïWéwé¶
		::glEnd();
	}

	CNCdata::DrawGLMillWire();
}

void CNCcircle::DrawGLWireWire(void) const
{
	const CViewOption*	pOpt = AfxGetNCVCApp()->GetViewOption();
	COLORREF	col = pOpt->GetNcDrawColor(NCCOL_G1);

	::glLineStipple(1, g_penStyle[pOpt->GetNcDrawType(NCCOLLINE_G1)].nGLpattern);

	// XY
	::glBegin(GL_LINE_STRIP);
		::glColor3ub(GetRValue(col), GetGValue(col), GetBValue(col));
		DrawGLWire();
	::glEnd();
	// UV
	if ( m_pWireObj ) {
		::glBegin(GL_LINE_STRIP);
			static_cast<CNCcircle*>(m_pWireObj)->DrawGLWire();
		::glEnd();
		// XYÇ∆UVÇÃê⁄ë±
		CPoint3D	pts(m_pWireObj->GetStartPoint()),
					pte(m_pWireObj->GetEndPoint());
		::glBegin(GL_LINES);
			::glVertex3dv((const GLdouble *)&m_ptValS);
			::glVertex3dv((const GLdouble *)&pts);
			::glVertex3dv((const GLdouble *)&m_ptValE);
			::glVertex3dv((const GLdouble *)&pte);
		::glEnd();
	}
}

void CNCcircle::DrawGLLatheFace(void) const
{
	::glBegin(GL_LINE_STRIP);
		DrawGLWire();
	::glEnd();
}

void CNCcircle::DrawGLWire(void) const
{
#ifdef _DEBUGOLD
	CMagaDbg	dbg;
	int			dbgCnt = 0;
#endif
	double		sq, eq, st, r = fabs(m_r);
	tie(sq, eq) = GetSqEq();
	CPoint3D	pt;

	switch ( GetPlane() ) {
	case XY_PLANE:
		// ARCSTEP Ç√Ç¬î˜ç◊ê¸ï™Ç≈ï`âÊ
		if ( m_nG23 == 0 ) {
			st = (sq - eq) / ARCCOUNT;
//			for ( pt.z=m_ptValS.z; sq>eq; sq-=ARCSTEP, pt.z+=m_dHelicalStep ) {
			for ( pt.z=m_ptValS.z; sq>eq; sq-=st, pt.z+=m_dHelicalStep ) {
				pt.x = r * cos(sq) + m_ptOrg.x;
				pt.y = r * sin(sq) + m_ptOrg.y;
				::glVertex3dv((const GLdouble *)&pt);
#ifdef _DEBUGOLD
				dbgCnt++;
#endif
			}
		}
		else {
			st = (eq - sq) / ARCCOUNT;
//			for ( pt.z=m_ptValS.z; sq<eq; sq+=ARCSTEP, pt.z+=m_dHelicalStep ) {
			for ( pt.z=m_ptValS.z; sq<eq; sq+=st, pt.z+=m_dHelicalStep ) {
				pt.x = r * cos(sq) + m_ptOrg.x;
				pt.y = r * sin(sq) + m_ptOrg.y;
				::glVertex3dv((const GLdouble *)&pt);
#ifdef _DEBUGOLD
				dbgCnt++;
#endif
			}
		}
		// í[êîï™ï`âÊ
		pt.x = r * cos(eq) + m_ptOrg.x;
		pt.y = r * sin(eq) + m_ptOrg.y;
		pt.z = m_ptValE.z;		// Õÿ∂ŸèIóπç¿ïW
		::glVertex3dv((const GLdouble *)&pt);
#ifdef _DEBUGOLD
		dbg.printf("DrawCnt=%d", dbgCnt+1);
#endif
		break;

	case XZ_PLANE:
		if ( m_nG23 == 0 ) {
			st = (sq - eq) / ARCCOUNT;
//			for ( pt.y=m_ptValS.y; sq>eq; sq-=ARCSTEP, pt.y+=m_dHelicalStep ) {
			for ( pt.y=m_ptValS.y; sq>eq; sq-=st, pt.y+=m_dHelicalStep ) {
				pt.x = r * cos(sq) + m_ptOrg.x;
				pt.z = r * sin(sq) + m_ptOrg.z;
				::glVertex3dv((const GLdouble *)&pt);
			}
		}
		else {
			st = (eq - sq) / ARCCOUNT;
//			for ( pt.y=m_ptValS.y; sq<eq; sq+=ARCSTEP, pt.y+=m_dHelicalStep ) {
			for ( pt.y=m_ptValS.y; sq<eq; sq+=st, pt.y+=m_dHelicalStep ) {
				pt.x = r * cos(sq) + m_ptOrg.x;
				pt.z = r * sin(sq) + m_ptOrg.z;
				::glVertex3dv((const GLdouble *)&pt);
			}
		}
		pt.x = r * cos(eq) + m_ptOrg.x;
		pt.y = m_ptValE.y;
		pt.z = r * sin(eq) + m_ptOrg.z;
		::glVertex3dv((const GLdouble *)&pt);
		break;

	case YZ_PLANE:
		if ( m_nG23 == 0 ) {
			st = (sq - eq) / ARCCOUNT;
//			for ( pt.x=m_ptValS.x; sq>eq; sq-=ARCSTEP, pt.x+=m_dHelicalStep ) {
			for ( pt.x=m_ptValS.x; sq>eq; sq-=st, pt.x+=m_dHelicalStep ) {
				pt.y = r * cos(sq) + m_ptOrg.y;
				pt.z = r * sin(sq) + m_ptOrg.z;
				::glVertex3dv((const GLdouble *)&pt);
			}
		}
		else {
			st = (eq - sq) / ARCCOUNT;
//			for ( pt.x=m_ptValS.x; sq<eq; sq+=ARCSTEP, pt.x+=m_dHelicalStep ) {
			for ( pt.x=m_ptValS.x; sq<eq; sq+=st, pt.x+=m_dHelicalStep ) {
				pt.y = r * cos(sq) + m_ptOrg.y;
				pt.z = r * sin(sq) + m_ptOrg.z;
				::glVertex3dv((const GLdouble *)&pt);
			}
		}
		pt.x = m_ptValE.x;
		pt.y = r * cos(eq) + m_ptOrg.y;
		pt.z = r * sin(eq) + m_ptOrg.z;
		::glVertex3dv((const GLdouble *)&pt);
		break;
	}
}

//	--- CNCcircle::DrawGLBottomFace() ÉTÉu
static inline void _SetEndmillPathXY
	(const CPointD& pt, double q, double h, double r1, double r2,
		int n, CPoint3D* lptStrip)
{
	double	cos_q = cos(q);
	double	sin_q = sin(q);
	lptStrip[n  ].x = r1 * cos_q + pt.x;
	lptStrip[n  ].y = r1 * sin_q + pt.y;
	lptStrip[n++].z = h;
	lptStrip[n  ].x = r2 * cos_q + pt.x;
	lptStrip[n  ].y = r2 * sin_q + pt.y;
	lptStrip[n  ].z = h;
}

static inline void _SetEndmillPathXY_Pipe
	(const CPointD& ptOrg, double q, double rr, double h, double d,
		CPoint3D	ptResult[ARCCOUNT])
{
	CPoint3D	pt(
		rr * cos(q) + ptOrg.x,
		rr * sin(q) + ptOrg.y,
		h
	);
	// Ç±ÇÃà íuÇ…XYïΩñ ÇÃâ~Çç¿ïWìoò^
	_SetEndmillCircle(d, pt, ptResult);
}

static inline void _SetEndmillPathXZ_Pipe
	(const CPointD& ptOrg, double q, double rr, double h, double d,
		CPoint3D	ptResult[ARCCOUNT])
{
	CPoint3D	pt(
		rr * cos(q) + ptOrg.x,
		h,
		rr * sin(q) + ptOrg.y
	);
	_SetEndmillCircle(d, pt, ptResult);
}

static inline void _SetEndmillPathYZ_Pipe
	(const CPointD& ptOrg, double q, double rr, double h, double d,
		CPoint3D	ptResult[ARCCOUNT])
{
	CPoint3D	pt(
		h,
		rr * cos(q) + ptOrg.x,
		rr * sin(q) + ptOrg.y
	);
	_SetEndmillCircle(d, pt, ptResult);
}

static inline void _SetEndmillPathXZ_Sphere
	(double d, double qp, const CPoint3D& ptOrg,
		CPoint3D ptResult[ARCCOUNT])
{
	double		x,
				cos_qp = cos(qp),
				sin_qp = sin(qp);
	CPoint3D	pt;

	for ( int i=0; i<ARCCOUNT; ++i ) {
		// XYïΩñ ÇÃâ~
		x    = d * _TABLECOS[i];
		pt.y = d * _TABLESIN[i];
		// ZÇ≈ÇÃâÒì]
		pt.x = x * cos_qp;	// - z * sin_qp;
		pt.z = x * sin_qp;	// + z * cos_qp;
		//
		ptResult[i] = pt + ptOrg;
	}
}

static inline void _SetEndmillPathYZ_Sphere
	(double d, double qp, const CPoint3D& ptOrg,
		CPoint3D ptResult[ARCCOUNT])
{
	double		y,
				cos_qp = cos(qp),
				sin_qp = sin(qp);
	CPoint3D	pt;

	for ( int i=0; i<ARCCOUNT; ++i ) {
		// XYïΩñ ÇÃâ~
		pt.x = d * _TABLECOS[i];
		y    = d * _TABLESIN[i];
		// ZÇ≈ÇÃâÒì]
		pt.y = y * cos_qp;	// - z * sin_qp;
		pt.z = y * sin_qp;	// + z * cos_qp;
		//
		ptResult[i] = pt + ptOrg;
	}
}
//	---
void CNCcircle::DrawGLBottomFace(void) const
{
	if ( !m_obCdata.IsEmpty() ) {
		CNCdata::DrawGLBottomFace();
		return;
	}

	if ( GetEndmillType() == 0 ) {
		CPoint3D	ptc[ARCCOUNT+1];
		::glVertexPointer(3, GL_DOUBLE, 0, ptc);
		// énì_ÇÃ¥›ƒﬁ–Ÿâ~
		_SetEndmillOrgCircle(m_dEndmill, m_ptValS, ptc, 0);
		::glDrawRangeElements(GL_TRIANGLE_FAN,
			0, ARCCOUNT,
			SIZEOF(GLFanElement[0]), GL_UNSIGNED_SHORT, GLFanElement[0]);
		// èIì_ÇÃ¥›ƒﬁ–Ÿâ~
		_SetEndmillOrgCircle(m_dEndmill, m_ptValE, ptc, 0);
		::glDrawRangeElements(GL_TRIANGLE_FAN,
			0, ARCCOUNT,
			SIZEOF(GLFanElement[0]), GL_UNSIGNED_SHORT, GLFanElement[0]);
		// ãOê’ ﬂΩÇÃç¿ïWåvéZ
		if ( GetPlane()==XY_PLANE && m_dHelicalStep==0 ) {
			// XYíPèÉãÈå`ç¿ïWåvéZ
			CPoint3D	ptStrip[(ARCCOUNT+1)*2];	// â~ç¿ïWç≈ëÂíl
			int	nCnt = SetEndmillXYPath(ptStrip);
			// ï`âÊÅi¥›ƒﬁ–ŸíÜêSÇ©ÇÁí∑ï˚å` ﬂΩÅj
			::glVertexPointer(3, GL_DOUBLE, 0, ptStrip);
			::glDrawArrays(GL_TRIANGLE_STRIP, 0, nCnt);
		}
		else {
			// ãOê’è„ÇÃâ~Ç ﬂ≤ÃﬂèÛÇ…Ç¬Ç»ÇÆ
			SetEndmillPipe();
		}
	}
	else {
		CPoint3D	ptSphere[ARCCOUNT/4][ARCCOUNT];
		::glVertexPointer(3, GL_DOUBLE, 0, ptSphere);
		// énì_ÅEèIì_ÇÃŒﬁ∞Ÿ¥›ƒﬁ–ŸãÖ
		_SetEndmillSphere(m_dEndmill, m_ptValS, ptSphere);
		_DrawBottomFaceSphere();
		_SetEndmillSphere(m_dEndmill, m_ptValE, ptSphere);
		_DrawBottomFaceSphere();
		// ãOê’ ﬂΩÇÃç¿ïWåvéZ
		SetEndmillBall();
	}
}

int CNCcircle::SetEndmillXYPath(CPoint3D* lptStrip) const
{
	int		i = 0;
	double	sq, eq, st, h, r1, r2, rr = fabs(m_r);
	CPointD	ptOrg(m_ptOrg.GetXY());

	tie(sq, eq) = GetSqEq();

	// â~å ï‚ä‘êÿçÌ ﬂΩç¿ïW
	if ( m_nG23 == 0 ) {
		r1 = rr + m_dEndmill;	// êiçsï˚å¸ç∂ë§
		r2 = rr - m_dEndmill;	// êiçsï˚å¸âEë§
		st = (sq - eq) / ARCCOUNT;
//		for ( h=m_ptValS.z; i<ARCCOUNT*2&&sq>eq; i+=2, sq-=ARCSTEP, h+=m_dHelicalStep )
		for ( h=m_ptValS.z; i<ARCCOUNT*2&&sq>eq; i+=2, sq-=st, h+=m_dHelicalStep )
			_SetEndmillPathXY(ptOrg, sq, h, r1, r2, i, lptStrip);
	}
	else {
		r1 = rr - m_dEndmill;
		r2 = rr + m_dEndmill;
		st = (eq - sq) / ARCCOUNT;
//		for ( h=m_ptValS.z; i<ARCCOUNT*2&&sq<eq; i+=2, sq+=ARCSTEP, h+=m_dHelicalStep )
		for ( h=m_ptValS.z; i<ARCCOUNT*2&&sq<eq; i+=2, sq+=st, h+=m_dHelicalStep )
			_SetEndmillPathXY(ptOrg, sq, h, r1, r2, i, lptStrip);
	}
	// í[êîï™
	_SetEndmillPathXY(ptOrg, eq, m_ptValE.z, r1, r2, i, lptStrip);

	return i+2;
}

void CNCcircle::SetEndmillPipe(void) const
{
	size_t		i = 0;
	double		sq, eq, st, h, rr = fabs(m_r);
	CPointD		ptOrg(GetPlaneValue(m_ptOrg));
	CPoint3D	ptPipe[ARCCOUNT+1][ARCCOUNT];	// [â~1é¸+èIì_(ç≈ëÂíl)][â~1é¸]

	tie(sq, eq) = GetSqEq();

	// â~å ï‚ä‘êÿçÌ ﬂΩç¿ïW
	switch ( GetPlane() ) {
	case XY_PLANE:
		if ( m_nG23 == 0 ) {
			st = (sq - eq) / ARCCOUNT;
//			for ( h=m_ptValS.z; i<ARCCOUNT&&sq>eq; i++, sq-=ARCSTEP, h+=m_dHelicalStep )
			for ( h=m_ptValS.z; i<ARCCOUNT&&sq>eq; i++, sq-=st, h+=m_dHelicalStep )
				_SetEndmillPathXY_Pipe(ptOrg, sq, rr, h, m_dEndmill, ptPipe[i]);
		}
		else {
			st = (eq - sq) / ARCCOUNT;
//			for ( h=m_ptValS.z; i<ARCCOUNT&&sq<eq; i++, sq+=ARCSTEP, h+=m_dHelicalStep )
			for ( h=m_ptValS.z; i<ARCCOUNT&&sq<eq; i++, sq+=st, h+=m_dHelicalStep )
				_SetEndmillPathXY_Pipe(ptOrg, sq, rr, h, m_dEndmill, ptPipe[i]);
		}
		_SetEndmillPathXY_Pipe(ptOrg, eq, rr, h, m_dEndmill, ptPipe[i]);
		break;

	case XZ_PLANE:
		if ( m_nG23 == 0 ) {
			st = (sq - eq) / ARCCOUNT;
//			for ( h=m_ptValS.y; i<ARCCOUNT&&sq>eq; i++, sq-=ARCSTEP, h+=m_dHelicalStep )
			for ( h=m_ptValS.y; i<ARCCOUNT&&sq>eq; i++, sq-=st, h+=m_dHelicalStep )
				_SetEndmillPathXZ_Pipe(ptOrg, sq, rr, h, m_dEndmill, ptPipe[i]);
		}
		else {
			st = (eq - sq) / ARCCOUNT;
//			for ( h=m_ptValS.y; i<ARCCOUNT&&sq<eq; i++, sq+=ARCSTEP, h+=m_dHelicalStep )
			for ( h=m_ptValS.y; i<ARCCOUNT&&sq<eq; i++, sq+=st, h+=m_dHelicalStep )
				_SetEndmillPathXZ_Pipe(ptOrg, sq, rr, h, m_dEndmill, ptPipe[i]);
		}
		_SetEndmillPathXZ_Pipe(ptOrg, eq, rr, h, m_dEndmill, ptPipe[i]);
		break;

	case YZ_PLANE:
		if ( m_nG23 == 0 ) {
			st = (sq - eq) / ARCCOUNT;
//			for ( h=m_ptValS.x; i<ARCCOUNT&&sq>eq; i++, sq-=ARCSTEP, h+=m_dHelicalStep )
			for ( h=m_ptValS.x; i<ARCCOUNT&&sq>eq; i++, sq-=st, h+=m_dHelicalStep )
				_SetEndmillPathYZ_Pipe(ptOrg, sq, rr, h, m_dEndmill, ptPipe[i]);
		}
		else {
			st = (eq - sq) / ARCCOUNT;
//			for ( h=m_ptValS.x; i<ARCCOUNT&&sq<eq; i++, sq+=ARCSTEP, h+=m_dHelicalStep )
			for ( h=m_ptValS.x; i<ARCCOUNT&&sq<eq; i++, sq+=st, h+=m_dHelicalStep )
				_SetEndmillPathYZ_Pipe(ptOrg, sq, rr, h, m_dEndmill, ptPipe[i]);
		}
		_SetEndmillPathYZ_Pipe(ptOrg, eq, rr, h, m_dEndmill, ptPipe[i]);
		break;
	}

	// ãOê’è„Ç…ï¿Ç‘â~Ç ﬂ≤ÃﬂèÛÇ…Ç¬Ç»ÇÆ
	_DrawEndmillPipe(i+1, ARCCOUNT, ptPipe);
}

void CNCcircle::SetEndmillBall(void) const
{
	size_t		i = 0, j;
	double		sq, eq, st, qp, h, rr = fabs(m_r);
	CPointD		ptOrg(GetPlaneValue(m_ptOrg));
	CPoint3D	pt, ptPipe[ARCCOUNT+1][ARCCOUNT];

	tie(sq, eq) = GetSqEq();

	switch ( GetPlane() ) {
	case XY_PLANE:
		// â~å ãOê’è„Ç…Zï˚å¸ÇÃîºâ~ç¿ïWÇåvéZÅiíºê¸ï‚ä‘Ç∆ìØÇ∂ï`âÊÇ≈ÇnÇjÅj
		if ( m_nG23 == 0 ) {
			st = (sq - eq) / ARCCOUNT;
//			for ( h=m_ptValS.z; i<ARCCOUNT&&sq>eq; i++, sq-=ARCSTEP, h+=m_dHelicalStep ) {
			for ( h=m_ptValS.z; i<ARCCOUNT&&sq>eq; i++, sq-=st, h+=m_dHelicalStep ) {
				pt.x = rr * cos(sq) + ptOrg.x;
				pt.y = rr * sin(sq) + ptOrg.y;
				pt.z = h;
				_SetEndmillSpherePath(m_dEndmill, sq, pt, ptPipe[i]);
			}
		}
		else {
			st = (eq - sq) / ARCCOUNT;
//			for ( h=m_ptValS.z; i<ARCCOUNT&&sq<eq; i++, sq+=ARCSTEP, h+=m_dHelicalStep ) {
			for ( h=m_ptValS.z; i<ARCCOUNT&&sq<eq; i++, sq+=st, h+=m_dHelicalStep ) {
				pt.x = rr * cos(sq) + ptOrg.x;
				pt.y = rr * sin(sq) + ptOrg.y;
				pt.z = h;
				_SetEndmillSpherePath(m_dEndmill, sq, pt, ptPipe[i]);
			}
		}
		pt.x = rr * cos(eq) + ptOrg.x;
		pt.y = rr * sin(eq) + ptOrg.y;
		pt.z = h;
		_SetEndmillSpherePath(m_dEndmill, eq, pt, ptPipe[i]);
		j = ARCCOUNT/2+1;
		break;

	case XZ_PLANE:
		// â~å ãOê’è„Ç…â~å ãOê’ÇÃíÜêSÇ…åXÇ¢ÇΩâ~ç¿ïWÇåvéZ
		if ( m_nG23 == 0 ) {
			st = (sq - eq) / ARCCOUNT;
//			for ( h=m_ptValS.y; i<ARCCOUNT&&sq>eq; i++, sq-=ARCSTEP, h+=m_dHelicalStep ) {
			for ( h=m_ptValS.y; i<ARCCOUNT&&sq>eq; i++, sq-=st, h+=m_dHelicalStep ) {
				pt.x = rr * cos(sq) + ptOrg.x;
				pt.y = h;
				pt.z = rr * sin(sq) + ptOrg.y + m_dEndmill;
				qp = atan2(m_ptOrg.z-pt.z, m_ptOrg.x-pt.x);	// â~Ç™XYïΩñ Ç…ëŒÇµÇƒåXÇ≠äpìx
				_SetEndmillPathXZ_Sphere(m_dEndmill, qp, pt, ptPipe[i]);
			}
		}
		else {
			st = (eq - sq) / ARCCOUNT;
//			for ( h=m_ptValS.y; i<ARCCOUNT&&sq<eq; i++, sq+=ARCSTEP, h+=m_dHelicalStep ) {
			for ( h=m_ptValS.y; i<ARCCOUNT&&sq<eq; i++, sq+=st, h+=m_dHelicalStep ) {
				pt.x = rr * cos(sq) + ptOrg.x;
				pt.y = h;
				pt.z = rr * sin(sq) + ptOrg.y + m_dEndmill;
				qp = atan2(m_ptOrg.z-pt.z, m_ptOrg.x-pt.x);
				_SetEndmillPathXZ_Sphere(m_dEndmill, qp, pt, ptPipe[i]);
			}
		}
		pt.x = rr * cos(eq) + ptOrg.x;
		pt.y = h;
		pt.z = rr * sin(eq) + ptOrg.y + m_dEndmill;
		qp = atan2(m_ptOrg.z-pt.z, m_ptOrg.x-pt.x);
		_SetEndmillPathXZ_Sphere(m_dEndmill, qp, pt, ptPipe[i]);
		j = ARCCOUNT;
		break;

	case YZ_PLANE:
		if ( m_nG23 == 0 ) {
			st = (sq - eq) / ARCCOUNT;
//			for ( h=m_ptValS.x; i<ARCCOUNT&&sq>eq; i++, sq-=ARCSTEP, h+=m_dHelicalStep ) {
			for ( h=m_ptValS.x; i<ARCCOUNT&&sq>eq; i++, sq-=st, h+=m_dHelicalStep ) {
				pt.x = h;
				pt.y = rr * cos(sq) + ptOrg.x;
				pt.z = rr * sin(sq) + ptOrg.y + m_dEndmill;
				qp = atan2(m_ptOrg.z-pt.z, m_ptOrg.y-pt.y);
				_SetEndmillPathYZ_Sphere(m_dEndmill, qp, pt, ptPipe[i]);
			}
		}
		else {
			st = (eq - sq) / ARCCOUNT;
//			for ( h=m_ptValS.x; i<ARCCOUNT&&sq<eq; i++, sq+=ARCSTEP, h+=m_dHelicalStep ) {
			for ( h=m_ptValS.x; i<ARCCOUNT&&sq<eq; i++, sq+=st, h+=m_dHelicalStep ) {
				pt.x = h;
				pt.y = rr * cos(sq) + ptOrg.x;
				pt.z = rr * sin(sq) + ptOrg.y + m_dEndmill;
				qp = atan2(m_ptOrg.z-pt.z, m_ptOrg.y-pt.y);
				_SetEndmillPathYZ_Sphere(m_dEndmill, qp, pt, ptPipe[i]);
			}
		}
		pt.x = h;
		pt.y = rr * cos(eq) + ptOrg.x;
		pt.z = rr * sin(eq) + ptOrg.y + m_dEndmill;
		qp = atan2(m_ptOrg.z-pt.z, m_ptOrg.y-pt.y);
		_SetEndmillPathYZ_Sphere(m_dEndmill, qp, pt, ptPipe[i]);
		j = ARCCOUNT;
		break;
	}

	// ãOê’è„Ç…ï¿Ç‘îºâ~Ç‹ÇΩÇÕâ~Ç ﬂ≤ÃﬂèÛÇ…Ç¬Ç»ÇÆ
	_DrawEndmillPipe(i+1, j, ptPipe);
}

int CNCcircle::AddGLWireVertex(vector<GLfloat>& vVertex, vector<GLfloat>& vNormal) const
{
	if ( !m_pWireObj )
		return -1;

#ifdef _DEBUGOLD
	CMagaDbg	dbg;
#endif
	int			nCnt = 0;
	CNCcircle*	pCircleUV = static_cast<CNCcircle*>(m_pWireObj);
	double		sqxy, eqxy, squv, equv, stxy, stuv,
				cxy, sxy, cuv, suv,
				rxy = fabs(m_r), ruv = fabs(pCircleUV->GetR());
	CPointD		ptOrgXY( m_ptOrg.GetXY() ),
				ptOrgUV( pCircleUV->GetOrg().GetXY() );
	CPoint3D	pt1, pt2;

	tie(sqxy, eqxy) = GetSqEq();
	tie(squv, equv) = pCircleUV->GetSqEq();
	pt1.z = m_ptValS.z;
	pt2.z = pCircleUV->GetStartPoint().z;

	// ç¿ïWílÇâ~ìõèÛÇ…ìoò^
	// ñ@ê¸Õﬁ∏ƒŸÇÕ r - 1.0
	// -- XYé≤Ç∆UVé≤ÇÃâ~å ÇÃëÂÇ´Ç≥Ç™à·Ç§èÍçáÇ™Ç†ÇÈÇ∆Ç´ÇÃèàóùÇí«â¡
	if ( m_nG23 == 0 ) {
		stxy = (sqxy - eqxy) / ARCCOUNT;
		stuv = (squv - equv) / ARCCOUNT;
//		for ( ; sqxy>eqxy || squv>equv; sqxy-=ARCSTEP, squv-=ARCSTEP, nCnt+=2 ) {
		for ( ; sqxy>eqxy || squv>equv; sqxy-=stxy, squv-=stuv, nCnt+=2 ) {
			cxy = cos(max(sqxy, eqxy));	sxy = sin(max(sqxy, eqxy));
			cuv = cos(max(squv, equv));	suv = sin(max(squv, equv));
			pt1.x = rxy * cxy + ptOrgXY.x;
			pt1.y = rxy * sxy + ptOrgXY.y;
			pt2.x = ruv * cuv + ptOrgUV.x;
			pt2.y = ruv * suv + ptOrgUV.y;
			vVertex.push_back((GLfloat)pt1.x);
			vVertex.push_back((GLfloat)pt1.y);
			vVertex.push_back((GLfloat)pt1.z);
			vVertex.push_back((GLfloat)pt2.x);
			vVertex.push_back((GLfloat)pt2.y);
			vVertex.push_back((GLfloat)pt2.z);
			pt1.x = (rxy-1.0) * cxy + ptOrgXY.x;
			pt1.y = (rxy-1.0) * sxy + ptOrgXY.y;
			pt2.x = (ruv-1.0) * cuv + ptOrgUV.x;
			pt2.y = (ruv-1.0) * suv + ptOrgUV.y;
			vNormal.push_back((GLfloat)pt1.x);
			vNormal.push_back((GLfloat)pt1.y);
			vNormal.push_back((GLfloat)pt1.z);
			vNormal.push_back((GLfloat)pt2.x);
			vNormal.push_back((GLfloat)pt2.y);
			vNormal.push_back((GLfloat)pt2.z);
		}
	}
	else {
		stxy = (eqxy - sqxy) / ARCCOUNT;
		stuv = (equv - squv) / ARCCOUNT;
//		for ( ; sqxy<eqxy || squv<equv; sqxy+=ARCSTEP, squv+=ARCSTEP, nCnt+=2 ) {
		for ( ; sqxy<eqxy || squv<equv; sqxy+=stxy, squv+=stuv, nCnt+=2 ) {
			cxy = cos(min(sqxy, eqxy));	sxy = sin(min(sqxy, eqxy));
			cuv = cos(min(squv, equv));	suv = sin(min(squv, equv));
			pt1.x = rxy * cxy + ptOrgXY.x;
			pt1.y = rxy * sxy + ptOrgXY.y;
			pt2.x = ruv * cuv + ptOrgUV.x;
			pt2.y = ruv * suv + ptOrgUV.y;
			vVertex.push_back((GLfloat)pt1.x);
			vVertex.push_back((GLfloat)pt1.y);
			vVertex.push_back((GLfloat)pt1.z);
			vVertex.push_back((GLfloat)pt2.x);
			vVertex.push_back((GLfloat)pt2.y);
			vVertex.push_back((GLfloat)pt2.z);
			pt1.x = (rxy-1.0) * cxy + ptOrgXY.x;
			pt1.y = (rxy-1.0) * sxy + ptOrgXY.y;
			pt2.x = (ruv-1.0) * cuv + ptOrgUV.x;
			pt2.y = (ruv-1.0) * suv + ptOrgUV.y;
			vNormal.push_back((GLfloat)pt1.x);
			vNormal.push_back((GLfloat)pt1.y);
			vNormal.push_back((GLfloat)pt1.z);
			vNormal.push_back((GLfloat)pt2.x);
			vNormal.push_back((GLfloat)pt2.y);
			vNormal.push_back((GLfloat)pt2.z);
		}
	}
	cxy = cos(eqxy);	sxy = sin(eqxy);
	cuv = cos(equv);	suv = sin(equv);
	pt1.x = rxy * cxy + ptOrgXY.x;
	pt1.y = rxy * sxy + ptOrgXY.y;
	pt2.x = ruv * cuv + ptOrgUV.x;
	pt2.y = ruv * suv + ptOrgUV.y;
	vVertex.push_back((GLfloat)pt1.x);
	vVertex.push_back((GLfloat)pt1.y);
	vVertex.push_back((GLfloat)pt1.z);
	vVertex.push_back((GLfloat)pt2.x);
	vVertex.push_back((GLfloat)pt2.y);
	vVertex.push_back((GLfloat)pt2.z);
	pt1.x = (rxy-1.0) * cxy + ptOrgXY.x;
	pt1.y = (rxy-1.0) * sxy + ptOrgXY.y;
	pt2.x = (ruv-1.0) * cuv + ptOrgUV.x;
	pt2.y = (ruv-1.0) * suv + ptOrgUV.y;
	vNormal.push_back((GLfloat)pt1.x);
	vNormal.push_back((GLfloat)pt1.y);
	vNormal.push_back((GLfloat)pt1.z);
	vNormal.push_back((GLfloat)pt2.x);
	vNormal.push_back((GLfloat)pt2.y);
	vNormal.push_back((GLfloat)pt2.z);

#ifdef _DEBUGOLD
	dbg.printf("DrawCnt=%d", nCnt/2+1);
#endif

	return nCnt+2;		// í∏ì_êî
}

int CNCcircle::AddGLWireTexture(int n, double& dAccuLength, double dAllLength, GLfloat* pfTEX) const
{
	if ( !m_pWireObj )
		return -1;

	int			nCnt = 0;
	CNCcircle*	pCircleUV = static_cast<CNCcircle*>(m_pWireObj);
	GLfloat		f;
	double		sqxy = m_sq, eqxy = m_eq,
				squv = pCircleUV->GetStartAngle(), equv = pCircleUV->GetEndAngle(),
				stxy = fabs(sqxy - eqxy) / ARCCOUNT,
				stuv = fabs(squv - equv) / ARCCOUNT,
				rxy = fabs(m_r), ruv = fabs(pCircleUV->GetR());

	// √∏Ω¡¨ç¿ïWÇÃìoò^ÇÕÅAâÒì]ï˚å¸ÇÕä÷åWÇ»Ç≠ÅAí∑Ç≥ÇÃäÑçáÇæÇØÇ≈ó«Ç¢
//	for ( ; sqxy<eqxy || squv<equv; sqxy+=ARCSTEP, squv+=ARCSTEP, nCnt+=4 ) {
	for ( ; sqxy<eqxy || squv<equv; sqxy+=stxy, squv+=stuv, nCnt+=4 ) {
		f = (GLfloat)(dAccuLength / dAllLength);
		pfTEX[n++] = f;
		pfTEX[n++] = 0.0;
		pfTEX[n++] = f;
		pfTEX[n++] = 1.0;
		dAccuLength += rxy*ARCSTEP;
	}
	f = (GLfloat)(dAccuLength / dAllLength);
	pfTEX[n++] = f;
	pfTEX[n++] = 0.0;
	pfTEX[n++] = f;
	pfTEX[n  ] = 1.0;

	return nCnt+4;
}
