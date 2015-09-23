//
//  VmusImage.cpp
//  ReadStaff
//
//  Created by yanbin on 14-8-6.
//  Modified by jiangnan on 15-5-12
//

#include "ParseExport.h"

#ifdef TARGET_OS_IPHONE
//#define NEW_PAGE_MODE
#else
#define NEW_PAGE_MODE
#endif

bool isRetinaDevice = false;

double round(double val)
{
	return (val > 0.0) ? floor(val+0.5) : ceil(val-0.5);
}

std::string multichar2utf8(const std::string& m_string)
{
#ifdef	WIN32
	//Calculate the length of unicode encoding
	int len = MultiByteToWideChar(CP_ACP, 0, (LPCTSTR)m_string.c_str(), -1, NULL, 0);		//CP_ACP indicates convert to unicode encoding
	wchar_t* w_string = (wchar_t*)malloc(2*len+2);
	memset(w_string, 0, 2*len+2);
	//Convert to unicode encoding from ansi
	MultiByteToWideChar(CP_ACP, 0, (LPCTSTR)m_string.c_str(), -1, w_string, len);
	//Calculate the length of utf8 encoding
	len = WideCharToMultiByte(CP_UTF8, 0, w_string, -1, NULL, 0, NULL, NULL);	//CP_UTF8 indicates convert to utf8 encoding
	char* utf8_string = (char*)malloc(len+1);
	memset(utf8_string, 0, len+1);
	//Convert to utf8 from unicode
	WideCharToMultiByte(CP_UTF8, 0, w_string, -1, utf8_string, len, NULL, NULL);
	free(w_string);
	std::string str(utf8_string);
	free(utf8_string);
	return str;
#else
	return m_string;
#endif
}

CGSize GetFontPixelSize(const std::string& text, size_t nFontSize)
{
	CGSize sizeTextSmart;
	if (text.empty() || !nFontSize)
		return sizeTextSmart;
#ifdef	WIN32
	HDC hDCScreen = GetDC(NULL);
	HDC hDCMem = CreateCompatibleDC(hDCScreen);
	ReleaseDC(NULL, hDCScreen);

	LOGFONT font_settings;
	font_settings.lfHeight = GetDeviceCaps(hDCMem, LOGPIXELSY)*nFontSize/(72+6);
	font_settings.lfWidth = 0;
	font_settings.lfEscapement = 0;
	font_settings.lfOrientation = 0;
	font_settings.lfWeight = FW_NORMAL;
	font_settings.lfItalic = FALSE;
	font_settings.lfUnderline = FALSE;
	font_settings.lfStrikeOut = FALSE;
	font_settings.lfCharSet = GB2312_CHARSET;
	font_settings.lfOutPrecision = OUT_STRING_PRECIS;
	font_settings.lfClipPrecision = CLIP_CHARACTER_PRECIS;
	font_settings.lfQuality = DRAFT_QUALITY;
	font_settings.lfPitchAndFamily = FF_SWISS | VARIABLE_PITCH;
	lstrcpyn(font_settings.lfFaceName,_T("Courier New"),sizeof(font_settings.lfFaceName)/sizeof(TCHAR));

	HFONT hFont = CreateFontIndirect(&font_settings);
	HFONT hFontOld = (HFONT)SelectObject(hDCMem, hFont);
	//calculate the width of the string in logical unit using the classic way
	SIZE sizeText;
	GetTextExtentPoint32(hDCMem, text.c_str(), text.size(), &sizeText);
	sizeTextSmart.height = sizeText.cy;
	sizeTextSmart.width = sizeText.cx;

	//Set the right-limit of the bounding rectangle to be scanned to just 'sizeText.cx+widthOfTheLastCharacter'
	SIZE sizeLastCharacter;
	GetTextExtentPoint32(hDCMem, text.substr(text.size()-1).c_str(), 1, &sizeLastCharacter);

	//enough to fit the text
	RECT rect = {0, 0, sizeTextSmart.width+sizeLastCharacter.cx, sizeTextSmart.height};
	HBITMAP hBitmap = CreateCompatibleBitmap(hDCMem, rect.right-rect.left, rect.bottom-rect.top);
	HBITMAP hBitmapOld = (HBITMAP)SelectObject(hDCMem, hBitmap);

	//fill with white
	FillRect(hDCMem, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
	DrawText(hDCMem, text.c_str(), -1, &rect, DT_LEFT|DT_TOP|DT_SINGLELINE|DT_NOPREFIX);

	int iXmax = 0;
	bool bFound = FALSE;
	for (int x = rect.right-1; x >= 0 && !bFound; x--)
	{
		for (int y = 0; y <= rect.bottom-1 && !bFound; y++)
		{
			COLORREF rgbColor = GetPixel(hDCMem, x, y);
			if (rgbColor != RGB(255, 255, 255))
			{
				//found a non-white pixel, save the horizontal position and
				//exit the loop. Job finished.
				iXmax = x;
				bFound = TRUE;
			}
		}
	}
	sizeTextSmart.width = iXmax+1;		//+1 because we use 0-based indexes

	SelectObject(hDCMem, hFontOld);
	SelectObject(hDCMem, hBitmapOld);
	DeleteObject(hBitmap);
	DeleteObject(hFont);
	DeleteDC(hDCMem);
#else
	//stub...
#endif
	return sizeTextSmart;
}

std::string& trim(std::string& s)
{
	if (s.empty())
		return s;

	s.erase(0, s.find_first_not_of("\r\n "));
	s.erase(s.find_last_not_of("\r\n ")+1);
	return s;
}

VmusImage::VmusImage()
{
	screen_width = 0;
	screen_height = 0;
	densitydpi=0;
	isLandscape = false;
	GROUP_STAFF_MID = 0;
	STAFF_HEADER_WIDTH = 0;
	GROUP_STAFF_NEXT = 0;
	memset(STAFF_OFFSET, 0, sizeof(int)*20);
	BARLINE_WIDTH = 0;
	BEAM_WIDTH = 0;
	OFFSET_X_UNIT = OFFSET_Y_UNIT = 0;
	ending_x1 = ending_y1 = 0;

	music = nullptr;
	fontContent = nullptr;
	staff_images = nullptr;
	STAFF_COUNT = 0;
	start_numerator = 0;
	start_denominator = 0;
	minNoteValue = maxNoteValue = 0;

	pageMode = true;
	showJianpu=false;
	start_tempo_num=108;
	start_tempo_type=1.0/4;

#ifdef OVE_IPHONE
	LINE_H=6;
	MARGIN_LEFT=10;
	MARGIN_RIGHT=5;
	MARGIN_TOP=30;
#else
	MARGIN_LEFT=40;
	MARGIN_RIGHT=30;
	MARGIN_TOP=40;//100;
	LINE_H=12;//7.5;
#endif

	measure_pos = new std::vector<MeasurePos>();
    svgXmlContent=nullptr;
	svgMeasurePosContent = nullptr;
	svgForceCurveContent = nullptr;
	svgXmlJianpuContent = nullptr;
	svgXmlJianpuFixDoContent = nullptr;
	svgXmlJianwpContent = nullptr;
	svgXmlJianwpFixDoContent = nullptr;
}

VmusImage::~VmusImage()
{
	if (measure_pos)
	{
		delete measure_pos;
		measure_pos = nullptr;
	}
	if (svgMeasurePosContent)
	{
		delete svgMeasurePosContent;
		svgMeasurePosContent = nullptr;
	}
	if (svgForceCurveContent)
	{
		delete svgForceCurveContent;
		svgForceCurveContent = nullptr;
	}
	if (svgXmlJianpuContent)
	{
		delete svgXmlJianpuContent;
		svgXmlJianpuContent = nullptr;
	}
	if (svgXmlJianpuFixDoContent)
	{
		delete svgXmlJianpuFixDoContent;
		svgXmlJianpuFixDoContent = nullptr;
	}
	if (svgXmlJianwpContent)
	{
		delete svgXmlJianwpContent;
		svgXmlJianwpContent = nullptr;
	}
	if (svgXmlJianwpFixDoContent)
	{
		delete svgXmlJianwpFixDoContent;
		svgXmlJianwpFixDoContent = nullptr;
	}
	Delete_MyArray(MyString, staff_images);
}

void VmusImage::loadMusic(VmusMusic* music, const CGSize& musicSize, bool landPage, const std::vector<Event>* midi_events)
{
	landPageMode = landPage;
	MidiEvents = midi_events;
	CGSize screenSize;
	screenSize.width = 1027;
	screenSize.height = 768;
	loadMusic(music, musicSize,screenSize);
}

void VmusImage::loadMusic(VmusMusic* music, const CGSize& musicSize, const CGSize& screenSize)
{
#ifdef ADAPT_CUSTOMIZED_SCREEN
	LINE_H=musicSize.width/175;
#else
	LINE_H=musicSize.width/120;		//7.5
#endif
	MARGIN_LEFT=LINE_H*4;
	MARGIN_RIGHT=LINE_H*3;
	MARGIN_TOP=LINE_H*4;
	if (LINE_H > 10) {
		BARLINE_WIDTH = 3;
		BEAM_WIDTH = 4;
	} else {
		BARLINE_WIDTH = 2;
		BEAM_WIDTH = 4;
	}

    densitydpi=128;
    screen_width=musicSize.width;
    screen_height=musicSize.height;
	page_height = musicSize.height;
	real_screen_size = screenSize;
    this->music=music;
	if (this->music)
		drawSvgMusic();
}

#define YIYIN_ZOOM 0.6
#define GRACE_X_OFFSET 0

#ifdef OVE_IPHONE
#define MEAS_LEFT_MARGIN (LINE_H) //10
#define MEAS_RIGHT_MARGIN (LINE_H) //10

#define BARLINE_WIDTH 2
#define BEAM_DISTANCE 5
#define BEAM_WIDTH 2.0
#define WAVY_LINE_WIDTH 1
#define SLUR_LINE_WIDTH 1
#define EXPR_FONT_SIZE  16
#define TITLE_FONT_SIZE 22
#define NORMAL_FONT_SIZE 16
#define JIANPU_FONT_SIZE 20
#else

#define MEAS_LEFT_MARGIN  (LINE_H*2.5)//(LINE_H*2)//20
#define MEAS_RIGHT_MARGIN (LINE_H*2.5)//(LINE_H*2)//20

//#define BARLINE_WIDTH 3
#define BEAM_DISTANCE (1.5*BEAM_WIDTH)
#define GLYPH_FONT_SIZE 20
#define GLYPH_FLAG_SIZE (GLYPH_FONT_SIZE*0.7)
#define GLYPH_FINGER_SIZE (GLYPH_FONT_SIZE*0.6)

#define TREMOLO_LINE_WIDTH 3
#define WAVY_LINE_WIDTH 2
#define SLUR_LINE_WIDTH 1
#define EXPR_FONT_SIZE  (LINE_H*2.0)//20
#define TITLE_FONT_SIZE (LINE_H*4.0)//26
#define NORMAL_FONT_SIZE  (LINE_H*1.6)//19
#define JIANPU_FONT_SIZE (LINE_H*2.2)//26
#endif

//#define fillColor "fill='white'"
//#define strokeColor "stroke='white'"
//#define bgColor "style=\"background-color:#A78464\"" //��ɫ
#define fillColor "fill='black'"
#define strokeColor "stroke='black'"
//#define bgColor "style=\"background-color:#FEFBEB\"" //77music��
//#define bgColor "style=\"background-color:#D1A072\"" // ��ɫţƤֽ
//#define bgColor "style=\"background-color:#E0C794\"" // ţƤֽ
//#define bgColor "style=\"background-color:#EFDAB0\"" //ǳɫţƤֽ

#ifdef TARGET_OS_IPHONE
#define bgColor "style=\"background-color:#F1E0B7\"" //ǳɫţƤֽ
#else
#define bgColor "style=\"background-color:#AEABAB\""		//��ɫ
#endif

#define grayColor		"#FEFBEB"
#define paperColor		"#F1E0B7"

#define SLUR_CONTINUE_NUM 8
struct SlurContinueInfo
{
    bool above, validate;
    int stop_measure, stop_offset;
    int right_line;
    int stop_staff;

	SlurContinueInfo()
	{
		memset(this, 0, sizeof(SlurContinueInfo));
	}
} slur_continue_info[SLUR_CONTINUE_NUM];

#define OCTAVE_CONTINUE_NUM 2
struct OctaveContinueInfo
{
	bool validate;
	int offset_y,staff, start_line;
	int octave_x1,octave_y1;

	OctaveContinueInfo()
	{
		memset(this, 0, sizeof(OctaveContinueInfo));
	}
} octave_continue_info[OCTAVE_CONTINUE_NUM];

#define GROUP(x,y,size) \
{   \
	char tmp[256]; \
	sprintf(tmp,"<text x='%d' y='%d' font-size='%d' "fillColor" style=\"font-family: 'Aloisen New';\">group</text>\n", (int)(x), (int)(y), (int)(size)); \
	svgXmlContent->appendString(tmp); \
}

#define GLYPH_Petrucci(x,y,size,rotate,glyph) \
{   \
	char tmp[256]; \
	sprintf(tmp,"<text x='%.1f' y='%.1f' font-size='%d' style=\"font-family: 'Aloisen New';\" "fillColor">&#x1%s;</text>\n", (float)(x), (float)(y), (int)(size*LINE_H*0.1),glyph); \
	svgXmlContent->appendString(tmp); \
}

#define  GLYPH_Petrucci_id(x, y, size, glyph, ID) \
{   \
	char tmp[256]; \
	sprintf(tmp,"<text id='%s' x='%.1f' y='%.1f' font-size='%d' style=\"font-family: 'Aloisen New';\" "fillColor">&#x1%s;</text>\n",ID, (float)(x), (float)(y), (int)(size*LINE_H*0.1),glyph); \
	svgXmlContent->appendString(tmp); \
}

#define	GLYPH_Petrucci_index(x, y, size, glyph, m, n, e)	\
{	\
	char tmp[256];	\
	sprintf(tmp, "<text id='%d_%d_%d' x='%.1f' y='%.1f' font-size='%d' style=\"font-family: 'Aloisen New';\" onclick='clickNote(this,%d,%d,%d)'>&#x1%s;</text>\n",m,n,e, (float)(x), (float)(y), (int)(size*LINE_H*0.1),m,n,e,glyph);	\
	svgXmlContent->appendString(tmp);	\
}

#define GLYPH_Petrucci_rotate(x,y,size,rotate,glyph) \
{   \
	char tmp[256]; \
	sprintf(tmp,"<text x='%.1f' y='%.1f' font-size='%d' style=\"font-family: 'Aloisen New';\" rotate='%d'>&#x1%s;</text>\n", (float)(x), (float)(y), (int)(size*LINE_H*0.1),rotate,glyph); \
	svgXmlContent->appendString(tmp); \
}

#define ELEM_WAVY "ac"
#define LINE_WAVY_VERTICAL(x, y1, y2, w) svgWavyVertical(x,y1,y2,w)
#define svgWavyVertical(x, y1, y2, w) \
{   \
    if (y1>y2) {    \
        int temp=y2;    \
        y2=y1;  \
        y1=temp;    \
    }   \
    int count=(y2+2.8*LINE_H-y1)/LINE_H;    \
    for(int n=0;n<count;n++)    \
    {   \
        GLYPH_Petrucci(x,y1+n*LINE_H,GLYPH_FLAG_SIZE,0,ELEM_WAVY);   \
    }   \
}

#define ELEM_WAVY_HORIZONTAL "e8"
#define LINE_WAVY_HORIZONTAL(x1, x2, y) svgWavyHorizontal(y,x1,x2)
#define svgWavyHorizontal(y,x1,x2) \
{   \
    for(int n=0;n<(x2-x1)/LINE_H;n++)   \
    {   \
        GLYPH_Petrucci(x1+n*LINE_H,y,GLYPH_FLAG_SIZE,0,ELEM_WAVY_HORIZONTAL);    \
    }   \
}

// http://en.wikipedia.org/wiki/List_of_musical_symbols
#define FIVE_LINES(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE,0,"3d") //=

#define Treble  "80" //@"26"
#define Bass    "81" //@"3f"
#define Middle  "82" //@"26"
#define Percussion1 "83"

//#define CLEF_TREBLE(x,y,zoom) GLYPH_Petrucci(x,y,40*zoom,0,"26") //&
#define CLEF_TREBLE(x,y,zoom) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE*zoom,0,Treble) //&
#define CLEF_BASS(x,y, zoom) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE*zoom,0,Bass) //?
#define CLEF_MID(x,y, zoom) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE*zoom,0,Middle) //B
#define CLEF_Percussion1(x,y, zoom) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE*zoom,0,Percussion1) //B

#define ELEM_TIME_SIGNATURE_COMMON_TIME "8a" //4/4��
#define ELEM_TIME_SIGNATURE_CUT_TIME "8b" //2/2��

#define ELEM_FLAG_SHARP "21"
#define ELEM_FLAG_FLAT  "22"
#define ELEM_FLAG_STOP  "23"
#define ELEM_FLAG_DOUBLE_SHARP "24"
#define ELEM_FLAG_DOUBLE_FLAT "25"
#define ELEM_FLAG_SHARP_CAUTION "26"
#define ELEM_FLAG_FLAT_CAUTION "27"
#define ELEM_FLAG_STOP_CAUTION "28"
#define ELEM_FLAG_DOUBLE_SHARP_CAUTION "29"
#define ELEM_FLAG_DOUBLE_FLAT_CAUTION "2a"

#define FLAG_SHARP(x,y,zoom) GLYPH_Petrucci(x,y,GLYPH_FLAG_SIZE*zoom,0,ELEM_FLAG_SHARP) //#
#define FLAG_SHARP_CAUTION(x,y) GLYPH_Petrucci(x,y,GLYPH_FLAG_SIZE,0,ELEM_FLAG_SHARP_CAUTION) //[ 123
#define FLAG_DOUBLE_SHARP(x,y,zoom) GLYPH_Petrucci(x,y,GLYPH_FLAG_SIZE*zoom,0,ELEM_FLAG_DOUBLE_SHARP) //0220
//#define FLAG_DOUBLE_SHARP(x,y) GLYPH_Petrucci(x,y,30,0,ELEM_FLAG_DOUBLE_SHARP) //0220
#define FLAG_DOUBLE_SHARP_CAUTION(x,y) GLYPH_Petrucci(x,y,GLYPH_FLAG_SIZE,0,ELEM_FLAG_DOUBLE_SHARP_CAUTION) // ]

#define FLAG_FLAT(x,y,zoom) GLYPH_Petrucci(x,y,GLYPH_FLAG_SIZE*zoom,0,ELEM_FLAG_FLAT) //b 98
#define FLAG_FLAT_CAUTION(x,y) GLYPH_Petrucci(x,y,GLYPH_FLAG_SIZE,0,ELEM_FLAG_FLAT_CAUTION) //{ 123
#define FLAG_DOUBLE_FLAT(x,y) GLYPH_Petrucci(x,y,GLYPH_FLAG_SIZE,0,ELEM_FLAG_DOUBLE_FLAT) //0186
#define FLAG_DOUBLE_FLAT_CAUTION(x,y) GLYPH_Petrucci(x,y,GLYPH_FLAG_SIZE,0,ELEM_FLAG_DOUBLE_FLAT_CAUTION) //0211

#define FLAG_STOP(x,y,zoom) GLYPH_Petrucci(x,y,GLYPH_FLAG_SIZE*zoom,0,ELEM_FLAG_STOP) //n
#define FLAG_STOP_CAUTION(x,y, zoom) GLYPH_Petrucci(x,y,GLYPH_FLAG_SIZE*zoom,0,ELEM_FLAG_STOP_CAUTION) //N

#define ELEM_NOTE_QUADRUPLE_WHOLE      "69"
#define ELEM_NOTE_DOUBLE_WHOLE      "40"
#define ELEM_NOTE_FULL      "41"
#define ELEM_NOTE_2_UP      "42"
#define ELEM_NOTE_4_UP      "43"
#define ELEM_NOTE_8_UP      "44"
#define ELEM_NOTE_16_UP     "45"
#define ELEM_NOTE_32_UP     "46"
#define ELEM_NOTE_64_UP     "47"
#define ELEM_NOTE_128_UP     "48"

#define ELEM_NOTE_2_DOWN    "52"//@"48"
#define ELEM_NOTE_4_DOWN    "53"//@"51"
#define ELEM_NOTE_8_DOWN    "54"//@"45"
#define ELEM_NOTE_16_DOWN   "55"//@"58"
#define ELEM_NOTE_32_DOWN   "56"
#define ELEM_NOTE_64_DOWN   "57"
#define ELEM_NOTE_128_DOWN   "58"

#define ELEM_NOTE_2   "7c" //@"7c"
#define ELEM_NOTE_4   "74" //@"74"

#define ELEM_NOTE_OpenHiHat "51"
#define ELEM_NOTE_CloseHiHat "4e"

#define NOTE(x,y,zoom, ELEM, ID) GLYPH_Petrucci_id(x,y-1,GLYPH_FONT_SIZE*zoom,ELEM, ID)
#define NOTE_Index(x,y,zoom, ELEM, m,n,e) GLYPH_Petrucci_index(x,y,GLYPH_FONT_SIZE*zoom,ELEM, m,n,e)

#define NOTE_FULL(x,y,zoom) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE*zoom,0,ELEM_NOTE_FULL) //w
#define NOTE_2(x,y,zoom)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE*zoom,0,ELEM_NOTE_2) //0250
#define NOTE_2_UP(x,y,zoom)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE*zoom,0,ELEM_NOTE_2_UP) //h
#define NOTE_2_DOWN(x,y,zoom)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE*zoom,0,ELEM_NOTE_2_DOWN) //H
#define NOTE_4(x,y,zoom)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE*zoom,0,ELEM_NOTE_4) //0x0207
#define NOTE_4_UP(x,y,zoom)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE*zoom,0,ELEM_NOTE_4_UP)//q
#define NOTE_4_DOWN(x,y,zoom)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE*zoom,0,ELEM_NOTE_4_DOWN)//Q
#define NOTE_8_UP(x,y,zoom)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE*zoom,0,ELEM_NOTE_8_UP)//e
#define NOTE_8_DOWN(x,y,zoom)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE*zoom,0,ELEM_NOTE_8_DOWN)//E
#define NOTE_16_UP(x,y,zoom)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE*zoom,0,ELEM_NOTE_16_UP)//x
#define NOTE_16_DOWN(x,y,zoom)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE*zoom,0,ELEM_NOTE_16_DOWN)//X

#define NOTE_32_UP(x,y,zoom)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE*zoom,0,ELEM_NOTE_32_UP) //GLYPH_Petrucci2(x,y,40*zoom,0,@"78;&#xf0fb");
#define NOTE_64_UP(x,y,zoom)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE*zoom,0,ELEM_NOTE_64_UP) //GLYPH_Petrucci2(x,y,40*zoom,0,@"78;&#xf0fb;&#xf0fb");
#define NOTE_128_UP(x,y,zoom)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE*zoom,0,ELEM_NOTE_128_UP)//GLYPH_Petrucci2(x,y,40*zoom,0,@"78;&#xf0fb;&#xf0fb;&#xf0fb");

#define NOTE_OpenHiHat(x,y,zoom)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE*zoom,0,ELEM_NOTE_OpenHiHat)
#define NOTE_CloseHiHat(x,y,zoom)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE*zoom,0,ELEM_NOTE_CloseHiHat)

/*
<text style="fill:#000000;font-family:Arial;font-weight:bold;font-size:40">
<tspan x="50" y="60,70,80,80,75,60,80,70">COMMUNICATION</tspan>
<tspan x="50" y="150" dx="0,15" dy="10,10,10,-10,-10,-10,10,10,-10">COMMUNICATION</tspan>
<tspan x="50" y="230" rotate="10,20,30,40,50,60,70,80,90,90,90,90,90">COMMUNICATION</tspan>
</text>
*/
#define NOTE_32_DOWN(x,y,zoom)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE*zoom,0,ELEM_NOTE_32_DOWN) //GLYPH_Petrucci3(x,y,40*zoom,0,@"58;&#xf0f0")
#define NOTE_64_DOWN(x,y,zoom)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE*zoom,0,ELEM_NOTE_64_DOWN) //GLYPH_Petrucci3(x,y,40*zoom,0,@"58;&#xf0f0;&#xf0f0")
#define NOTE_128_DOWN(x,y,zoom)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE*zoom,0,ELEM_NOTE_128_DOWN) //GLYPH_Petrucci3(x,y,40*zoom,0,@"58;&#xf0f0;&#xf0f0;&#xf0f0")

#define NOTE_DOT(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "c7") //@"6b") //2e,6b
#define NORMAL_DOT(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "c7") //@"2e") //2e,6b

#define TAIL_EIGHT_UP(x,y, zoom)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE*zoom,0,"49") //GLYPH_Petrucci(x,y,40*zoom,0,@"6a")//j,251,K
#define TAIL_16_UP_ZOOM(x,y,zoom)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE*zoom,0,"4a") //GLYPH_Petrucci_Tails_up(x,y,40*zoom,@"6a;&#xf0fb")//r
#define TAIL_16_UP(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE,0,"4a") //GLYPH_Petrucci_Tails_up(x,y,40,@"6a;&#xf0fb")//r
#define TAIL_32_UP(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE,0,"4b") //GLYPH_Petrucci_Tails_up(x,y,40,@"6a;&#xf0fb;&#xf0fb")
#define TAIL_64_UP(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE,0,"4c") //GLYPH_Petrucci_Tails_up(x,y,40,@"6a;&#xf0fb;&#xf0fb;&#xf0fb")
#define TAIL_128_UP(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE,0,"4d")

#define TAIL_EIGHT_DOWN(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE,0,"59") //GLYPH_Petrucci(x,y,40,0,@"4A")//J,239,240
#define TAIL_16_DOWN(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE,0,"5a") //GLYPH_Petrucci_Tails_down(x,y,40,@"4a;&#xf0f0")//R
#define TAIL_32_DOWN(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE,0,"5b") //GLYPH_Petrucci_Tails_down(x,y,40,@"4a;&#xf0f0;&#xf0f0")
#define TAIL_64_DOWN(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE,0,"5c") //GLYPH_Petrucci_Tails_down(x,y,40,@"4a;&#xf0f0;&#xf0f0;&#xf0f0")
#define TAIL_128_DOWN(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE,0,"5d")

#define RESET_QUARTER(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "63") //@"\x63\x00") //@"ce") //206
#define RESET_EIGHT(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "64") //@"\x64\x00") //@"e4") //228
#define RESET_16(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "65") //@"\x65\x00") //@"c5")//197
#define RESET_32(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "66") //@"\x66\x00") //@"a8")//168
#define RESET_64(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "67") //@"\x67\x00") //@"f4")//244
#define RESET_128(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "68") //@"\x68\x00") //@"e5")//229

#define ELEM_P      "90"//@"70" //Piano
#define ELEM_PP     "91"//@"b9" //Pianissimo
#define ELEM_PPP    "92"//@"b8" //Pianississimo
#define ELEM_PPPP   "93"//@"af"  
#define ELEM_MP     "94"//@"50" //Mezzo piano
#define ELEM_F      "95"//@"66" //Forte
#define ELEM_FF     "96"//@"c4" //Fortissimo
#define ELEM_FFF    "97"//@"ec" //Fortississimo
#define ELEM_FFFF   "98"//@"eb"
#define ELEM_MF     "99"//@"46" //Mezzo forte
#define ELEM_SF     "9a"//@"53"
//#define ELEM_SFF    "9a95"
#define ELEM_FZ     "9b"//@"5a"
#define ELEM_SFZ    "9c"//@"a7" //Sforzando
#define ELEM_FP     "9d"//@"ea" //Forte-piano

#define DYNAMICS_PPPP(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, ELEM_PPPP)//175
#define DYNAMICS_PPP(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, ELEM_PPP)//184
#define DYNAMICS_PP(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, ELEM_PP)//185
#define DYNAMICS_P(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, ELEM_P)//p
#define DYNAMICS_MP(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, ELEM_MP)//P
#define DYNAMICS_MF(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, ELEM_MF)//F
#define DYNAMICS_F(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, ELEM_F)//f
#define DYNAMICS_FF(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, ELEM_FF)//196
#define DYNAMICS_FFF(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, ELEM_FFF)//236
#define DYNAMICS_FFFF(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, ELEM_FFFF)//235
#define DYNAMICS_S(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "73")//s
#define DYNAMICS_SF(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, ELEM_SF)//S
#define DYNAMICS_SFF(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, ELEM_SF);GLYPH_Petrucci(x+GLYPH_FONT_SIZE/2,y,GLYPH_FONT_SIZE, 0, ELEM_F)//S
#define DYNAMICS_Z(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "7a")//z
#define DYNAMICS_FZ(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, ELEM_FZ)//Z
#define DYNAMICS_SFZ(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, ELEM_SFZ)//167
#define DYNAMICS_SFFZ(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "8d")//141
#define DYNAMICS_FP(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, ELEM_FP)//234
#define DYNAMICS_SFP(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "9a90")//130
#define DYNAMICS_SFPP(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "9a91")//182

#define TEXT_NUM_ALL(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "30;&#xf031;&#xf032;&#xf033;&#xf034;&#xf035;&#xf036;&#xf037;&#xf038;&#xf039")
#define TEXT_NUM_0(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "30") //num=0-9
#define TEXT_NUM_1(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "31") //num=0-9
#define TEXT_NUM_2(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "32") //num=0-9
#define TEXT_NUM_3(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "33") //num=0-9
#define TEXT_NUM_4(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "34") //num=0-9
#define TEXT_NUM_5(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "35") //num=0-9
#define TEXT_NUM_6(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "36") //num=0-9
#define TEXT_NUM_7(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "37") //num=0-9
#define TEXT_NUM_8(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "38") //num=0-9
#define TEXT_NUM_9(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "39") //num=0-9
#define TEXT_NUM(x,y,num) TEXT_NUM_##num(x,y) //num=0-9

//http://en.wikipedia.org/wiki/Dal_Segno
#define REPEAT_SEGNO(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "b0")//@"25")
#define REPEAT_CODA(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "b1")//@"de")
//http://en.wikipedia.org/wiki/Category:Musical_notation

//Staccato���� ��������һ���� 
#define ART_STACCATO(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "c7")//@"6b") //2e,6b
//Tenuto ������ ��������һ������
#define ART_TENUTO(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "c8")//@"2d") //2d
//Accent //Marcato ����/���� ��������һ�����ںš�>��
#define ART_MARCATO(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "c0")//@"3e") //3e
//Marcato_Dot ���ض��� ��������һ�����ںš�>�������һ����
#define ART_MARCATO_DOT_UP(x,y)  GLYPH_Petrucci(x,y,GLYPH_FLAG_SIZE, 0, "c1")//@"f9")
#define ART_MARCATO_DOT_DOWN(x,y)  GLYPH_Petrucci_rotate(x+LINE_H,y,GLYPH_FLAG_SIZE, 90, "c1")

//Marcato //strong_accent_placement  ��������һ��"^"�����·�һ��"V"
#define ART_STRONG_ACCENT_UP(x,y)  GLYPH_Petrucci(x,y,GLYPH_FLAG_SIZE, 0, "c2")//@"5e")
#define ART_STRONG_ACCENT_DOWN(x,y)  GLYPH_Petrucci(x,y,GLYPH_FLAG_SIZE, 0, "c4")//@"76")
//SForzando_Dot ������һ��^��һ���㣬�����·�V��һ���㡣
#define ART_SFORZANDO_UP(x,y)  GLYPH_Petrucci(x,y,GLYPH_FLAG_SIZE, 0, "c3")//@"ac")
#define ART_SFORZANDO_DOWN(x,y)  GLYPH_Petrucci(x,y,GLYPH_FLAG_SIZE, 0, "c5")//@"e8")
//Staccatissimo or Spiccato ���� ��������һ��ʵ�ĵ�������
#define ART_STACCATISSIMO(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "c6")//@"ae")
//#define ART_STACCATISSIMO_DOWN(x,y)  GLYPH_Petrucci(x+LINE_H,y,40, 180, @"c6")//@"27")
#define ART_STACCATISSIMO_DOWN(x,y)  GLYPH_Petrucci_rotate(x+LINE_H,y,GLYPH_FONT_SIZE, 180, "c6")//@"27")

//Articulation_Fermata://�ӳ��Ǻ� ��������һ����Բ��������һ���㡣
#define ART_FERMATA_DOWN(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "b3")//@"75") //"fermata"="�ӳ��Ǻţ�ͣ���Ǻ�";
#define ART_FERMATA_UP(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "b2")//@"55")
//Mordent ҧ��
/*
1. Mordent or lower mordent: a shake sign crossed by a vertical line:һ����ݷ����м䴩��һ������
Articulation_Short_Mordent
����ķ�����C������������ž���Ҫ���ࣺ C-B-C, ǰ������16������������������8��������

2. Upper Mordent or inverted mordent: a same shake sign: һ��ͬ���ľ�ݷ���
Articulation_Inverted_Short_Mordent
����ķ�����C������������ž���Ҫ���ࣺ�� D-C-D-C, ǰ3����16����������ʣ�µ����һ�����㡣
Articulation_Inverted_Long_Mordent
����ķ�����C������������ž���Ҫ���ࣺ D-C-D-C-D-C..., ǰ��5������16����������ʣ�µ����һ�����㡣
*/
#define ART_MORDENT_UPPER(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "a3")//@"6d") //
#define ART_MORDENT_LOWER(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "a4")//@"4d") //
#define ART_MORDENT_LONG(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "a5")//@"b5") //
//turn ��������һ�����S�֣� ��ʾҪ�������ࣺ������һ����������
#define ART_TURN(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "a6")//@"54") //
#define ART_PEDAL_DOWN(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "d0")//@"a1")//161 //̤��
#define ART_PEDAL_UP(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "d1")//@"2a")//*

//bowUp/down
#define ART_BOWDOWN(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "cd")
#define ART_BOWUP(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "cc")
#define ART_BOWDOWN_BELOW(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "cf")
#define ART_BOWUP_BELOW(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE, 0, "ce")

//���� tr
#define ART_TRILL(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE,0,"a0")//@"d9")

//Octave 8va
#define OCTAVE_ATTAVA(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE,0,"b4")
//Octave 8vb
#define OCTAVE_ATTAVB(x,y)  GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE,0,"b5")
//Octave 15ma
#define OCTAVE_QUINDICESIMA(x,y)    GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE,0,"b6")

//���� Tremolo
#define TREMOLO_8(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE,0,"a8")
#define TREMOLO_16(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE,0,"a9")
#define TREMOLO_32(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE,0,"aa")
#define TREMOLO_64(x,y) GLYPH_Petrucci(x,y,GLYPH_FONT_SIZE,0,"ab")

/*
<line x1="0" y1="0" x2="300" y2="300" style="stroke:rgb(99,99,99);stroke-width:2"/>
<line x1="40" x2="120" y1="100" y2="100" stroke="black" stroke-width="20" stroke-linecap="round"/>
�ߵĶ˵�: stroke-linecap����������Զ������߶ζ˵�ķ��������Կ���ʹ��butt(ƽ),square(��),round(Բ)����ֵ.
*/

#define	BEGIN_MEASURE(m, x, y, w, h)	\
{	\
	char tmp[128];	\
	sprintf(tmp, "<rect id='m%d' x='%d' y='%d' width='%d' height='%d' style='fill: rgba(200, 200, 210, 0);' onclick='clickMm(%d)' />\n",m,x,y,w,h,m);	\
	svgXmlContent->appendString(tmp);	\
}

//#define	BEGIN_MEASURE(m, x, y, w, h)	\
//{	\
//	char tmp[128];	\
//	sprintf(tmp, "<rect id='m%d' x='%d' y='%d' width='%d' height='%d' style='fill: rgba(200, 200, 210, 0);' />\n",m,x,y,w,h);	\
//	svgXmlContent->appendString(tmp);	\
//}

#define	END_MEASURE()

#define LINE_C(x1,y1,x2,y2,stroke,w)  \
{   \
char tmp[128]; \
sprintf(tmp,"<line x1='%.1f' x2='%.1f' y1='%.1f' y2='%.1f' stroke='%s' stroke-width='%d' />\n",x1,x2,y1,y2,stroke,w); \
svgXmlContent->appendString(tmp); \
}

#define LINE_W(x1,y1,x2,y2,w)  \
{   \
char tmp[128]; \
sprintf(tmp,"<line x1='%.1f' x2='%.1f' y1='%.1f' y2='%.1f' "strokeColor" stroke-width='%.1f' />\n",(float)(x1),(float)(x2),(float)(y1),(float)(y2),(float)(w)); \
svgXmlContent->appendString(tmp); \
}

void VmusImage::LINE(float x1,float y1,float x2,float y2)
{
    char tmp[128];
    sprintf(tmp,"<line x1='%.1f' x2='%.1f' y1='%.1f' y2='%.1f' "strokeColor" />\n",(float)(x1),(float)(x2),(float)(y1),(float)(y2));
    svgXmlContent->appendString(tmp);
}

#define LINE_DOT(x1,y1,x2,y2)  \
{   \
char tmp[256]; \
sprintf(tmp,"<line x1='%d' x2='%d' y1='%d' y2='%d' stroke='black' stroke-width=\"1\" stroke-dasharray=\"5,5\" />\n",(int)(x1),(int)(x2),(int)(y1),(int)(y2)); \
svgXmlContent->appendString(tmp); \
}

#define TEXT(x,y,font_size, text)  \
{   \
char tmp[256]; \
sprintf(tmp,"<text x='%d' y='%d' font-size='%d' "fillColor">%s</text>\n", (int)(x), (int)(y+(font_size)),(int)(font_size), text); \
svgXmlContent->appendString(tmp); \
}

#define TEXT_ATTR(x,y,font_size, text, isBold, isItalic)  \
{   \
char tmp[256]; \
sprintf(tmp,"<text x='%d' y='%d' font-size='%d' "fillColor" font-weight='%s' font-style='%s'>%s</text>\n", (int)(x), (int)(y+(font_size)),(int)(font_size), (isBold)?"bold":"normal", (isItalic)?"italic":"normal", text); \
svgXmlContent->appendString(tmp); \
}

#define TEXT_ATTR_FONTFAMILY(x,y,font_size, text, isBold, isItalic, font_family)  \
{   \
	char tmp[256]; \
	sprintf(tmp,"<text x='%d' y='%d' font-size='%d' fill='black' font-weight='%s' font-style='%s' style=\"font-family: '%s';\">%s</text>\n", (int)(x), (int)(y+(font_size)),(int)(font_size), (isBold)?"bold":"normal", (isItalic)?"italic":"normal", font_family, text); \
	svgXmlContent->appendString(tmp); \
}

#define TEXT_CENTER(x,y,font_size, text)  \
{   \
	char tmp[256]; \
	sprintf(tmp,"<text x='%d' y='%d' font-size='%d' text-anchor=\"middle\" "fillColor">%s</text>\n", (int)(x), (int)(y+(font_size)),(int)(font_size), text); \
	svgXmlContent->appendString(tmp); \
}

#define TEXT_RIGHT(x,y,font_size, text)  \
{   \
	char tmp[256]; \
	sprintf(tmp,"<text x='%d' y='%d' font-size='%d' text-anchor=\"end\" "fillColor">%s</text>\n", (int)(x), (int)(y+(font_size)),(int)(font_size), text); \
	svgXmlContent->appendString(tmp); \
}

#define TEXT_RIGHT_ITALIC(x,y,font_size, text)  \
{	\
char tmp[256];	\
sprintf(tmp, "<text x='%d' y='%d' font-size='%d' text-anchor=\"end\" "fillColor"  font-style='italic'>%s</text>\n", (int)(x), (int)(y+(font_size)),(int)(font_size), text);	\
svgXmlContent->appendString(tmp);	\
}

#define TEXT_JIANPU(x,y,text, fixdoText, size)  \
{   \
char tmp[256]; \
sprintf(tmp,"<text x='%d' y='%d' font-size='%d' fill='blue'>%s</text>\n", (int)(x), (int)(y+(size)),(int)(size), text); \
svgXmlJianpuContent->appendString(tmp); \
sprintf(tmp,"<text x='%d' y='%d' font-size='%d' fill='blue'>%s</text>\n", (int)(x), (int)(y+(size)),(int)(size), fixdoText); \
svgXmlJianpuFixDoContent->appendString(tmp); \
}

//style="fill:#000000;font-family:Arial;font-weight:bold;font-size:40;"
//<circle cx="15" cy="15" r="15" fill="yellow" transform="skewX(45)" />
#define BACK_JIANWP(x,y,r,color) \
{   \
char tmp[256]; \
sprintf(tmp,"<circle cx='0' cy='0' r='%.1f' fill='%s' transform='translate(%d,%d)' />\n", r, color, (int)(x+r*1.5), (int)(y-r*0.0)); \
svgXmlJianwpContent->appendString(tmp); \
sprintf(tmp,"<circle cx='0' cy='0' r='%.1f' fill='%s' transform='translate(%d,%d)' />\n", r,  color, (int)(x+r*1.5), (int)(y-r*0.00)); \
svgXmlJianwpFixDoContent->appendString(tmp); \
}

#define TEXT_JIANWP(x,y,text,fixdoText,size, color) \
{   \
char tmp[256]; \
sprintf(tmp,"<text x='%d' y='%d' style='fill:%s;font-family:Arial;font-weight:bold;font-size:%d;'>%s</text>\n", (int)(x+LINE_H*0.4), (int)(y+LINE_H*0.45), color, (int)(size), text); \
svgXmlJianwpContent->appendString(tmp); \
sprintf(tmp,"<text x='%d' y='%d' style='fill:%s;font-family:Arial;font-weight:bold;font-size:%d;'>%s</text>\n", (int)(x+LINE_H*0.4), (int)(y+LINE_H*0.45), color, (int)(size), fixdoText); \
svgXmlJianwpFixDoContent->appendString(tmp); \
}

#define SHARP_JIANPU(x,y)   \
{   \
char tmp[256]; \
sprintf(tmp,"<text x='%.1f' y='%.1f' font-size=\"20\" fill=\"blue\" style=\"font-family: 'Aloisen New'\">%s</text>\n", (float)(x), (float)(y), ELEM_FLAG_SHARP); \
svgXmlJianpuContent->appendString(tmp); \
sprintf(tmp,"<text x='%.1f' y='%.1f' font-size=\"20\" fill=\"blue\" style=\"font-family: 'Aloisen New'\">%s</text>\n", (float)(x), (float)(y), ELEM_FLAG_SHARP); \
svgXmlJianpuFixDoContent->appendString(tmp);	\
}

#define FLAT_JIANPU(x,y)   \
{   \
char tmp[256]; \
sprintf(tmp,"<text x='%.1f' y='%.1f' font-size='24' fill='blue' style=\"font-family: 'Aloisen New'\">%s</text>\n", (float)(x), (float)(y), ELEM_FLAG_FLAT); \
svgXmlJianpuContent->appendString(tmp); \
sprintf(tmp,"<text x='%.1f' y='%.1f' font-size='24' fill='blue' style=\"font-family: 'Aloisen New'\">%s</text>\n", (float)(x), (float)(y), ELEM_FLAG_FLAT); \
svgXmlJianpuFixDoContent->appendString(tmp); \
}

#define DOT_JIANPU(x,y) \
{   \
char tmp[256]; \
sprintf(tmp,"<text x='%.1f' y='%.1f' font-size=\"30\" fill=\"blue\" style=\"font-family: 'Aloisen New'\">%s</text>\n", (float)(x), (float)(y), "c7"); \
svgXmlJianpuContent->appendString(tmp); \
sprintf(tmp,"<text x='%.1f' y='%.1f' font-size=\"30\" fill=\"blue\" style=\"font-family: 'Aloisen New'\">%s</text>\n", (float)(x), (float)(y), "c7"); \
svgXmlJianpuFixDoContent->appendString(tmp); \
}

#define RECT_JIANPU(x,y,w,h)   \
{   \
char tmp[256]; \
sprintf(tmp,"<rect x='%d' y='%d' width='%d' height='%d' fill='rgba(252,252,230,0.75)' />", (int)(x), (int)(y), (int)(w), (int)(h)); \
svgXmlJianpuContent->appendString(tmp); \
sprintf(tmp,"<rect x='%d' y='%d' width='%d' height='%d' fill='rgba(252,252,230,0.75)' />", (int)(x), (int)(y), (int)(w), (int)(h)); \
svgXmlJianpuFixDoContent->appendString(tmp); \
}

#define LINE_JIANPU(x1,x2,y,w)    \
{   \
char tmp[256]; \
sprintf(tmp,"<line x1='%.1f' x2='%.1f' y1='%.1f' y2='%.1f' stroke=\"blue\" stroke-width='%d' />\n",(float)(x1),(float)(x2),(float)(y),(float)(y), (w)); \
svgXmlJianpuContent->appendString(tmp); \
sprintf(tmp,"<line x1='%.1f' x2='%.1f' y1='%.1f' y2='%.1f' stroke=\"blue\" stroke-width='%d' />\n",(float)(x1),(float)(x2),(float)(y),(float)(y), (w)); \
svgXmlJianpuFixDoContent->appendString(tmp); \
}

std::shared_ptr<OveNote> VmusImage::getNoteWithOffset(int meas_offset, int meas_pos , const std::shared_ptr<OveMeasure>& measure, int staff, int voice)
{
    std::shared_ptr<OveNote> note2 = nullptr;
	auto& next_measure = music->measures[measure->number+meas_pos];
	
	for (auto it = next_measure->notes.begin(); it != next_measure->notes.end(); it++) {
		if ((*it)->pos.start_offset == meas_offset && (*it)->staff == staff && (*it)->voice == voice) {
			note2 = *it;
			break;
		}
	}
    return note2; 
}

float VmusImage::lineToY(int line, int staff)
{
    float y = (4-line)*(LINE_H*0.5f);  //getNoteY:note];
    if (staff>1)
        y+=STAFF_OFFSET[staff-1];
    return y;
}

NoteHeadType VmusImage::headType(std::shared_ptr<NoteElem>& elem, int staff)
{
    NoteHeadType ret=NoteHead_Standard;
    if (staff<music->trackes.size()) {
		auto& track = music->trackes[staff];
        if (track->start_clef>=Clef_Percussion1) {
            track_node *node_info=&(track->node);
            for (int i=0; i<16; i++) {
                if (node_info->line[i]==elem->line) {
                    ret=(NoteHeadType)node_info->head_type[i];
                    break;
                }
            }
        }
    }
    return ret;
}

/*
 index staff voice
 0      1       0
 1      1       1
 2      2       0
 3      2       1
*/

static inline MyRect MyRectMake(float x, float y, float width, float height)
{
    MyRect rect;
    rect.origin.x = x; rect.origin.y = y;
    rect.size.width = width; rect.size.height = height;
    return rect;
}

MyRect beam_continue_pos[4];
MyRect beam_current_pos;
MyRect VmusImage::getBeamRect(const std::shared_ptr<OveBeam>& beam, float start_x, float start_y, const std::shared_ptr<OveMeasure>& measure, bool reload)
{
    MyRect drawPos=MyRectMake(beam->drawPos_x, beam->drawPos_y, beam->drawPos_width, beam->drawPos_height);
    if (drawPos.size.width==0 || reload) {
        BeamElem *elem0=beam->beam_elems.front().get();
		if (elem0)
        {
            float y1, y2;
            float x1, x2;
            y1 = start_y+ lineToY(beam->left_line,beam->staff);
            y2 = start_y+ lineToY(beam->right_line,beam->stop_staff);
            x1 = start_x+MEAS_LEFT_MARGIN+elem0->start_measure_offset*OFFSET_X_UNIT;
            x2 = start_x+MEAS_LEFT_MARGIN+elem0->stop_measure_offset*OFFSET_X_UNIT;
			if (elem0->stop_measure_pos>0)		//���������С��
                x2+=measure->meas_length_size*OFFSET_X_UNIT+MEAS_RIGHT_MARGIN+MEAS_LEFT_MARGIN;
            
			//����beam��ʼλ�õ�note
            auto note1=getNoteWithOffset(elem0->start_measure_offset,0, measure,beam->staff,beam->voice);
            float zoom=1;
            
			//���beam��ʼ��note��Ҫ�任staff,���޸�y1����
            if (note1!=nullptr)
            {
                if (note1->isGrace) {		//����
                    zoom=YIYIN_ZOOM;
                }
                if (note1->stem_up) {
                    x1+=LINE_H*zoom;
                }
                
                NoteElem *firstElem=note1->note_elems.front().get();
                if (firstElem->offsetStaff!=0) {
                    //NSLog("The note1 beam need offset:%d in measure=%d", note1->note_elem[0].offsetStaff, measure->number);
                    y1 = start_y+ lineToY(beam->left_line,beam->staff+firstElem->offsetStaff);
                }
            }
            
			//����beam����λ�õ�note
            auto note2=getNoteWithOffset(elem0->stop_measure_offset,elem0->stop_measure_pos, measure,beam->staff,beam->voice);
            if (note2 && note2->stem_up) {
                x2+=LINE_H*zoom;
            }
            
			//���beam�յ�note��Ҫ�任staff,���޸�y2����
            if (note2!=nullptr) {
                NoteElem *firstElem=note2->note_elems.front().get();
                if (firstElem->offsetStaff!=0) {
                    //NSLog("The note2 beam need offset:%d in measure=%d", note2.note_elem[0].offsetStaff, measure->number);
                    y2 = start_y+ lineToY(beam->right_line,beam->stop_staff+firstElem->offsetStaff);
                }
            }

            beam->drawPos_x=drawPos.origin.x=x1;
            beam->drawPos_y=drawPos.origin.y=y1;
            beam->drawPos_width=drawPos.size.width=x2-x1;
            beam->drawPos_height=drawPos.size.height=y2-y1;
        }
    }
    return drawPos;
}

void VmusImage::drawSvgStem(MyRect beam_pos, const std::shared_ptr<OveNote>& note, float x, float y)
{
	//��stem
    float tmp_x=x;
    float zoom=1;
    if (note->isGrace)
        zoom=YIYIN_ZOOM;
    
    if (note->stem_up) {
        tmp_x=x+LINE_H+1;
		if (LINE_H < 10)
			tmp_x -= 0.5;
        if (note->isGrace)
            tmp_x=x+LINE_H*YIYIN_ZOOM+0.5f;
    }else{
        tmp_x=x+0.0f;
    }
    if (note->isGrace)
        tmp_x+=GRACE_X_OFFSET;
    
    {
        float tmp_y2=beam_pos.size.height/beam_pos.size.width*(tmp_x-beam_pos.origin.x)*zoom+beam_pos.origin.y;
        if (tmp_y2>y) {
            //tmp_y2=beam_current_pos[1].size.height/beam_current_pos[1].size.width*(x-beam_current_pos[1].origin.x)+beam_current_pos[1].origin.y;
        }
        
		//���beam�������
        if (tmp_y2<0 || tmp_y2>screen_height) {
			//...
        }else{
            LINE(tmp_x, y+LINE_H*0.0f, tmp_x, tmp_y2);
        }
    }
}

void VmusImage::drawSvgTitle()
{
    if (music->work_title != "")
        TEXT_CENTER(screen_width*0.5, LINE_H*1.5, TITLE_FONT_SIZE, music->work_title.c_str());

	//work_number
	if (music->work_number != "")
	{
		int font_size = TITLE_FONT_SIZE*0.6;
		TEXT_CENTER(screen_width*0.5, LINE_H*2.5+TITLE_FONT_SIZE, font_size, music->work_number.c_str());
	}

	//composer
// 	if (music->composer != "")
// 	{
// 		int font_size = TITLE_FONT_SIZE*0.6;
// 		TEXT_RIGHT(screen_width-20, TITLE_FONT_SIZE, font_size, music->composer.c_str());
// 	}
}

bool VmusImage::drawSvgAccidental(AccidentalType accidental_type, float acc_x, float y, bool isGrace)
{
	//accidental ��������
    if (accidental_type>Accidental_Normal) {
        float zoom=1;
        if (isGrace) {
            zoom=YIYIN_ZOOM+0.1f;
			acc_x += 1;
        }
        if (accidental_type == Accidental_Sharp) {
            FLAG_SHARP(acc_x-0.7*LINE_H*zoom, y, zoom);
        }else if (accidental_type == Accidental_DoubleSharp) {
            FLAG_DOUBLE_SHARP(acc_x-0.9*LINE_H*zoom, y, zoom);
        }else if (accidental_type == Accidental_DoubleSharp_Caution) {
            FLAG_DOUBLE_SHARP_CAUTION(acc_x-1.5*LINE_H*zoom, y);
        }else if(accidental_type == Accidental_Sharp_Caution){
            FLAG_SHARP_CAUTION(acc_x-1.5*LINE_H*zoom, y);
        }else if (accidental_type == Accidental_Natural) {
            FLAG_STOP(acc_x-0.7*LINE_H*zoom, y, zoom);
        }else if(accidental_type == Accidental_Natural_Caution){
            FLAG_STOP_CAUTION(acc_x-2*LINE_H*zoom, y, zoom);
        }else if(accidental_type == Accidental_Flat){
            FLAG_FLAT(acc_x-0.6*LINE_H*zoom, y, zoom);
        }else if(accidental_type == Accidental_DoubleFlat){
            FLAG_DOUBLE_FLAT(acc_x-1.1*LINE_H*zoom, y);
        }else if(accidental_type == Accidental_Flat_Caution){
            FLAG_FLAT_CAUTION(acc_x-2.5*LINE_H*zoom, y);
        }else{
            return false;
        }
    }
    return true;
}

void VmusImage::drawSvgDiaohaoWithClef(ClefType clef,int key_fifths,int x, float start_y,bool stop_flag)
{
#ifdef OVE_IPHONE
#define DRAW_FLAG(off_x, line)    \
{   \
GLYPH_Petrucci(off_x+7*LINE_H,tmp_y+LINE_H*line,GLYPH_FONT_SIZE,0,flag); \
}
#else
#define DRAW_FLAG(off_x, line)    \
{   \
GLYPH_Petrucci(off_x+5*LINE_H,tmp_y+LINE_H*line,GLYPH_FONT_SIZE,0,flag); \
}
#endif

    float tmp_y;
    const char *flag = nullptr;
    if (stop_flag) {
        flag=ELEM_FLAG_STOP;
        tmp_y=start_y+LINE_H*(-1);
        x-=1*LINE_H;
    }else if(key_fifths>0){
        flag=ELEM_FLAG_SHARP;
        tmp_y=start_y+LINE_H*(-1.0f);
    }else{
        flag=ELEM_FLAG_FLAT;
        tmp_y=start_y+LINE_H*(-1);
    }

    if (showJianpu && clef==Clef_Treble && !stop_flag) {
        //jianpu
        //ELEM_FLAG_SHARP="&#xf023"
        const char *jianpuhao[]={"1= C","1= G","1= D","1= A","1= E","1= B","1=F",
                             "1=C",
							 "1=G","1=D","1=A","1=E","1=B","1= F","1= C"};
        if (last_fifths<-1) {
            FLAT_JIANPU(x+5*LINE_H, start_y-2.5*LINE_H);
        }else if (last_fifths>5){
            SHARP_JIANPU(x+5*LINE_H, start_y-2.5*LINE_H);
        }
        TEXT_JIANPU(x+2*LINE_H, start_y-3*LINE_H, jianpuhao[last_fifths+7], "1=C", JIANPU_FONT_SIZE);
    }
    
    if (clef==Clef_Treble) {
        switch (key_fifths) {
            case 0://C major or a minor: CEG:135, ace:
                break;
            case 1://G major, e minor
                DRAW_FLAG(x-5,1);
                break;
            case 2://D major, b minor
                DRAW_FLAG(x-8,1);
                DRAW_FLAG(x+5,2.5);
                break;
            case 3://A major, f# minor
                DRAW_FLAG(x-12,1);
                DRAW_FLAG(x+0, 2.5);
                DRAW_FLAG(x+12,0.5);
                break;
            case 4://E major, c# minor
                DRAW_FLAG(x-15, 1);
                DRAW_FLAG(x-5, 2.5);
                DRAW_FLAG(x+5,0.5);
                DRAW_FLAG(x+15,2);
                break;
            case 5://B major, g# minor
                DRAW_FLAG(x-20,1.0);
                DRAW_FLAG(x-10, 2.5);
                DRAW_FLAG(x+0, 0.5);
                DRAW_FLAG(x+10, 2.0);
                DRAW_FLAG(x+20,3.5);
                break;
            case 6://F# major, d# minor
                DRAW_FLAG(x-22,1.0);
                DRAW_FLAG(x-12, 2.5);
                DRAW_FLAG(x-2, 0.5);
                DRAW_FLAG(x+8, 2.0);
                DRAW_FLAG(x+18,3.5);
                DRAW_FLAG(x+28,1.5);
                break;
            case 7://C# major, a# minor
                DRAW_FLAG(x-22,1.0);
                DRAW_FLAG(x-12, 2.5);
                DRAW_FLAG(x-2, 0.5);
                DRAW_FLAG(x+8, 2.0);
                DRAW_FLAG(x+18,3.5);
                DRAW_FLAG(x+28,1.5);
                DRAW_FLAG(x+38,3.0);
                break;
            case -1://F major, d minor
                DRAW_FLAG(x-5, 3.0);
                break;
            case -2://Bb major, g minor
                DRAW_FLAG(x-5, 3.0);
                DRAW_FLAG(x+5, 1.5);
                break;
            case -3://Eb major, c minor
                DRAW_FLAG(x-10, 3.0);
                DRAW_FLAG(x+0, 1.5);
                DRAW_FLAG(x+10, 3.5);
                break;
            case -4://Ab major, f minor
                DRAW_FLAG(x-13, 3.0);
                DRAW_FLAG(x-4, 1.5);
                DRAW_FLAG(x+5,3.5);
                DRAW_FLAG(x+14,2.0);
                break;
            case -5://Db major, bb minor
                DRAW_FLAG(x-16,3.0);
                DRAW_FLAG(x-8, 1.5);
                DRAW_FLAG(x+0, 3.5);
                DRAW_FLAG(x+8, 2.0);
                DRAW_FLAG(x+16,4.0);
                break;
            case -6://Gb major, eb minor
                DRAW_FLAG(x-18,3.0);
                DRAW_FLAG(x-10, 1.5);
                DRAW_FLAG(x-2, 3.5);
                DRAW_FLAG(x+6, 2.0);
                DRAW_FLAG(x+14,4.0);
                DRAW_FLAG(x+22,2.5);
                break;
            case -7://Cb major, ab minor
                DRAW_FLAG(x-20,3.0);
                DRAW_FLAG(x-12,1.5);
                DRAW_FLAG(x-4, 3.5);
                DRAW_FLAG(x+4, 2.0);
                DRAW_FLAG(x+12,4.0);
                DRAW_FLAG(x+20,2.5);
                DRAW_FLAG(x+28,4.5);
                break;
            default:
                //NSLog("Error: unknow diaohao:%d",key_fifths);
                break;
        }
    }else{
        switch (key_fifths) {
            case 1://G major
                DRAW_FLAG(x-5,2);
                break;
            case 2://D major
                DRAW_FLAG(x-8,2);
                DRAW_FLAG(x+5,3.5);
                break;
            case 3://A major, f# minor
                DRAW_FLAG(x-12,2);
                DRAW_FLAG(x+0,3.5);
                DRAW_FLAG(x+12,1.5);
                break;
            case 4://E major
                DRAW_FLAG(x-15,2.0);
                DRAW_FLAG(x-5,3.5);
                DRAW_FLAG(x+5,1.5);
                DRAW_FLAG(x+15,3.0);
                break;
            case 5://B major
                DRAW_FLAG(x-20,2.0);
                DRAW_FLAG(x-10, 3.5);
                DRAW_FLAG(x+0, 1.5);
                DRAW_FLAG(x+10, 3.0);
                DRAW_FLAG(x+20,4.5);
                break;
            case 6://F# major
                DRAW_FLAG(x-20, 2.0);
                DRAW_FLAG(x-10, 3.5);
                DRAW_FLAG(x-0, 1.5);
                DRAW_FLAG(x+10, 3.0);
                DRAW_FLAG(x+20, 4.5);
                DRAW_FLAG(x+30, 2.5);
                break;
            case 7://C# major
                DRAW_FLAG(x-20, 2.0);
                DRAW_FLAG(x-10, 3.5);
                DRAW_FLAG(x-0, 1.5);
                DRAW_FLAG(x+10, 3.0);
                DRAW_FLAG(x+20, 4.5);
                DRAW_FLAG(x+30, 2.5);
                DRAW_FLAG(x+40, 4.0);
                break;
            case -1://F major, D minor
                DRAW_FLAG(x-5, 4.0);
                break;
            case -2://Bb major
                DRAW_FLAG(x-5, 4);
                DRAW_FLAG(x+5, 2.5);
                break;
            case -3://Eb major
                DRAW_FLAG(x-10, 4);
                DRAW_FLAG(x+0, 2.5);
                DRAW_FLAG(x+10, 4.5);
                break;
            case -4://Ab major
                DRAW_FLAG(x-13,4.0);
                DRAW_FLAG(x-4, 2.5);
                DRAW_FLAG(x+5, 4.5);
                DRAW_FLAG(x+14,3);
                break;
            case -5://Db major, bb minor
                DRAW_FLAG(x-16, 4.0);
                DRAW_FLAG(x-8, 2.5);
                DRAW_FLAG(x+0, 4.5);
                DRAW_FLAG(x+8, 3);
                DRAW_FLAG(x+16, 5.0);
                break;
            case -6://Gb major, eb minor
                DRAW_FLAG(x-18, 4.0);
                DRAW_FLAG(x-10, 2.5);
                DRAW_FLAG(x-2, 4.5);
                DRAW_FLAG(x+6, 3);
                DRAW_FLAG(x+14, 5.0);
                DRAW_FLAG(x+22, 3.5);
                break;
            case -7://ab minor
                DRAW_FLAG(x-20, 4.0);
                DRAW_FLAG(x-12, 2.5);
                DRAW_FLAG(x-4, 4.5);
                DRAW_FLAG(x+4, 3);
                DRAW_FLAG(x+12, 5.0);
                DRAW_FLAG(x+20, 3.5);
                DRAW_FLAG(x+28, 5.5);
                break;
            case 0: //C major or A minor
                break;
            default:
                //NSLog("Error: unknow diaohao:%d",key_fifths);
                break;
        }
    }
}

//time signature �ĺţ�����֮����
void VmusImage::drawSvgTimeSignature(OveMeasure* measure, float x,float start_y, unsigned long staff_count)
{
    if (measure->denominator>0 && measure->numerator>0)
    {
        for (int nn=0; nn<staff_count; nn++) {
            if (measure->denominator==2 && measure->numerator==2) {
                GLYPH_Petrucci(x, start_y+STAFF_OFFSET[nn]+2*LINE_H, GLYPH_FONT_SIZE, 0, ELEM_TIME_SIGNATURE_CUT_TIME);
            }else if (measure->denominator==4 && measure->numerator==4) {
                GLYPH_Petrucci(x, start_y+STAFF_OFFSET[nn]+2*LINE_H, GLYPH_FONT_SIZE, 0, ELEM_TIME_SIGNATURE_COMMON_TIME);
            }else{
                float tmp_x=x;
                if (measure->numerator>=10)
                    tmp_x=x-8;

                char tmp_numerator[64];
                if (measure->numerator>10) {
					sprintf(tmp_numerator,"3%d;&#x3%d",measure->numerator/10,measure->numerator%10);
                }else{
                    sprintf(tmp_numerator,"3%d",measure->numerator%10);
                }
                GLYPH_Petrucci(tmp_x, start_y+STAFF_OFFSET[nn]+1*LINE_H,GLYPH_FONT_SIZE, 0, tmp_numerator);
				//denominator ��ĸ
                tmp_x=x;
                if (measure->denominator>=10)
                    tmp_x=x-8;
                
                if (measure->denominator>10) {
					sprintf(tmp_numerator,"3%d;&#x3%d",measure->denominator/10,measure->denominator%10);
                }else{
                    sprintf(tmp_numerator,"3%d",measure->denominator);
                }
                GLYPH_Petrucci(tmp_x, start_y+STAFF_OFFSET[nn]+1*LINE_H+2*LINE_H,GLYPH_FONT_SIZE, 0, tmp_numerator);
            }
        }
    }
}

#define CURVE(x1,x2,y1,y2,cp1x, cp1y,cp2x, cp2y)    \
{   \
char tmp[256]; \
sprintf(tmp,"<path d=\"M%d,%d C%d,%d %d,%d %d,%d C%d,%d %d,%d %d,%d\" "strokeColor" stroke-width='0.5' "fillColor"/>",(int)(x1),(int)(y1),(int)(cp1x),(int)(cp1y),(int)(cp2x),(int)(cp2y),(int)(x2),(int)(y2),(int)cp2x,(int)cp2y+3,(int)cp1x,(int)cp1y+3,(int)x1,(int)y1); \
svgXmlContent->appendString(tmp); \
}

void VmusImage::drawSvgCurveLine(int w, float x1, float y1, float x2, float y2, bool above)
{
#if 1
#define COS20 0.94f
#define SIN20 0.342f
    
#define COS30 0.866f
#define SIN30 0.5f
    
#define COS45 0.7071f
#define SIN45 0.7071f
    
#define COS60 0.5f
#define SIN60 0.866f
    
    float COSA=COS30;
    float SINA=(-SIN30);
    float SIN_A=SIN30;
    float LEFT_RATE=0.3f;
    float RIGHT_RATE=0.7f;
    
    float cp1x,cp2x;
    float cp1y,cp2y;
#ifdef ADAPT_CUSTOMIZED_SCREEN
	LEFT_RATE=0.2f;
	COSA=COS20;
	SINA=(-SIN20);
	SIN_A=SIN20;
#else
    if (x2-x1<100) {
        LEFT_RATE=0.30f;
        COSA=COS60;
        SINA=(-SIN60);
        SIN_A=SIN60;
        if (x2-x1<1.5*LINE_H) {
            x1-=LINE_H*0.5;
            x2=x1+1.5*LINE_H;
        }
    }else if (x2-x1<200) {
        COSA=COS45;
        SINA=(-SIN45);
        SIN_A=SIN45;
    }else if(x2-x1>400)
    {
        LEFT_RATE=0.2f;
        COSA=COS20;
        SINA=(-SIN20);
        SIN_A=SIN20;
    }
#endif
    RIGHT_RATE=1-LEFT_RATE;
    
    float tmp1x=x1+(x2-x1)*LEFT_RATE;
    float tmp1y=y1+(y2-y1)*LEFT_RATE;
    float tmp2x=x1+(x2-x1)*RIGHT_RATE;
    float tmp2y=y1+(y2-y1)*RIGHT_RATE;
    
    if (above) {
        cp1x=(tmp1x-x1)*COSA-(tmp1y-y1)*SINA+x1;
        cp1y=(tmp1x-x1)*SINA+(tmp1y-y1)*COSA+y1;
        
        cp2x=(tmp2x-x2)*COSA-(tmp2y-y2)*SIN_A+x2;
        cp2y=(tmp2x-x2)*SIN_A+(tmp2y-y2)*COSA+y2;
    }else{
        cp1x=(tmp1x-x1)*COSA-(tmp1y-y1)*SIN_A+x1;
        cp1y=(tmp1x-x1)*SIN_A+(tmp1y-y1)*COSA+y1;
        
        cp2x=(tmp2x-x2)*COSA-(tmp2y-y2)*SINA+x2;
        cp2y=(tmp2x-x2)*SINA+(tmp2y-y2)*COSA+y2;
    }
#endif
    CURVE(x1, x2, y1, y2, cp1x, cp1y, cp2x, cp2y);
}

bool VmusImage::drawSvgArt(const std::shared_ptr<NoteArticulation>& art, bool art_placement_above, int x, int y, float start_y)
{
	ArticulationType art_type = art->art_type;
    if (art_type == Articulation_Staccato) {		//staccato ����/����/���� ��������һ����
        if (art_placement_above) {
			ART_STACCATO(x+LINE_H*0.4, y+LINE_H*0.5);
        } else {
			ART_STACCATO(x+LINE_H*0.4, y-LINE_H*0.6);
        }
    } else if (art_type == Articulation_Tenuto) {		//tenuto ������ ��������һ������
        if (art_placement_above) {
            LINE_W(x-LINE_H*0.3, y+LINE_H*0.5, x+LINE_H*1.2, y+LINE_H*0.5, 2);
        } else {
			LINE_W(x-LINE_H*0.2, y-LINE_H*0.5, x+LINE_H*1.3, y-LINE_H*0.5, 2);
        }
    } else if (art_type == Articulation_Detached_Legato) {		//һ�����ߣ������һ����
		if (art_placement_above) {
			LINE_W(x, y+LINE_H*0.5, x+12, y+LINE_H*0.5, 2);
			ART_STACCATO(x+LINE_H*0.4, y);
		} else {
			LINE(x, y-LINE_H*0.5, x+12, y-LINE_H*0.5);
			ART_STACCATO(x+LINE_H*0.4, y);
		}
	} else if (art_type == Articulation_Natural_Harmonic) {		//һ��ԲȦ
        if (art_placement_above) {
            GLYPH_Petrucci(x+LINE_H*0.4,y-LINE_H*1.0,GLYPH_FONT_SIZE, 0, "c9");
        } else {
            GLYPH_Petrucci(x+LINE_H*0.4,y+LINE_H*1.0,GLYPH_FONT_SIZE, 0, "c9");
        }
    } else if (art_type==Articulation_Marcato) {		//Marcato ����/���� ��������һ�����ںš�>��
        if (art_placement_above) {
            ART_MARCATO(x+LINE_H*0, y+LINE_H*0.5);
        } else {
            ART_MARCATO(x+LINE_H*0, y+LINE_H*1);
        }
    } else if (art_type==Articulation_Marcato_Dot) {		//Marcato_Dot ���ض��� ��������һ�����ںš�>�������һ����
        if (art_placement_above) {
            ART_MARCATO_DOT_UP(x+LINE_H*0, y-LINE_H*0);
        } else {
			//���� >.
			/*
			����4����С�� 1
			<beat-unit>quarter</beat-unit>
			<beat-unit-dot/>
			<beat-unit>quarter</beat-unit>
			
			�������� ���ɲ��ԡ� ����6�� ����1
			*/
			ART_STACCATO(x+LINE_H*0.4, y);
            ART_MARCATO(x+LINE_H*0, y+LINE_H*1);
        }
    } else if (art_type==Articulation_Heavy_Attack) {		//Heavy_Attack ǿ�� ��������һ�����ںš�>�������һ������
        if (art_placement_above) {
            ART_MARCATO(x+LINE_H*0, y-LINE_H*1.5);
            LINE(x, y-LINE_H*1.0f, x+12.0f, y-LINE_H*1.0f);
        } else {
            ART_MARCATO(x+LINE_H*0, y-LINE_H*1);
            LINE(x, y-LINE_H*0.5f, x+12.0f, y-LINE_H*0.5f);
        }
    } else if (art_type==Articulation_SForzando || art_type==Articulation_SForzando_Inverted) {
		//strong_accent_placement ͻǿ ��������һ��"^" or ͻǿ(����) ��������һ��"V"
        if (art_placement_above) {
            ART_STRONG_ACCENT_UP(x+0.2*LINE_H, y-0.5*LINE_H);
        } else {
            ART_STRONG_ACCENT_DOWN(x+0.2*LINE_H, y+LINE_H*2.0);
        }
    } else if (art_type==Articulation_SForzando_Dot ||art_type==Articulation_SForzando_Dot_Inverted) {
		//SForzando_Dot ͻǿ���� ��������һ��"^"�����һ���� or ͻǿ����(����) ��������һ��"V"�����һ����
        if (art_placement_above) {
            ART_SFORZANDO_UP(x+0.0*LINE_H, y+0.5*LINE_H);
        } else {
            ART_SFORZANDO_DOWN(x+0.2*LINE_H, y+LINE_H*0.5);
        }
    } else if (art_type==Articulation_Heavier_Attack) {		//Heavier_Attack:��ǿ�� ������һ��^�����һ������
        if (art_placement_above) {
            ART_STRONG_ACCENT_UP(x+0.2*LINE_H, y-1.2*LINE_H);
			LINE(x, y-LINE_H*1.0f, x+12.0f, y-LINE_H*1.0f);
		} else {
			ART_STRONG_ACCENT_DOWN(x+0.2*LINE_H, y+LINE_H*2.0);
			LINE(x, y-LINE_H*1.0f, x+12.0f, y-LINE_H*1.0f);
        }
    } else if (art_type == Articulation_Staccatissimo) {		//staccatissimo_placement ������/���� ��������һ��ʵ�ĵ�������
        if (art_placement_above) {
            ART_STACCATISSIMO(x, y+LINE_H*0.5);
        } else {
            ART_STACCATISSIMO_DOWN(x, y-LINE_H*0.5);
        }
    } else if (art_type == Articulation_Fermata || art_type==Articulation_Fermata_Inverted) {
		//Articulation_Fermata://�ӳ��Ǻ� ��������һ����Բ��������һ���㡣
        if (art_placement_above) {
            ART_FERMATA_UP(x-5, y-LINE_H*0.0);
        } else {
            ART_FERMATA_DOWN(x-5, y+LINE_H*1.0);
        }
    } else if (art_type == Articulation_Inverted_Short_Mordent || art_type==Articulation_Inverted_Long_Mordent || art_type == Articulation_Short_Mordent) {
		//Mordent ����/����
		/*
		1. ������˳����/�ϲ�����Mordent/Upper Mordent/inverted mordent��Articulation_Inverted_Short_Mordent һ���̵ľ�ݷ���
		����ķ�����C������������ž���Ҫ���ࣺ 
		��1��C-D-C, ǰ������32������������������8���������Ӹ���
		��2��C-D-C, ǰ������16������������������8��������
		
		2.��˳���� Articulation_Inverted_Long_Mordent һ���̵ľ�ݷ��ţ� ͬ˳����
		����ķ�����C������������ž���Ҫ���ࣺ
		��1��C-D-C-D-C, ǰ4����32������������5����8��������
		
		3. �沨����Inverted Mordent/lower mordent�����²��� Articulation_Inverted_Short_Mordent
		: a shake sign crossed by a vertical line:һ����ݷ����м䴩��һ������
		Articulation_Short_Mordent
		����ķ�����C������������ž���Ҫ���ࣺ 
		��1��C-B-C, ǰ������32������������������8���������Ӹ���
		��2��C-B-C, ǰ������16������������������8��������
		
		4. ���沨���� һ������ݷ����м䴩��һ������
		Articulation_Long_Mordent
		����ķ�����C������������ž���Ҫ���ࣺ C-B-C-B-C  ǰ4����32������������5����8��������
		
		��������Ϸ����·����˸�������b��#
		��ô���߻��߽��͵�������B��D��Ҫ����b��# �������Bb,D#�ȣ�
		*/
        int tmp_x=x-LINE_H;
        int tmp_y = y;

        if (art_type == Articulation_Inverted_Short_Mordent) {
            ART_MORDENT_UPPER(tmp_x+1, tmp_y);
        }else if (art_type==Articulation_Short_Mordent){
            ART_MORDENT_LOWER(tmp_x+1, tmp_y);
        }else {
            ART_MORDENT_LONG(tmp_x-1, tmp_y);
        }
		if (Accidental_Natural == art->accidental_mark) {
			FLAG_STOP(tmp_x+LINE_H, tmp_y+1.5*LINE_H, 1);
		} else if (Accidental_Sharp ==art->accidental_mark) {
			FLAG_SHARP(tmp_x+LINE_H, tmp_y+1.5*LINE_H, 1);
		} else if (Accidental_Flat == art->accidental_mark) {
			FLAG_FLAT(tmp_x+LINE_H, tmp_y+1.5*LINE_H, 1);
		}
    } else if (art_type == Articulation_Turn) {
		/*
		turn ����
		1. ˳��������������һ�����S�֣� �����ע��һ���ķ�����C�ϣ���ʾҪ�������ࣺ
		//��1��������һ����������
		��2��D-C-B-C: 4��16������
		��3��D-C-B-C: ǰ3����һ����16�������ģ�����������4������(һ����8������������Ҫ�Ӹ���)
		��4��C-D-C-B-C: һ��16��������5����
		��5��C-D-C-B-C: ǰ4����32����������4����8������; ���������������֮�䣬���1����8����������4����32������
		2. ���������������һ�����S�֣��м䴩��һ�����ߣ� �����ע��һ���ķ�����C�ϣ���ʾҪ�������ࣺ
		��2��B-C-D-C: 4��16������
		��3��B-C-D-C: ǰ3����һ����16�������ģ�����������4����8������
		��4��C-B-C-D-C: һ��16��������5����
		��5��C-B-C-D-C: ǰ4����32����������4������(һ����8������������Ҫ�Ӹ���); ���������������֮�䣬���1����8����������4����32������
		
		��������Ϸ����·����˸�������b��#
		��ô��͵�������B��Ҫ����b��# �������Bb,B#�ȣ�
		*/
        //if (y>start_y)
        //    y=start_y;

        if (art_placement_above) {
            ART_TURN(x-5, y-LINE_H*0);
        } else {
            ART_TURN(x-5, y+LINE_H*0);
        }

		if (Accidental_Natural == art->accidental_mark) {
			FLAG_STOP(x, y+1.5*LINE_H, 1);
		} else if (Accidental_Sharp == art->accidental_mark) {
			FLAG_SHARP(x, y+1.5*LINE_H, 1);
		} else if (Accidental_Flat == art->accidental_mark) {
			FLAG_FLAT(x, y+1.5*LINE_H, 1);
		}
    } else if(art_type==Articulation_Down_Bow) {
        if (art_placement_above) {
            ART_BOWDOWN(x-5, y-LINE_H*1.8);
        } else {
            ART_BOWDOWN_BELOW(x-5, y-LINE_H*3.5);
        }
    } else if(art_type==Articulation_Up_Bow) {
        if (art_placement_above) {
            ART_BOWUP(x-5, y-LINE_H*1.8);
        } else {
            ART_BOWUP_BELOW(x-5, y-LINE_H*3.5);
        }
    } /*else if ((art_type >= Articulation_Finger_1 && art_type<=Articulation_Finger_5) || art_type==Articulation_Open_String) {
		//fingering_placement ָ��
        char tmp_finger[64];
        if (art_type==Articulation_Open_String) {
            strcpy(tmp_finger, "30");
        }else{
            sprintf(tmp_finger, "3%x",art_type-Articulation_Finger_1+1);
        }
        if (art_type==Articulation_Finger_1)
            x+=1;

        if (art_placement_above) {
            y+=-1.5*LINE_H;
        } else {
            y+=+1.5*LINE_H;
        }
        //"39"
        GLYPH_Petrucci(x+2, y-0, GLYPH_FINGER_SIZE, 0, tmp_finger);
    }*/ else if (art_type == Articulation_Pedal_Down || art_type==Articulation_Pedal_Up) {
        float tmp_x=x;
        float tmp_y=start_y+LINE_H*8+STAFF_OFFSET[1];	// - note_art->art_offset.offset_y*OFFSET_Y_UNIT;
        if (art_type==Articulation_Pedal_Down) {		//����̤��
            ART_PEDAL_DOWN(tmp_x-LINE_H*1, tmp_y);
        } else if (art_type==Articulation_Pedal_Up) {		//�ɿ�̤��
            ART_PEDAL_UP(tmp_x-LINE_H*1, tmp_y);
        } else if (art_type==Articulation_Toe_Pedal || art_type==Articulation_Heel_Pedal){
			//...
        }else{
            NSLog("Error: unknown aa art_type=0x%x", art_type);
        }
    } else if(art_type == Articulation_Major_Trill || art_type==Articulation_Minor_Trill) {
        //...
    } else {
        return false;
    }
    return true;
}

bool VmusImage::isNote(const std::shared_ptr<OveNote>& note, const std::shared_ptr<OveBeam>& beam)
{
	std::shared_ptr<BeamElem>& elem0 = beam->beam_elems.front();
	//bool ret = ((beam->staff == note->staff || beam->stop_staff == note->staff) && (beam->voice == note->voice) && (!beam->isGrace == !note->isGrace)
	//	&& (elem0->start_measure_offset <= note->pos.start_offset && (elem0->stop_measure_offset >= note->pos.start_offset || elem0->stop_measure_pos > 0)));
	bool ret = ((beam->voice == note->voice) && (!beam->isGrace == !note->isGrace)
				&& (elem0->start_measure_offset <= note->pos.start_offset && (elem0->stop_measure_offset >= note->pos.start_offset || elem0->stop_measure_pos > 0)));
	if (ret && beam->staff < 3 && note->staff > 2)
		ret = false;
	return ret;
}

float VmusImage::checkSlurY(const std::shared_ptr<MeasureSlur>& slur, const std::shared_ptr<OveMeasure>& measure, const std::shared_ptr<OveNote>& note, float start_x, float start_y, float slurY)
{
	if (note->inBeam && note->stem_up == slur->slur1_above) {
		for (auto beam = measure->beams.begin(); beam != measure->beams.end(); beam++) {
			if (isNote(note, *beam)) {
				MyRect beam_pos = getBeamRect(*beam, start_x, start_y, measure, false);
				float tmp_x = start_x+MEAS_LEFT_MARGIN+note->pos.start_offset*OFFSET_X_UNIT;
				if (note->stem_up)
					tmp_x += LINE_H;
				float tmp_y2 = beam_pos.size.height/beam_pos.size.width*(tmp_x-beam_pos.origin.x)*1+beam_pos.origin.y;
				if (slur->slur1_above) {
					if (slurY >= tmp_y2)
						slurY = tmp_y2-LINE_H/2;
				} else {
					if (slurY <= tmp_y2)
						slurY = tmp_y2+LINE_H/2;
				}
			}
		}
	}
	return slurY;
}

void VmusImage::drawSvgRepeat(const std::shared_ptr<OveMeasure>& measure, float start_x, float start_y)
{
    //repeat_type
    if (measure->repeat_type!=Repeat_Null)
    {
        float tmp_x;	//=start_x+measure->meas_length_size*OFFSET_X_UNIT*0.5;
        float tmp_y=start_y + STAFF_OFFSET[1]+STAFF_OFFSET[1]/2+1*LINE_H;

        if (measure->repeat_type==Repeat_Segno) {		//���ص�
            tmp_x=start_x+(measure->repeate_symbol_pos.start_offset+measure->repeat_offset.offset_x)*OFFSET_X_UNIT;
			if (measure->repeat_offset.offset_y)
				tmp_y = start_y+measure->repeat_offset.offset_y*OFFSET_Y_UNIT;
			else
				tmp_y=start_y + STAFF_OFFSET[1]-LINE_H*2 + measure->repeat_offset.offset_y*OFFSET_Y_UNIT;
            REPEAT_SEGNO(tmp_x, tmp_y);
        } else if (measure->repeat_type==Repeat_Coda) {		//���ص�ToCada
            tmp_x=start_x+(measure->repeate_symbol_pos.start_offset+measure->repeat_offset.offset_x)*OFFSET_X_UNIT;
            tmp_y=start_y + STAFF_OFFSET[0]-LINE_H*2;	// + measure->repeat_offset.offset_y*OFFSET_Y_UNIT;
            REPEAT_CODA(tmp_x-LINE_H, tmp_y);
            TEXT_RIGHT_ITALIC(tmp_x+5*LINE_H, tmp_y-2*LINE_H, EXPR_FONT_SIZE, "Coda");
        } else if (measure->repeat_type==Repeat_ToCoda) {
            tmp_x=start_x+(measure->repeate_symbol_pos.start_offset+measure->repeat_offset.offset_x)*OFFSET_X_UNIT;
            tmp_y=start_y + STAFF_OFFSET[1]-LINE_H*2 + measure->repeat_offset.offset_y*OFFSET_Y_UNIT;
            tmp_x += MEAS_LEFT_MARGIN+measure->meas_length_size*OFFSET_X_UNIT;
            REPEAT_CODA(tmp_x, tmp_y);
            TEXT_RIGHT_ITALIC(tmp_x-LINE_H*7, tmp_y-20, EXPR_FONT_SIZE, "To Coda");
        } else if (measure->repeat_type==Repeat_DSAlCoda) {
            tmp_x=start_x+measure->meas_length_size*OFFSET_X_UNIT+MEAS_LEFT_MARGIN;
            TEXT_RIGHT_ITALIC(tmp_x, tmp_y-15, EXPR_FONT_SIZE, "D.S. al Coda");
        } else if (measure->repeat_type==Repeat_DSAlFine) {
            tmp_x=start_x+measure->meas_length_size*OFFSET_X_UNIT+MEAS_LEFT_MARGIN;
            TEXT_RIGHT_ITALIC(tmp_x, tmp_y-0*LINE_H, EXPR_FONT_SIZE, "D.S. al Fine");
        } else if (measure->repeat_type==Repeat_DCAlCoda) {
            tmp_x=start_x+measure->meas_length_size*OFFSET_X_UNIT+MEAS_LEFT_MARGIN;
            TEXT_RIGHT_ITALIC(tmp_x, tmp_y-15, EXPR_FONT_SIZE, "D.C. al Code");
        } else if (measure->repeat_type==Repeat_DC) {
			tmp_x = start_x+measure->meas_length_size*OFFSET_X_UNIT+MEAS_LEFT_MARGIN;
			TEXT_RIGHT_ITALIC(tmp_x, tmp_y-15, EXPR_FONT_SIZE, "D.C.");
		} else if (measure->repeat_type==Repeat_DCAlFine) {		// ���ص���ͷ��Ȼ��play��"Fine"����
            tmp_x=start_x+measure->meas_length_size*OFFSET_X_UNIT+MEAS_LEFT_MARGIN;
            TEXT_RIGHT_ITALIC(tmp_x, tmp_y-15, EXPR_FONT_SIZE, "D.C. al Fine");
        } else if (measure->repeat_type==Repeat_Fine) {
            tmp_x=start_x+measure->meas_length_size*OFFSET_X_UNIT+MEAS_LEFT_MARGIN;
            TEXT_RIGHT_ITALIC(tmp_x, tmp_y-15, EXPR_FONT_SIZE, "Fine");
        } else {
            NSLog("repeat_type=%d at measure(%d)", measure->repeat_type, measure->number);
        }
    }
}

void VmusImage::drawSvgTexts(const std::shared_ptr<OveMeasure>& measure, float start_x, float start_y)
{
    if (!measure->meas_texts.empty()) {
		for (auto text = measure->meas_texts.begin(); text != measure->meas_texts.end(); text++) {
			if ((*text)->text != "") {
                float x1 = start_x+MEAS_LEFT_MARGIN+((*text)->pos.start_offset+(*text)->offset_x)*OFFSET_X_UNIT;
				float y1 = start_y-1*LINE_H+((*text)->offset_y)*OFFSET_Y_UNIT;
                if ((*text)->staff>1)
                    y1+=STAFF_OFFSET[(*text)->staff-1];

				float font_size = (*text)->font_size*0.9;
				y1 -= font_size*0.5;
				TEXT_ATTR(x1, y1, font_size, (*text)->text.c_str(), (*text)->isBold, (*text)->isItalic);
            }else{
                NSLog("empty measure text at measure(%d)", measure->number);
            }
        }
    }
}

void VmusImage::drawSvgSlurs(const std::shared_ptr<OveMeasure>& measure, float start_x, float start_y, const std::shared_ptr<OveLine>& ove_line)
{
	int staff_count = ove_line->staves.size();
	//slur ����  ����һ����û��slurû�л���
	if (measure->number==ove_line->begin_bar) {
		for (int ss=0; ss<SLUR_CONTINUE_NUM; ss++) {
			if (slur_continue_info[ss].validate) {
				float x1 = start_x-LINE_H*2;
				float x2 = start_x+MEAS_LEFT_MARGIN+slur_continue_info[ss].stop_offset*OFFSET_X_UNIT;

				float y2 = start_y+ lineToY(slur_continue_info[ss].right_line, slur_continue_info[ss].stop_staff);
				float y1 = 0;
				if (!slur_continue_info[ss].above) {
					y1=y2+LINE_H*1.5f;
				}else{
					if (slur_continue_info[ss].right_line > 8)
						y1 = start_y+lineToY(8, slur_continue_info[ss].stop_staff);
					else
						y1=y2-LINE_H*1.5f;
				}

				std::shared_ptr<OveMeasure> next_measure = nullptr;
				for (int nn=0; nn<slur_continue_info[ss].stop_measure; nn++) {
					if (measure->number+nn >= music->measures.size()) {
						next_measure = music->measures.back();
					} else {
						next_measure = music->measures[measure->number+nn];
					}
					x2+=next_measure->meas_length_size*OFFSET_X_UNIT+MEAS_LEFT_MARGIN+MEAS_RIGHT_MARGIN;
				}
				if (x2 <= screen_width-MARGIN_RIGHT) {
					drawSvgCurveLine(2, x1, y1, x2, y2 ,slur_continue_info[ss].above);
					slur_continue_info[ss].validate=false;
				} else {
					//���е�ǰ��Σ�
					y2 = y1;
					drawSvgCurveLine(2, x1, y1, screen_width-MARGIN_RIGHT, y2, slur_continue_info[ss].above);
					//���Ҫ�����ˣ��Ͱ��ӳ���slur��������
					slur_continue_info[ss].stop_measure -= ove_line->begin_bar+ove_line->bar_count-measure->number;
				}
			}
		}
	}

	//slur����
	if (!measure->slurs.empty()) {
		for (auto it = measure->slurs.begin(); it != measure->slurs.end(); it++) {
			auto& slur = *it;
			if (slur->staff>staff_count)
				continue;

			float x1 = start_x+MEAS_LEFT_MARGIN+slur->pos.start_offset*OFFSET_X_UNIT;
			float x2=start_x+MEAS_LEFT_MARGIN+slur->offset.stop_offset*OFFSET_X_UNIT;

			float y1 = start_y+ lineToY(slur->pair_ends.left_line, slur->staff);
			float y2 = start_y+ lineToY(slur->pair_ends.right_line, slur->stop_staff);

			std::shared_ptr<OveMeasure> next_measure;
			for (int nn=0; nn<slur->offset.stop_measure; nn++) {
				if (measure->number+nn >= music->measures.size()) {
					next_measure = music->measures.back();
				} else {
					next_measure = music->measures[measure->number+nn];
				}
				x2+=next_measure->meas_length_size*OFFSET_X_UNIT+MEAS_LEFT_MARGIN+MEAS_RIGHT_MARGIN;
			}

			//if (next_measure)
			{
				auto& note1 = slur->slur_start_note;
				auto& note2 = slur->slur_stop_note;
				//Ѱ��slur����note
				if (note1 == nullptr) {
					for (auto it = measure->notes.begin(); it != measure->notes.end(); it++) {
						std::shared_ptr<OveNote> note = *it;
						if(note->pos.start_offset>=slur->pos.start_offset && note->staff==slur->staff){
							note1=note;
							break;
						}
					}
				}

				//Ѱ��slur�յ��note
				if (!note2) {
					if (measure->number+slur->offset.stop_measure >= music->measures.size()) {
						next_measure = music->measures.back();
					} else {
						next_measure = music->measures[measure->number+slur->offset.stop_measure];
					}
					for (int nn = 0; nn < next_measure->notes.size(); nn++)
					{
						auto& note = next_measure->notes[nn];
						if (note->pos.start_offset >= slur->offset.stop_offset && note->staff == slur->stop_staff)
						{
							note2 = note;
							break;
						}
					}
				}

				if (note1)
				{
					if (!note1->note_elems.empty())
					{
						auto& firstElem1 = note1->note_elems.front();
						if (firstElem1->offsetStaff==1 && slur->pair_ends.left_line>=0)
							y1+=STAFF_OFFSET[note1->staff];
					}
					y1 = checkSlurY(slur, measure, note1, start_x, start_y, y1);
				}
				//���slur�յ��note��staff�仯�ˣ��͸ı�y2����

				if (!note2) {
					//...
				} else {
					if (!note2->note_elems.empty())
					{
						NoteElem *firstElem2=note2->note_elems.front().get();
						if (firstElem2->offsetStaff==1 && slur->pair_ends.right_line>=0)
							y2+=STAFF_OFFSET[note2->staff];
					}
					if (!slur->offset.stop_measure && !note1->isGrace)
						y2 = checkSlurY(slur, measure, note2, start_x, start_y, y2);
				}
			}

			if (x1>=x2) {		//���� yiyin
				x2=x1+LINE_H*1.0;
			} else {
				x1+=LINE_H*0.8f;
			}
			if (x2<=screen_width-MARGIN_RIGHT) {
				drawSvgCurveLine(2, x1, y1, x2, y2 ,slur->slur1_above);
			}else{
				//���е�ǰ��Σ�
				if (slur->slur1_above) {
					y2=y1-LINE_H*2;
					if (slur->staff > 0 && y2 > start_y+STAFF_OFFSET[slur->staff-1]-LINE_H*3)
						y2 = start_y+STAFF_OFFSET[slur->staff-1]-LINE_H*3;
				}else{
					y2=y1+LINE_H*2;
					if (slur->staff > 0 && y2 < start_y+STAFF_OFFSET[slur->staff-1]+LINE_H*6)
						y2 = start_y+STAFF_OFFSET[slur->staff-1]+LINE_H*6;
				}
				//���Ҫ�����ˣ��Ͱ��ӳ���slur��������
				drawSvgCurveLine(2, x1, y1, screen_width-MARGIN_RIGHT, y2 ,slur->slur1_above);
#if 1
				for (int nn=0; nn<SLUR_CONTINUE_NUM; nn++)
				{
					if (!slur_continue_info[nn].validate)
					{
						slur_continue_info[nn].above=slur->slur1_above;
						slur_continue_info[nn].validate=true;
						slur_continue_info[nn].stop_staff=slur->stop_staff;
						slur_continue_info[nn].right_line=slur->pair_ends.right_line;
						slur_continue_info[nn].stop_offset=slur->offset.stop_offset;
						slur_continue_info[nn].stop_measure=slur->offset.stop_measure-((ove_line->begin_bar+ove_line->bar_count)-measure->number);
						break;
					}
				}
#endif
			}
		}
	}
}

void VmusImage::drawSvgTies(const std::shared_ptr<OveMeasure>& measure, float start_x, float start_y, const std::shared_ptr<OveLine>& ove_line)
{
	//tie��
	int staff_count = ove_line->staves.size();
	if (!measure->ties.empty()) {
		for (auto it = measure->ties.begin(); it != measure->ties.end(); it++) {
			auto& tie = *it;
			if (tie->staff>staff_count)
				continue;

			float x1 = start_x+MEAS_LEFT_MARGIN+tie->pos.start_offset*OFFSET_X_UNIT+1.0f*LINE_H;
			if (tie->pos.start_offset<0)
				x1-=2*MEAS_LEFT_MARGIN;

			float y1 = start_y+ lineToY(tie->pair_ends.left_line, tie->staff);
			float y2 = start_y+ lineToY(tie->pair_ends.right_line, tie->staff);
			if (tie->above) {
				y1-=LINE_H;
				y2-=LINE_H;
			}else{
				y1+=LINE_H;
				y2+=LINE_H;
			}

			float x2=start_x;
			std::shared_ptr<OveMeasure> next_measure = nullptr;
			for (int nn=0; nn<tie->offset.stop_measure; nn++)
			{
				next_measure = music->measures[measure->number+nn];
				x2+=MEAS_LEFT_MARGIN+next_measure->meas_length_size*OFFSET_X_UNIT+MEAS_RIGHT_MARGIN;
			}
			x2+=MEAS_LEFT_MARGIN+tie->offset.stop_offset*OFFSET_X_UNIT-0.0f*LINE_H;

			if (x2<=screen_width-MARGIN_RIGHT) {
				drawSvgCurveLine(2, x1, y1, x2, y2 ,tie->above);
			}else{
#if 1
				//���е�ǰ��Σ�
				drawSvgCurveLine(2, x1, y1, screen_width-MARGIN_RIGHT, y1 ,tie->above);
				//���Ҫ�����ˣ��Ͱ��ӳ���tie����������
				for (int nn=0; nn<SLUR_CONTINUE_NUM; nn++)
				{
					if (!slur_continue_info[nn].validate)
					{
						slur_continue_info[nn].above=tie->above;
						slur_continue_info[nn].validate=true;
						slur_continue_info[nn].stop_staff=tie->staff;
						slur_continue_info[nn].right_line=tie->pair_ends.right_line;
						slur_continue_info[nn].stop_offset=tie->offset.stop_offset;
						slur_continue_info[nn].stop_measure=tie->offset.stop_measure-((ove_line->begin_bar+ove_line->bar_count)-measure->number);
						break;
					}
				}
#endif
			}
		}
	}
}

void VmusImage::drawSvgPedals(const std::shared_ptr<OveMeasure>& measure, float start_x, float start_y, const std::shared_ptr<OveLine>& ove_line, const std::shared_ptr<OveLine>& nextLine)
{
	int staff_count = ove_line->staves.size();
	if (!measure->pedals.empty()) {
		for (auto it = measure->pedals.begin(); it != measure->pedals.end(); it++) {
			auto& pedal = *it;
			if (pedal->staff > staff_count)
				continue;
			float x1=start_x+MEAS_LEFT_MARGIN+pedal->pos.start_offset*OFFSET_X_UNIT;
			float y1=start_y+lineToY(pedal->pair_ends.left_line, pedal->staff);
			float x2 = start_x;		//=start_x+MEAS_LEFT_MARGIN+pedal->offset->stop_offset*OFFSET_X_UNIT;
			float y2=start_y+lineToY(pedal->pair_ends.left_line, pedal->staff);

			std::shared_ptr<OveMeasure> next_measure = nullptr;
			for (int nn=0; nn<pedal->offset.stop_measure; nn++)
			{
				next_measure = music->measures[measure->number+nn];
				x2+=MEAS_LEFT_MARGIN+next_measure->meas_length_size*OFFSET_X_UNIT+MEAS_RIGHT_MARGIN;
			}
			x2+=MEAS_LEFT_MARGIN+pedal->offset.stop_offset*OFFSET_X_UNIT;

			if (pedal->isLine) {
				LINE(x1, y1-5, x1+3, y1);
				LINE(x1+3, y1, x2-3, y2);

				if (x2>screen_width-MARGIN_RIGHT) {
					if (nextLine) {
						y1 += STAFF_OFFSET[nextLine->staves.size()-1]+(4+GROUP_STAFF_NEXT)*LINE_H;
						//analyze never read
						//y2 += STAFF_OFFSET[nextLine->staves.size()-1]+(4+GROUP_STAFF_NEXT)*LINE_H;
					} else {
						y1 += STAFF_OFFSET[staff_count-1]+(4+GROUP_STAFF_NEXT)*LINE_H;
						//analyze never read
						//y2 += STAFF_OFFSET[staff_count-1]+(4+GROUP_STAFF_NEXT)*LINE_H;
					}
					x1=MARGIN_LEFT+STAFF_HEADER_WIDTH;
					x2=x2-(screen_width-MARGIN_RIGHT)+x1;//+MEAS_LEFT_MARGIN;
					LINE(x1, y1, x2-3, y1);
					LINE(x2, y1-5, x2-3, y1);
				} else {
					LINE(x2, y1-5, x2-3, y1);
				}
			} else {
				ART_PEDAL_DOWN(x1-LINE_H*0, y1-LINE_H*0.5);//����̤��
				ART_PEDAL_UP(x2-LINE_H*1.5, y2-LINE_H*0.5); //�ɿ�̤��
			}
		}
	}
}

void VmusImage::drawSvgTuplets(const std::shared_ptr<OveMeasure>& measure, float start_x, float start_y, const std::shared_ptr<OveLine>& ove_line)
{
	int staff_count = ove_line->staves.size();
	if (!measure->tuplets.empty()) {
		for (auto it = measure->tuplets.begin(); it != measure->tuplets.end(); it++) {
			auto& tuplet = *it;
			if (tuplet->staff>staff_count)
				continue;

			float x1 = start_x+MEAS_LEFT_MARGIN+tuplet->pos.start_offset*OFFSET_X_UNIT;
			float y1 = start_y+ lineToY(tuplet->pair_ends.left_line, tuplet->staff);
			float y2 = start_y+ lineToY(tuplet->pair_ends.right_line, tuplet->staff);
			float x2 = start_x;

			std::shared_ptr<OveMeasure> next_measure = nullptr;
			for (int nn=0; nn<tuplet->offset.stop_measure; nn++)
			{
				next_measure = music->measures[measure->number+nn];
				x2+=MEAS_LEFT_MARGIN+next_measure->meas_length_size*OFFSET_X_UNIT+MEAS_RIGHT_MARGIN;
			}
			x2+=MEAS_LEFT_MARGIN+tuplet->offset.stop_offset*OFFSET_X_UNIT;

			LINE(x1, y1, x1, y1+5);
			LINE(x2, y2, x2, y2+5);
			LINE(x1, y1, x2, y2);
			char tmp_tuplet[8];
			sprintf(tmp_tuplet, "%d", tuplet->tuplet);
			int tmp_x=(x1+x2)/2;
#ifdef OVE_IPHONE
			int tmp_y=(y1+y2)/2-15;
			TEXT(tmp_x, tmp_y, EXPR_FONT_SIZE, tmp_tuplet);
#else
			int tmp_y=(y1+y2)/2-16;
			TEXT(tmp_x, tmp_y, EXPR_FONT_SIZE, tmp_tuplet);
#endif
		}
	}
}

void VmusImage::drawSvgLyrics(const std::shared_ptr<OveMeasure>& measure, float start_x, float start_y)
{
	//lyrics
#ifndef OVE_IPHONE
	if (!measure->lyrics.empty()) {
		for (auto it = measure->lyrics.begin(); it != measure->lyrics.end(); it++) {
			auto& lyric = *it;
			if (lyric->lyric_text.size()>0) {
				CGSize tmp_size = GetFontPixelSize(lyric->lyric_text, EXPR_FONT_SIZE);
				int tmp_x = start_x+MEAS_LEFT_MARGIN+lyric->pos.start_offset*OFFSET_X_UNIT-tmp_size.width*0.5;
				if (tmp_x<start_x+LINE_H)
					tmp_x=start_x+LINE_H;

				int tmp_y = start_y+lyric->verse*LINE_H*2;
				if (lyric->offset.offset_y==0) {
					tmp_y+=8*LINE_H;
				}else{
					tmp_y-=lyric->offset.offset_y*OFFSET_Y_UNIT;
				}

				tmp_y+=STAFF_OFFSET[lyric->staff-1];
				TEXT(tmp_x, tmp_y-tmp_size.height*0.5, EXPR_FONT_SIZE, lyric->lyric_text.c_str());
			}else{
				NSLog("empty lyrics text at measure(%d)", measure->number);
			}
		}
	}
#endif
}

void VmusImage::drawSvgDynamics(const std::shared_ptr<OveMeasure>& measure, float start_x, float start_y)
{
	//harmony_guitar_framesharmony_guitar_frames
	if (!measure->harmony_guitar_frames.empty()) {
		for (int i = 0; i < measure->harmony_guitar_frames.size(); i++) {
			const std::shared_ptr<HarmonyGuitarFrame>& tmp = measure->harmony_guitar_frames[i];
			NSLog("measure(%d) root:%d bass:%d pos:[%d] type:%d", measure->number, tmp->root, tmp->bass, tmp->pos.start_offset, tmp->type);
		}
	}
	//dynamics
	if (!measure->dynamics.empty()) {
		for (auto it = measure->dynamics.begin(); it != measure->dynamics.end(); it++) {
			auto& dyn = *it;
			int tmp_x = start_x+MEAS_LEFT_MARGIN+dyn->pos.start_offset*OFFSET_X_UNIT;	//-LINE_H;
			int tmp_y = start_y+0*LINE_H+dyn->offset_y*OFFSET_Y_UNIT;
			if (dyn->staff>=2)
				tmp_y+=STAFF_OFFSET[dyn->staff-1]-1*LINE_H;

			if (dyn->dynamics_type==Dynamics_p) {
				//str_dyn = "p";
				DYNAMICS_P(tmp_x-2, tmp_y-0);
			}else if (dyn->dynamics_type==Dynamics_pp) {
				DYNAMICS_PP(tmp_x-2, tmp_y-0);
				//str_dyn = "pp";
			}else if (dyn->dynamics_type==Dynamics_ppp) {
				DYNAMICS_PPP(tmp_x-2, tmp_y-0);
				//str_dyn = "ppp";
			}else if (dyn->dynamics_type==Dynamics_pppp) {
				DYNAMICS_PPPP(tmp_x-2, tmp_y-0);
				//str_dyn = "pppp";
			}else if (dyn->dynamics_type==Dynamics_mp) {
				DYNAMICS_MP(tmp_x-2, tmp_y-0);
			}else if (dyn->dynamics_type==Dynamics_f) {
				DYNAMICS_F(tmp_x-2, tmp_y-0);
			}else if (dyn->dynamics_type==Dynamics_ff) {
				DYNAMICS_FF(tmp_x-2, tmp_y-0);
			}else if (dyn->dynamics_type==Dynamics_fff) {
				DYNAMICS_FFF(tmp_x-2, tmp_y-0);
			}else if (dyn->dynamics_type==Dynamics_ffff) {
				DYNAMICS_FFFF(tmp_x-2, tmp_y-0);
			}else if (dyn->dynamics_type==Dynamics_mf) {
				DYNAMICS_MF(tmp_x-2, tmp_y-0);
			}else if (dyn->dynamics_type==Dynamics_sf) {
				DYNAMICS_SF(tmp_x-2, tmp_y-0);
			}else if (dyn->dynamics_type==Dynamics_sff) {
				DYNAMICS_SFF(tmp_x-2, tmp_y-0);
			}else if (dyn->dynamics_type==Dynamics_fz) {
				DYNAMICS_FZ(tmp_x-2, tmp_y-0);
			}else if (dyn->dynamics_type==Dynamics_sfz) {
				DYNAMICS_SFZ(tmp_x-2, tmp_y-0);
			}else if (dyn->dynamics_type==Dynamics_fp || dyn->dynamics_type==Dynamics_sffz) {
				DYNAMICS_FP(tmp_x-2, tmp_y-0);
			}else if (dyn->dynamics_type==Dynamics_sfp) {
				DYNAMICS_SFP(tmp_x-2, tmp_y-0);
			}else{
				NSLog("Error unknow dynamics_type=%d", dyn->dynamics_type);
			}
		}
	}
}

void VmusImage::drawSvgExpressions(const std::shared_ptr<OveMeasure>& measure, float start_x, float start_y)
{
	//expresssions
	if (!measure->expressions.empty()) {
		for (int i = 0; i < measure->expressions.size(); i++) {
			auto& expr = measure->expressions[i];
			if (expr->exp_text.size() > 0) {
				int tmp_x = start_x+MEAS_LEFT_MARGIN+expr->pos.start_offset*OFFSET_X_UNIT;
				int tmp_y = start_y+LINE_H*1+expr->offset_y*OFFSET_Y_UNIT;

				if (expr->staff >= 2) {
					tmp_y += STAFF_OFFSET[expr->staff-1]-LINE_H*0;
				} else if (expr->offset_y < 0) {
					tmp_y -= LINE_H*3;
				}
				if (tmp_x < MARGIN_LEFT+STAFF_HEADER_WIDTH/2)
					tmp_x = MARGIN_LEFT+STAFF_HEADER_WIDTH/2;
				if (tmp_y < 30)
					tmp_y = 30;
				TEXT(tmp_x+2, tmp_y-5, EXPR_FONT_SIZE, expr->exp_text.c_str());
			} else {
				NSLog("empty expresssion text at measure(%d)", measure->number);
			}
		}
	}
}

void VmusImage::drawSvgOctaveShift(const std::shared_ptr<OveMeasure>& measure, float start_x, float start_y, const std::shared_ptr<OveLine>& ove_line, int line_index)
{
	//octave shift �޸�8��
	if (!measure->octaves.empty()) {
		for (auto it = measure->octaves.begin(); it != measure->octaves.end(); it++) {
			auto& octave = *it;
			static int octave_x1[2]={0, 0};
			static int octave_y1[2]={0, 0};
			static int octave_y_continue1[2]={0, 0};
			int tmp_y=start_y+LINE_H*1+STAFF_OFFSET[octave->staff-1]+octave->offset_y*OFFSET_Y_UNIT-LINE_H;

			if (octave->octaveShiftType>=OctaveShift_8_Start && octave->octaveShiftType<=OctaveShift_Minus_15_Start) {
				octave_x1[octave->staff-1] = start_x+MEAS_LEFT_MARGIN+(octave->pos.start_offset)*OFFSET_X_UNIT;
				if (octave_x1[octave->staff-1]<MEAS_LEFT_MARGIN + STAFF_HEADER_WIDTH)
					octave_x1[octave->staff-1]=MEAS_LEFT_MARGIN+STAFF_HEADER_WIDTH;

				octave_y1[octave->staff-1] = tmp_y;
				octave_y_continue1[octave->staff-1]=octave_y1[octave->staff-1];
				if (octave->octaveShiftType==OctaveShift_8_Start) {		//8va���8��
					OCTAVE_ATTAVA(octave_x1[octave->staff-1]-LINE_H*0.5, octave_y1[octave->staff-1]+1*LINE_H);
				} else if (octave->octaveShiftType==OctaveShift_Minus_8_Start) {		//8vb����8��
					OCTAVE_ATTAVB(octave_x1[octave->staff-1]-LINE_H*0.5, octave_y1[octave->staff-1]+LINE_H);
				} else if (octave->octaveShiftType==OctaveShift_15_Start) {		//15va�������8��
					OCTAVE_QUINDICESIMA(octave_x1[octave->staff-1]-LINE_H*0.5, octave_y1[octave->staff-1]+1*LINE_H);
				} else if (octave->octaveShiftType==OctaveShift_Minus_15_Start) {		//15va��������8��
					TEXT(octave_x1[octave->staff-1]-LINE_H*0.5, octave_y1[octave->staff-1]+LINE_H, NORMAL_FONT_SIZE, "15mb");
				}
				for (int k = 0; k < OCTAVE_CONTINUE_NUM; k++) {
					if (!octave_continue_info[k].validate) {
						octave_continue_info[k].validate = true;
						octave_continue_info[k].offset_y = octave->offset_y;
						octave_continue_info[k].staff = octave->staff;
						octave_continue_info[k].start_line = line_index;
						octave_continue_info[k].octave_x1 = octave_x1[octave->staff-1];
						octave_continue_info[k].octave_y1 = octave_y1[octave->staff-1];
					}
				}
			} else if (octave->octaveShiftType>=OctaveShift_8_Stop && octave->octaveShiftType<=OctaveShift_Minus_15_Stop) {
				for (int k = 0; k < OCTAVE_CONTINUE_NUM; k++) {
					if (octave_continue_info[k].validate && octave_continue_info[k].staff == octave->staff)
						octave_continue_info[k].validate = false;
				}
				//draw current line
				int octave_x2 = start_x+MEAS_LEFT_MARGIN+(octave->pos.start_offset)*OFFSET_X_UNIT+LINE_H;
				if (octave->length>0)
					octave_x2 = start_x+MEAS_LEFT_MARGIN+(+octave->length)*OFFSET_X_UNIT;
				int octave_y2 = tmp_y;

				if (octave_y2!=octave_y1[octave->staff-1]) {
					LINE_DOT(MARGIN_LEFT+STAFF_HEADER_WIDTH,octave_y2, octave_x2, octave_y2);
				} else {
					if (octave_x2 < octave_x1[octave->staff-1]+3*LINE_H)
						octave_x2 = octave_x1[octave->staff-1]+3*LINE_H;
					LINE_DOT(octave_x1[octave->staff-1]+2*LINE_H, octave_y1[octave->staff-1], octave_x2, octave_y1[octave->staff-1]);
				}

				LINE(octave_x2-5, octave_y2, octave_x2, octave_y2);
				if (octave->octaveShiftType==OctaveShift_8_Stop || octave->octaveShiftType==OctaveShift_15_Start) {
					LINE(octave_x2, octave_y2, octave_x2, octave_y2+8);
				} else {
					LINE(octave_x2, octave_y2, octave_x2, octave_y2-8);
				}
			} else if(octave->octaveShiftType>=OctaveShift_8_Continue && octave->octaveShiftType<=OctaveShift_Minus_15_Continue) {
				//if (measure->number==ove_line->begin_bar+ove_line->bar_count-1) {		//�������һ��С��
				//	if (tmp_y!=octave_y_continue1) {
				//		LINE_DOT(MARGIN_LEFT+STAFF_HEADER_WIDTH, tmp_y, screen_width-MARGIN_RIGHT, tmp_y);
				//	}else{
				//		LINE_DOT(octave_x1+6, tmp_y, screen_width-MARGIN_RIGHT, tmp_y);
				//	}
				//	octave_y_continue1=tmp_y;
				//}
			} else if(octave->octaveShiftType==OctaveShift_8_StartStop || octave->octaveShiftType==OctaveShift_Minus_8_StartStop) {
				//draw current line
				octave_x1[octave->staff-1] = 12+start_x+MEAS_LEFT_MARGIN+(octave->pos.start_offset)*OFFSET_X_UNIT;
				int octave_x2 = start_x+MEAS_LEFT_MARGIN+(octave->length)*OFFSET_X_UNIT;
				octave_y1[octave->staff-1] = tmp_y;
				LINE_DOT(octave_x1[octave->staff-1]+LINE_H, octave_y1, octave_x2, octave_y1);
				LINE(octave_x2-5, octave_y1[octave->staff-1], octave_x2, octave_y1[octave->staff-1]);

				if (octave->octaveShiftType==OctaveShift_8_StartStop) {	//8va���8��
					OCTAVE_ATTAVA(octave_x1[octave->staff-1]-LINE_H*0.5, octave_y1[octave->staff-1]-LINE_H);
					LINE(octave_x2, octave_y1[octave->staff-1], octave_x2, octave_y1[octave->staff-1]+8);
				} else if (octave->octaveShiftType==OctaveShift_Minus_8_Start) {	//8vb����8��
					OCTAVE_ATTAVB(octave_x1[octave->staff-1]-LINE_H*0.5, octave_y1[octave->staff-1]-LINE_H);
					LINE(octave_x2, octave_y1[octave->staff-1], octave_x2, octave_y1[octave->staff-1]-8);
				} else if (octave->octaveShiftType==OctaveShift_15_Start) {		//15va�������8��
					OCTAVE_QUINDICESIMA(octave_x1[octave->staff-1]-LINE_H*0.5, octave_y1[octave->staff-1]-LINE_H);
					LINE(octave_x2, octave_y1[octave->staff-1], octave_x2, octave_y1[octave->staff-1]+8);
				} else if (octave->octaveShiftType==OctaveShift_Minus_15_Start) {		//15va��������8��
					TEXT(octave_x1[octave->staff-1]-LINE_H*0.5, octave_y1[octave->staff-1]-LINE_H, NORMAL_FONT_SIZE, "15mb");
					LINE(octave_x2, octave_y1[octave->staff-1], octave_x2, octave_y1[octave->staff-1]-8);
				}
			} else {
				NSLog("Error: unknow octave type=%d at measure=%d", octave->octaveShiftType, measure->number);
			}
		}
	}
	if (measure->number == ove_line->begin_bar+ove_line->bar_count-1) {
		for (int k = 0; k < OCTAVE_CONTINUE_NUM; k++) {
			if (octave_continue_info[k].validate) {
				//draw: continue lines
				if (line_index == octave_continue_info[k].start_line) {
					LINE_DOT(octave_continue_info[k].octave_x1+2*LINE_H, octave_continue_info[k].octave_y1, screen_width-MARGIN_RIGHT, octave_continue_info[k].octave_y1);
				} else if (line_index > octave_continue_info[k].start_line) {
					float continue_y = ove_line->y_offset*OFFSET_Y_UNIT+MARGIN_TOP;
					continue_y += 0*LINE_H+STAFF_OFFSET[octave_continue_info[k].staff-1]+octave_continue_info[k].offset_y*OFFSET_Y_UNIT;
					LINE_DOT(MARGIN_LEFT+STAFF_HEADER_WIDTH, continue_y, screen_width-MARGIN_RIGHT, continue_y);
				}
			}
		}
	}
}

void VmusImage::drawSvgGlissandos(const std::shared_ptr<OveMeasure>& measure, float start_x, float start_y, const std::shared_ptr<OveLine>& ove_line, int line_index, int line_count)
{
	int staff_count = ove_line->staves.size();
	if (!measure->glissandos.empty())
	{
		for (auto it = measure->glissandos.begin(); it != measure->glissandos.end(); it++) {
			auto& gliss = *it;
			int tmp_x = start_x+MEAS_LEFT_MARGIN+gliss->pos.start_offset*OFFSET_X_UNIT;
			int tmp_y = start_y-3*LINE_H+lineToY(gliss->pair_ends.left_line, 1);
			{
				int x2=start_x+5;
				std::shared_ptr<OveMeasure> next_measure = nullptr;
				for (int nn=0; nn<gliss->offset.stop_measure; nn++)
				{
					next_measure = music->measures[measure->number+nn];
					x2+=MEAS_LEFT_MARGIN+next_measure->meas_length_size*OFFSET_X_UNIT+MEAS_RIGHT_MARGIN;
				}

				x2+=MEAS_LEFT_MARGIN+gliss->offset.stop_offset*OFFSET_X_UNIT;
				if (gliss->straight_wavy) { //wavy
					LINE_WAVY_HORIZONTAL(tmp_x, x2, tmp_y);
					if (x2>screen_width-MARGIN_RIGHT && line_index<line_count-1)
					{
						tmp_x=MARGIN_LEFT+STAFF_HEADER_WIDTH;
						tmp_y+=STAFF_OFFSET[staff_count-1]+(4+GROUP_STAFF_NEXT)*LINE_H;
						x2=x2-(screen_width-MARGIN_RIGHT)+tmp_x;
						LINE_WAVY_HORIZONTAL(tmp_x, x2, tmp_y);
					}
				} else { //straight
					LINE_W(tmp_x, tmp_y, x2, tmp_y, WAVY_LINE_WIDTH);
					if (x2>screen_width-MARGIN_RIGHT && line_index<line_count-1) {
						tmp_x=MARGIN_LEFT+STAFF_HEADER_WIDTH;
						tmp_y+=STAFF_OFFSET[staff_count-1]+(4+GROUP_STAFF_NEXT)*LINE_H;
						x2=x2-(screen_width-MARGIN_RIGHT)+tmp_x;
						LINE_W(tmp_x, tmp_y, x2, tmp_y, WAVY_LINE_WIDTH);
					}
				}
			}
		}
	}
}

void VmusImage::drawSvgWedges(const std::shared_ptr<OveMeasure>& measure, float start_x, float start_y, const std::shared_ptr<OveLine>& ove_line, int line_index, int line_count)
{
	int staff_count = ove_line->staves.size();
	if (!measure->wedges.empty()) {
		for (auto it = measure->wedges.begin(); it != measure->wedges.end(); it++) {
			auto& wedge = *it;
			int tmp_x = start_x+MEAS_LEFT_MARGIN+wedge->pos.start_offset*OFFSET_X_UNIT;
			int tmp_y = start_y+LINE_H*3+wedge->offset_y*OFFSET_Y_UNIT;
			if (wedge->staff>0 && wedge->staff<=STAFF_COUNT)
				tmp_y+=STAFF_OFFSET[wedge->staff-1];

			if (wedge->wedgeOrExpression) {		//wedge
				int x2=start_x;
				std::shared_ptr<OveMeasure> next_measure = nullptr;
				for (int nn=0; nn<wedge->offset.stop_measure; nn++)
				{
					next_measure = music->measures[measure->number+nn];
					x2+=MEAS_LEFT_MARGIN+next_measure->meas_length_size*OFFSET_X_UNIT+MEAS_RIGHT_MARGIN;
				}
				x2+=MEAS_LEFT_MARGIN+wedge->offset.stop_offset*OFFSET_X_UNIT;
				if (x2>screen_width)
					NSLog("Error wedge is too long, x2=%d", x2);

				if (wedge->wedgeType==Wedge_Cres_Line) { //<
					if (x2>screen_width-MARGIN_RIGHT && line_index<line_count-1) {
						LINE(tmp_x, tmp_y, screen_width-MARGIN_RIGHT, tmp_y-5);
						LINE(tmp_x, tmp_y, screen_width-MARGIN_RIGHT, tmp_y+5);

						tmp_x=MARGIN_LEFT+STAFF_HEADER_WIDTH;
						tmp_y+=STAFF_OFFSET[staff_count-1]+(4+GROUP_STAFF_NEXT)*LINE_H;
						x2=x2-(screen_width-MARGIN_RIGHT)+tmp_x;	//-MEAS_LEFT_MARGIN-MEAS_RIGHT_MARGIN;
						LINE(tmp_x, tmp_y-2, x2, tmp_y-5);
						LINE(tmp_x, tmp_y+2, x2, tmp_y+5);
					} else {
						LINE(tmp_x, tmp_y, x2, tmp_y-5);
						LINE(tmp_x, tmp_y, x2, tmp_y+5);
					}
				} else if (wedge->wedgeType==Wedge_Decresc_Line) { //>
					if (x2>screen_width-MARGIN_RIGHT && line_index<line_count-1) {
						LINE(tmp_x, tmp_y-5, screen_width-MARGIN_RIGHT, tmp_y);
						LINE(tmp_x, tmp_y+5, screen_width-MARGIN_RIGHT, tmp_y);

						tmp_x=MARGIN_LEFT+STAFF_HEADER_WIDTH;
						tmp_y+=STAFF_OFFSET[staff_count-1]+(4+GROUP_STAFF_NEXT)*LINE_H;
						x2=x2-(screen_width-MARGIN_RIGHT)+tmp_x;
						LINE(tmp_x, tmp_y-5, x2, tmp_y);
						LINE(tmp_x, tmp_y+5, x2, tmp_y);
					} else {
						LINE(tmp_x, tmp_y-5, x2, tmp_y);
						LINE(tmp_x, tmp_y+5, x2, tmp_y);
					}
				} else if (wedge->wedgeType==Wedge_Double_Line) { //<>
					LINE(tmp_x, tmp_y-5, x2, tmp_y);
					LINE(tmp_x, tmp_y+5, x2, tmp_y);
					if (x2>screen_width-MARGIN_RIGHT && line_index<line_count-1)
					{
						tmp_x=MARGIN_LEFT+STAFF_HEADER_WIDTH;
						tmp_y+=STAFF_OFFSET[staff_count-1]+(4+GROUP_STAFF_NEXT)*LINE_H;
						x2=x2-(screen_width-MARGIN_RIGHT)+tmp_x;
						LINE(tmp_x, tmp_y, (tmp_x+x2)/2, tmp_y-5);
						LINE(tmp_x, tmp_y, (tmp_x+x2)/2, tmp_y+5);
						LINE((tmp_x+x2)/2, tmp_y-5, x2, tmp_y);
						LINE((tmp_x+x2)/2, tmp_y+5, x2, tmp_y);
					}
				} else {
					NSLog("Unknow wedge type=%d", wedge->wedgeType);
				}
			} else if (wedge->expression_text.size() > 0) { //expression
				TEXT(tmp_x, tmp_y-6, EXPR_FONT_SIZE, wedge->expression_text.c_str());
			} else {
				NSLog("empty wedge->expression_text text at measure(%d)", measure->number);
			}
		}
	}
}

void VmusImage::drawSvgClefs(const std::shared_ptr<OveMeasure>& measure, float start_x, float start_y, const std::shared_ptr<OveLine>& ove_line)
{
	if (measure->number < music->measures.size()-1)
	{
		//if next measure changed clef, show it at the end of current measure
		auto& nextMeasure = music->measures[measure->number+1];
		for (auto clef = nextMeasure->clefs.begin(); clef != nextMeasure->clefs.end(); clef++) {
			if ((*clef)->pos.start_offset == 0 && (*clef)->pos.tick == 0) {
				int tmp_x = start_x+MEAS_LEFT_MARGIN+measure->meas_length_size*OFFSET_X_UNIT-LINE_H*0.5;
				if (Barline_RepeatRight == measure->right_barline)
					tmp_x -= LINE_H*1;

				int tmp_y = start_y;
				if ((*clef)->staff >= 2)
					tmp_y += STAFF_OFFSET[(*clef)->staff-1];
				if (Clef_Treble == (*clef)->clef) {
					CLEF_TREBLE(tmp_x, tmp_y+3.0*LINE_H, 0.7);
				} else {
					CLEF_BASS(tmp_x, tmp_y+1.0*LINE_H, 0.7);
				}
			}
		}
	}
	if (!measure->clefs.empty()) {
		for (auto clef = measure->clefs.begin(); clef != measure->clefs.end(); clef++) {
			if ((*clef)->pos.start_offset == 0 && (*clef)->pos.tick == 0 && measure->number > 0)
				continue;

			int tmp_x;
			if ((*clef)->pos.tick == measure->meas_length_tick) {
				tmp_x = start_x+MEAS_LEFT_MARGIN+measure->meas_length_size*OFFSET_X_UNIT-LINE_H*0.5;
				if (Barline_Default != measure->right_barline)
					tmp_x -= LINE_H*1;
			} else if (0 == measure->number) {
				tmp_x = start_x+MEAS_LEFT_MARGIN+(*clef)->pos.start_offset*OFFSET_X_UNIT+2.0*LINE_H;
			} else if (measure->number == ove_line->begin_bar) {
				tmp_x = start_x+MEAS_LEFT_MARGIN+(*clef)->pos.start_offset*OFFSET_X_UNIT+2.0*LINE_H;
			} else {
				tmp_x = start_x+MEAS_LEFT_MARGIN+(*clef)->pos.start_offset*OFFSET_X_UNIT+2.0*LINE_H;
			}

			int tmp_y = start_y;
			if ((*clef)->staff >= 2)
				tmp_y += STAFF_OFFSET[(*clef)->staff-1];
			if (Clef_Treble == (*clef)->clef) {
				CLEF_TREBLE(tmp_x-LINE_H, tmp_y+3.0*LINE_H, 0.7);
			} else {
				CLEF_BASS(tmp_x, tmp_y+1.0*LINE_H, 0.7);
			}
		}
	}
}

void VmusImage::drawSvgRest(const std::shared_ptr<OveNote>& note, const std::shared_ptr<OveMeasure>& measure, float start_x, float start_y)
{
	if (!note->isRest)
		return;

	int line=note->line;
	//check other voice note
	//if (0 == note->line)
	//{
	//	char buf[16];
	//	sprintf(buf, "%d", note->pos.tick);
	//	auto& notes = measure->sorted_notes[buf];
	//	for (auto tmp_note = notes.begin(); tmp_note != notes.end(); tmp_note++) {
	//		if (note->staff == (*tmp_note)->staff && note->voice != (*tmp_note)->voice) {
	//			if ((*tmp_note)->voice > note->voice && (*tmp_note)->line > -4) {
	//				line = (*tmp_note)->line+4;
	//			} else if ((*tmp_note)->voice < note->voice && (*tmp_note)->line < 6) {
	//				line = (*tmp_note)->line-6;
	//			}
	//			break;
	//		}
	//	}
	//}
	float x = start_x+MEAS_LEFT_MARGIN+note->pos.start_offset*OFFSET_X_UNIT;
	float y = start_y + lineToY(line, note->staff);

	if (note->note_type==Note_Whole) {		//ȫ��ֹ
		x = start_x+MEAS_LEFT_MARGIN+measure->meas_length_size*OFFSET_X_UNIT/2-LINE_H;
		if (note->line==0) {
			y = y-LINE_H*1+2;
		} else {
			//y+=2;
			y = y-LINE_H*1+2;
		}
		LINE_W(x, y, x+LINE_H*1.2, y, LINE_H*0.5);
	} else if (note->note_type==Note_Half) {		//������ֹ��
		LINE_W(x+LINE_H*0.5, y-2, x+LINE_H*2.0, y-2, LINE_H/2);
	} else if (note->note_type == Note_Quarter) {
		RESET_QUARTER(x-LINE_H*0, y+LINE_H*0);
	} else if (note->note_type == Note_Eight) {
		RESET_EIGHT(x-LINE_H*0, y-LINE_H*0.5);
	} else if (note->note_type == Note_Sixteen) {
		RESET_16(x+LINE_H*0.0, y+LINE_H*0.0);
	} else if (note->note_type == Note_32) {
		RESET_32(x+LINE_H*0.0, y+LINE_H*1.5);
	} else {
		NSLog("Error: unknow rest flag. note_type=%d at measure=%d", note->note_type, measure->number);
	}
	note->display_note_x = x;

	if (note->isDot)		//����
	{
		float dot_x=x+LINE_H*1.5;
		if (note->note_type==Note_Whole || note->note_type==Note_Half)
			dot_x+=LINE_H*1.5;

		float dot_y=y-LINE_H*0.5;
		if (note->line%2!=0)
			dot_y+=LINE_H*0.5;

		for (int dot=0; dot<note->isDot; dot++)
			NOTE_DOT(dot_x+LINE_H*dot, dot_y);
	}
	if (!note->note_arts.empty()) {
		for (auto it = note->note_arts.begin(); it != note->note_arts.end(); it++) {
			auto& note_art = *it;
			ArticulationType art_type=note_art->art_type;
			bool art_placement_above=(note_art->art_placement_above == 1) ? true : false;
			float art_y=y, art_x=x;

			if (art_placement_above) {
				if (!note_art->offset.offset_x && !note_art->offset.offset_y) {
					art_y-=LINE_H*2;
				} else {
					art_y-=note_art->offset.offset_y*OFFSET_Y_UNIT;
				}
			} else {
				if (!note_art->offset.offset_x && !note_art->offset.offset_y) {
					art_y+=LINE_H*1;
				} else {
					art_y+=note_art->offset.offset_y*OFFSET_Y_UNIT;
				}
			}
			art_x+=note_art->offset.offset_x*OFFSET_X_UNIT;

			int staff_start_y=start_y+STAFF_OFFSET[note->staff-1];
			if (art_type==Articulation_Pedal_Down || art_type==Articulation_Pedal_Up) {
				staff_start_y=start_y;
			}
			if (!drawSvgArt(note_art ,art_placement_above ,art_x, art_y, staff_start_y)) {}
		}
	}
}

void VmusImage::drawSvgTrill(const std::shared_ptr<NoteArticulation>& note_art, const std::shared_ptr<OveMeasure>& measure, const std::shared_ptr<OveNote>& note, float x, float art_y)
{
	/*
	trillNoteType ����
	���෽�������Դ��������Ϸ��������·���������������ָʾ��С��������ʼ�������࣬��������32���������ٶ�,���Ի�����������������ָʾ��С������������
	�磺 C�Ĳ���:
	(1) ���Դ�������ʼ��C,D,C,D,C,D .....D,C,B,C
	(2) ���Դ��Ϸ�������ʼ��D,C,D,C,D,C .....D,C,B,C
	(3) ���Դ��·�������ʼ��B,C,D,C,D,C .....D,C,B,C
	*/
	if (note_art->trillNoteType >= Note_Sixteen && note_art->trillNoteType <= Note_256)
	{
		//if (art_y > start_y+STAFF_OFFSET[staff-1]+LINE_H)
		//	art_y = start_y+STAFF_OFFSET[staff-1]+LINE_H;
		int tmp_y = art_y;		//-LINE_H*1;
		int x2 = 0;
		if (note_art->has_wavy_line)
		{
			x2 = x+2*LINE_H;
			auto stop_measure = measure;
			if (note_art->wavy_stop_measure > 0) {
				for (int mmm = 0; mmm <= note_art->wavy_stop_measure && mmm < music->measures.size(); mmm++) {
					stop_measure = music->measures[mmm+measure->number];
					if (mmm == 0) {
						x2 += (measure->meas_length_size-note->pos.start_offset)*OFFSET_X_UNIT+MEAS_RIGHT_MARGIN;
					} else if (mmm == note_art->wavy_stop_measure) {
						x2 += MEAS_LEFT_MARGIN;
					} else {
						x2 += stop_measure->meas_length_size*OFFSET_X_UNIT+MEAS_RIGHT_MARGIN+MEAS_LEFT_MARGIN;
					}
				}
			}
			std::shared_ptr<OveNote> stop_note = nullptr;
			if (note_art->wavy_stop_note < stop_measure->notes.size()) {
				stop_note = stop_measure->notes[note_art->wavy_stop_note];
				if (note_art->wavy_stop_note < stop_measure->notes.size()-1) {
					auto& next_note = stop_measure->notes[note_art->wavy_stop_note+1];
					if (next_note->staff != stop_note->staff)
						x2 += (stop_measure->meas_length_size-stop_note->pos.start_offset)*OFFSET_X_UNIT;
					else
						x2 += (next_note->pos.start_offset-stop_note->pos.start_offset)*OFFSET_X_UNIT;
				} else {
					x2 += (stop_measure->meas_length_size-stop_note->pos.start_offset)*OFFSET_X_UNIT;
				}
			} else {
				stop_note = stop_measure->notes.back();
			}
			x2 += stop_note->pos.start_offset*OFFSET_X_UNIT;
		}
		if (note_art->art_placement_above) {
			ART_TRILL(x-3, tmp_y);
			if (note_art->has_wavy_line)
				LINE_WAVY_HORIZONTAL(x+2*LINE_H, x2, tmp_y);
		} else {
			ART_TRILL(x-3, art_y+LINE_H*1.7+2);
			if (note_art->has_wavy_line)
				LINE_WAVY_HORIZONTAL(x+2*LINE_H, x2, tmp_y);
		}
		if (Accidental_Natural == note_art->accidental_mark) {
			FLAG_STOP(x+LINE_H, tmp_y-1.5*LINE_H, 1);
		} else if (Accidental_Sharp == note_art->accidental_mark) {
			FLAG_SHARP(x+LINE_H, tmp_y-1.5*LINE_H, 1);
		} else if (Accidental_Flat == note_art->accidental_mark) {
			FLAG_FLAT(x+LINE_H, tmp_y-1.5*LINE_H, 1);
		}
	}
}

float VmusImage::drawSvgMeasure(const std::shared_ptr<OveMeasure>& measure, float start_x, float start_y, const std::shared_ptr<OveLine>& ove_line, int line_index, int line_count)
{
	float x = start_x;
	size_t staff_count=ove_line->staves.size();// clefEveryStaff.count;
	int last_staff_lines=5;
	if (staff_count>0)
	{
		std::shared_ptr<LineStaff> line_staff = nullptr;
		if (staff_count-1 < ove_line->staves.size())
			line_staff = ove_line->staves[staff_count-1];

		if (line_staff->clef==Clef_TAB)
			last_staff_lines=6;
	}
	drawSvgRepeat(measure, start_x, start_y);

	//���
	if (measure->key.key!=measure->key.previousKey && ove_line->begin_bar != measure->number)
	{
		int key=measure->key.key;
		int previousKey = measure->key.previousKey;
		if (key >= 0 && previousKey > key) {
			previousKey -= key;
			float tmp_x=x-2*fabs(static_cast<float>(previousKey));
			drawSvgDiaohaoWithClef(Clef_Treble, previousKey, tmp_x, start_y, true);
			drawSvgDiaohaoWithClef(Clef_Bass, previousKey, tmp_x, start_y+STAFF_OFFSET[1], true);
			start_x+=0+9*fabs(static_cast<float>(previousKey));
			x=start_x;
		} else {
			if (measure->key.previousKey != 0 && (measure->key.previousKey > 0 && measure->key.key < measure->key.previousKey))
			{
				float tmp_x = x-9*abs(static_cast<float>(previousKey));
				drawSvgDiaohaoWithClef(Clef_Treble, previousKey, tmp_x, start_y, true);
				drawSvgDiaohaoWithClef(Clef_Bass, previousKey, tmp_x, start_y+STAFF_OFFSET[1], true);
				start_x += 0+9*abs(static_cast<float>(previousKey));
				x = start_x;
			}
		}

		last_fifths=key;
		drawSvgDiaohaoWithClef(Clef_Treble,key,x-2*LINE_H,start_y,false);
		drawSvgDiaohaoWithClef(Clef_Bass,key,x-2*LINE_H,start_y+STAFF_OFFSET[1],false);
		start_x+=9+9*fabs(static_cast<float>(key));
	}
	drawSvgTexts(measure, start_x, start_y);
	//there has no need to use measure->images.

    if (measure->notes.size()>0)
	{
		//����ÿ��beam��λ�� slurs
		for (int beam_index = 0; beam_index < measure->beams.size(); beam_index++)
		{
			auto& beam = measure->beams[beam_index];
			getBeamRect(beam, start_x, start_y, measure, true);
		}
		std::shared_ptr<OveLine> nextLine = nullptr;
		if (line_index < music->lines.size()-1)
			nextLine = music->lines[line_index+1];
		drawSvgSlurs(measure, start_x, start_y, ove_line);
		drawSvgTies(measure, start_x, start_y, ove_line);
		drawSvgPedals(measure, start_x, start_y, ove_line, nextLine);
		drawSvgTuplets(measure, start_x, start_y, ove_line);
		drawSvgLyrics(measure, start_x, start_y);
		drawSvgDynamics(measure, start_x, start_y);
		drawSvgExpressions(measure, start_x, start_y);
		drawSvgOctaveShift(measure, start_x, start_y, ove_line, line_index);
		drawSvgGlissandos(measure, start_x, start_y, ove_line, line_index, line_count);
		drawSvgWedges(measure, start_x, start_y, ove_line, line_index, line_count);
		drawSvgClefs(measure, start_x, start_y, ove_line);

		//note����
		for (int i = 0; i < measure->notes.size(); i++)
		{
            auto& note = measure->notes[i];
			if (note->staff>staff_count)
				continue;

            float y=start_y;		//ÿһ���������ĵ�y����
            int staff=note->staff;
			//ÿ������ķ��������x����
            x = start_x+MEAS_LEFT_MARGIN+note->pos.start_offset*OFFSET_X_UNIT;
            
            if (note->isRest) {
				drawSvgRest(note, measure, start_x, start_y);
            } else {		// !isRest
                float note_y0=0, note_y1=y;
				auto& note_elem0 = note->sorted_note_elems.back();
				note_y0 = start_y + lineToY(note_elem0->line, staff);

				if (note->sorted_note_elems.size() > 1) {
					auto& note_elem1 = note->sorted_note_elems.front();
					note_y1 = start_y + lineToY(note_elem1->line, staff);
				} else {
					note_y1 = note_y0;
				}

				float note_up_y = note_y0;
				float note_below_y = note_y1;
				if (note->note_type > Note_Whole)
				{
					if (note->stem_up)
						note_up_y = note_y0-3.5*LINE_H;
					else
						note_below_y = note_y1+3.5*LINE_H;
				}
                
				bool note_tailed_drawed = false;
                float note_x=x;
                float stem_x=note_x+0.5;
                float delta_stem_x=0;
				float acc_x = note_x;
				note->display_note_x = note_x;

				for (int elem_nn = 0; elem_nn < note->note_elems.size(); elem_nn++)
                {
                    auto& note_elem=note->note_elems[elem_nn];
                    note_x=x;
                    
                    if (note_elem->note<minNoteValue)
                        minNoteValue=note_elem->note;

                    if (note_elem->note>maxNoteValue)
                        maxNoteValue=note_elem->note;

					//y:��ͷ���ĵ������
                    staff = note->staff+note_elem->offsetStaff;
                    y = start_y + lineToY(note_elem->line,staff);

					//TAB �ױ�
                    bool isTABStaff=false;
                    if(note->staff-1<ove_line->staves.size())
					{
						auto& line_staff = ove_line->staves[staff_count-1];
                        if (line_staff->clef==Clef_TAB)
                            isTABStaff=true;
                    }
                    if (isTABStaff)
					{
                        char tmp_tabstaff[64];
                        sprintf(tmp_tabstaff, "%d",note_elem->head_type-NoteHead_Guitar_0);
                        TEXT(note_x-1, y-LINE_H*1.2, NORMAL_FONT_SIZE, tmp_tabstaff);
                        continue;
                    }
                    
					//��ͷ
					bool dontDrawHead = false;
                    if (note->note_elems.size()>1)
                    {
                        if (!note->stem_up) {
							//������ͺ�һ�������1,�Ͱ��������ת��ʾ�� 0,-5, -2, 0, 0 -1 -6, -6, -1, 2
                            if (elem_nn<note->note_elems.size()-1) {
								auto prev_elem = note->note_elems[elem_nn+1];
                                if (!prev_elem->display_revert)
								{
                                    int delta = note_elem->line-prev_elem->line;
                                    if (delta==1||delta==-1)
                                    {
                                        if (note->stem_up) {
                                            note_x+=LINE_H;
                                            if (note->note_type==Note_Whole)
                                                note_x+=0.4*LINE_H;
                                        } else {
                                            note_x-=LINE_H+0;
                                            if (note->note_type==Note_Whole)
                                                note_x-=0.4*LINE_H;
                                            acc_x=x-LINE_H;
                                        }
                                        note_elem->display_revert=true;
                                    }
                                }
                            }
                        } else {
							//�������ǰһ�������1,�Ͱ��������ת��ʾ�� 0,-5, -2, 0, 0 -1 -6, -6, -1, 2
                            if (elem_nn>0) {
								std::shared_ptr<NoteElem> prev_elem = note->note_elems[elem_nn-1];
                                if (!prev_elem->display_revert)
								{
                                    int delta = note_elem->line-prev_elem->line;
                                    if (delta==1||delta==-1) {
                                        if (note->stem_up) {
                                            note_x+=LINE_H;
                                            if (note->note_type==Note_Whole)
                                                note_x+=0.4*LINE_H;
                                        } else {
                                            note_x-=LINE_H+0;
                                            if (note->note_type==Note_Whole)
                                                note_x-=0.4*LINE_H;
                                            acc_x=x-LINE_H;
                                        }
                                        note_elem->display_revert=true;
                                    } else if (delta==0&&note_elem->note!=prev_elem->note) {
                                        if (note->stem_up) {
                                            note_x+=2*LINE_H;
                                            acc_x=x+2*LINE_H;
                                        }else{
                                            note_x-=2*LINE_H+0;
                                            acc_x=x-LINE_H;
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        std::shared_ptr<OveNote> note2 = nullptr;
                        int delta=0;
						//�����ǰһ��������λ�õ�����1��0��Ҳ���������λ��ʾ
                        if (note->voice>0)
                        {
                            note2=getNoteWithOffset(note->pos.start_offset, 0 , measure, note->staff, note->voice-1);
                            if (note2 && !note2->isRest)
                            {
								auto& firstElem2=note2->note_elems.front();
                                delta = firstElem2->line-note_elem->line;
                                if (!(delta==1||delta==-1 || delta==0) && note2->note_elems.size()>1)
                                {
                                    auto& lastElem2=note2->note_elems.back();
                                    delta = lastElem2->line-note_elem->line;
                                }

                                if ((delta==0 && ((note->note_type<=Note_Half && note2->note_type>Note_Half) || (note->note_type<=Note_Whole && note2->note_type>Note_Whole)) && note2->tupletCount==0)) {
                                    note_x+=LINE_H+1;
                                    if (note->note_type==Note_Whole)
                                        note_x+=1;
                                    stem_x=note_x+0.5;
                                    delta_stem_x+=LINE_H;
								} else if (delta == 0 && note_elem->dontPlay) {
									dontDrawHead = true;
                                } else if(delta==-1) {
                                    if (!note->inBeam)
									{
                                        note_x-=LINE_H;
                                        if (note->note_type==Note_Whole)
                                            note_x-=3;

                                        stem_x=note_x+0.5;
                                        delta_stem_x-=LINE_H;
                                        acc_x=note_x-LINE_H;
                                    }
                                } else if(delta==1) {
                                    if (note->note_elems.size()==1 && (!note->inBeam) && note2->inBeam)
                                    {
                                        note_x+=LINE_H;
                                        if (note->note_type==Note_Whole)
                                            note_x+=3;

                                        stem_x=note_x+0.5;
                                    }
									acc_x=note_x-LINE_H;
                                } /*else if(delta==2||delta==-2||delta==3||delta==-3) {
                                    if (note->stem_up == note2->stem_up) {
                                        if (!note->inBeam) {
                                            note_x+=0.5*LINE_H;
                                            stem_x+=0.5*LINE_H;
                                            delta_stem_x+=0.5*LINE_H;
                                        }
                                    }
                                }*/
                            }
                        }
						//�������һ��������λ�õ�����1��Ҳ��������ƶ���ʾ
                        note2=getNoteWithOffset(note->pos.start_offset, 0 , measure, note->staff, note->voice+1);
                        if (note2 && !note2->isRest)
                        {
							auto& firstElem2=note2->note_elems.back();
                            int delta = firstElem2->line-note_elem->line;
                            if (delta==-1) {
                                if (/*note->note_elems->count==1 &&*/ !note->inBeam) {
                                    if (note->isDot) {
                                        note_x-=LINE_H;
                                        delta_stem_x-=LINE_H;
                                    } else {
                                        note_x-=LINE_H;
                                        delta_stem_x-=LINE_H;
                                        if (note->note_type==Note_Whole)
                                            note_x-=3;
                                    }
                                    acc_x=note_x;
                                    stem_x=note_x+0.5;
                                }
                            } else if(delta==0) {
                                if ((note->note_type<=Note_Half && note2->note_type>Note_Half) || (note->note_type<=Note_Whole && note2->note_type>Note_Whole))
								{
                                    note_x-=LINE_H;
                                    delta_stem_x-=LINE_H;
                                    if (note->note_type==Note_Whole)
                                        note_x-=3;

                                    acc_x=x-LINE_H;
                                    stem_x=note_x+0.5;
                                }
                            } else if(delta==1) {
                                if (/*note->note_elems->count==1 &&*/ !note->inBeam)
                                {
                                    note_x+=LINE_H;
                                    delta_stem_x+=LINE_H;
                                    if (note->note_type==Note_Whole)
                                        note_x+=3;

                                    stem_x=note_x+0.5;
                                    acc_x=note_x-LINE_H;
                                }
                            } else if(delta==2 /*|| delta==3*/) {
                                //if (note->stem_up == note2->stem_up)
                                {
                                    if (!note->inBeam && note->note_type > Note_Whole)
									{
                                        note_x+=LINE_H;
                                        stem_x+=LINE_H;
                                        delta_stem_x+=LINE_H;
                                    }
                                }
                            } /*else if(delta==-2 || delta==-3) {
                                if (note->stem_up == note2->stem_up && note->note_type > Note_Whole)
                                {
                                    if (note2->inBeam)
									{
                                        note_x-=LINE_H;
                                        stem_x-=LINE_H;
                                        delta_stem_x-=LINE_H;
                                    }
                                }
                            }*/
                        }
                    }

                    float zoom=1;
                    if (note->isGrace)		//����
                        zoom=YIYIN_ZOOM;
                    NoteHeadType type=headType(note_elem, note->staff-1);

					if (!dontDrawHead)
					{
						if (NoteHead_Percussion == type) {
							NOTE_OpenHiHat(note_x, y, zoom);
						} else if (NoteHead_Closed_Rhythm == type) {
							NOTE_CloseHiHat(note_x+LINE_H*0.3, y, zoom);
						} else {
							char elem_id[64];
							sprintf(elem_id, "%d_%d_%d", measure->number, i, elem_nn);
							const char* elem_note = ELEM_NOTE_4;
							if (note->note_type==Note_Whole) {		//whole
								elem_note = ELEM_NOTE_FULL;
							} else if (note->note_type == Note_Half) {		//��������
								elem_note = ELEM_NOTE_2;
							} else {		//0:����, 0.125:32th, 0.25:16th, 0.5:eighth, 1:quater 1.5:
								//...
							}
							NOTE(note_x, y, zoom, elem_note, elem_id);
						}
					}
                    
                    note_elem->display_x=note_x;
                    note_elem->display_y=y;
                    
                    if (note->isDot)		//����
					{
                        float dot_x=(note->note_type==Note_Whole)?note_x+LINE_H*2:stem_x+LINE_H*1.3;
                        float dot_y=y-LINE_H*0.5;
						if (abs(static_cast<int>(note_elem->line))%2 != 0)
							dot_y += LINE_H*0.5;

						/*if (note_elem->display_revert) {
							dot_x += LINE_H;
						} else */
						if (elem_nn == note->note_elems.size()-1) {
							//��ǰһ�����������һ�������1���͵�������λ��
							auto note2 = getNoteWithOffset(note->pos.start_offset, 0, measure, note->staff, note->voice-1);
							if (note2 && !note2->isRest)
							{
								auto& bottomElem = note2->note_elems.front();
								if (1 == bottomElem->line-note_elem->line)
								{
									dot_x += LINE_H;
									dot_y = y;
									if (abs(static_cast<int>(note_elem->line))%2 != 1)
										dot_y = y+0.5*LINE_H;
								}
							}
						} else if (note->note_elems.size() > 1 && elem_nn < note->note_elems.size()-1) {
							//������͸�һ�������1��2���͵�������λ��
							auto& hi_elem = note->note_elems[elem_nn+1];
							if (1 == hi_elem->line-note_elem->line)
							{
								//dot_x += LINE_H;
								dot_y = y;	//+LINE_H;
								if (1 != abs(note_elem->line)%2)
									dot_y = y+0.5*LINE_H;
							}
						}

                        for (int dot=0; dot<note->isDot; dot++)
                            NOTE_DOT(dot_x+LINE_H*dot, dot_y);
                    }

					//accidental ��������
                    if (note_elem->accidental_type>Accidental_Normal)
					{
						float temp_acc_x = acc_x;
						//���������Ǻ�
						if (elem_nn == note->note_elems.size()-1)
						{
							//��ǰһ�����������һ�������1���͵��������Ǻ�
							auto note2 = getNoteWithOffset(note->pos.start_offset, 0, measure, note->staff, note->voice-1);
							if (note2 && !note2->note_elems.empty())
							{
								auto& bottomeElem = note2->note_elems.front();
								if (bottomeElem->accidental_type > Accidental_Normal) {
									if (1 == bottomeElem->line-note_elem->line)
										temp_acc_x -= 2*LINE_H;
								}
							}
						}
						//������͸�һ�������1��2���͵��������Ǻ�
						if (note->note_elems.size() > 1 && elem_nn < note->note_elems.size()-1)
						{
							auto& hi_elem = note->note_elems[elem_nn+1];
							if (hi_elem->accidental_type > Accidental_Normal)
							{
								int delta = hi_elem->line-note_elem->line;
								if (2 == delta || 1 == delta)
									temp_acc_x -= LINE_H;
							}
						}
                        if(!drawSvgAccidental(note_elem->accidental_type, temp_acc_x, y, note->isGrace))
                            NSLog("Error unknow accidental_type=%d in measure=%d", note_elem->accidental_type, measure->number);
					}

					//�������߲��ֻ�����
					float more_line_y1 = 0;
                    int more_line_num=0;
                    if (elem_nn == note->note_elems.size()-1 && note_elem->line > 5) {		//�ڱ�������
						more_line_num = (note_elem->line-6)/2+1;
						more_line_y1 = start_y+STAFF_OFFSET[staff-1]-more_line_num*LINE_H;
                    } else if (elem_nn == 0 && note_elem->line < -5) {		//�ڱ��е�����
						more_line_num = (-note_elem->line-6)/2+1;
						more_line_y1 = start_y+STAFF_OFFSET[staff-1]+LINE_H*5;
                    }

                    if (more_line_y1>0) {
                        for (int j=0; j<more_line_num; j++) {
							if (Note_Whole == note->note_type) {
								LINE(x-0.4*LINE_H, more_line_y1, x+2.0*LINE_H, more_line_y1);
							} else if (note->stem_up) {
                                LINE(x-0.4*LINE_H, more_line_y1, x+1.8*LINE_H, more_line_y1);
							} else {
                                LINE(x-0.5*LINE_H, more_line_y1, x+1.8*LINE_H, more_line_y1);
							}
                            more_line_y1+= LINE_H;
                        }
                    }
                }

				//������beam
                if (note->inBeam)
                {
                    bool stem_drawed=false;
                    
					for (auto it = measure->beams.begin(); it != measure->beams.end(); it++) {
						auto& beam = *it;
						auto& elem0 = beam->beam_elems.front();
						if (isNote(note, beam))
                        {
                            float zoom=1.0;
                            if (note->isGrace)
                                zoom=YIYIN_ZOOM;

                            beam_current_pos=getBeamRect(beam, start_x, start_y , measure, false);
                            if (beam_current_pos.size.width<=0)
                                continue;

                            if (elem0->stop_measure_pos>0)
                            {
                                int tmp_index=(note->staff-1)*2+note->voice;
                                if (tmp_index>3)
								{
                                    NSLog("Error: staff(%d) or voice(%d) too big for beam-> measure %d",note->staff,note->voice, measure->number);
                                    tmp_index=3;
                                }
                                beam_continue_pos[tmp_index]=beam_current_pos;
                            }
                            
							//�����ǰnote��beam��ʼ�ĵ�һ��λ��,�ͻ�����
                            if (elem0->start_measure_offset == note->pos.start_offset)
                            {
								for (int j = 0; j < beam->beam_elems.size(); j++) {
									auto& elem = beam->beam_elems[j];
                                    float y1,y2;
                                    float x1 = start_x+MEAS_LEFT_MARGIN+elem->start_measure_offset*OFFSET_X_UNIT+delta_stem_x;
                                    float x2 = start_x+MEAS_LEFT_MARGIN+elem->stop_measure_offset*OFFSET_X_UNIT+delta_stem_x;
                                    if (note->isGrace)
									{
                                        x1+=GRACE_X_OFFSET;
                                        x2+=GRACE_X_OFFSET;
                                    }
                                    if (elem->stop_measure_pos>0)
                                        x2+=measure->meas_length_size*OFFSET_X_UNIT+MEAS_RIGHT_MARGIN+MEAS_LEFT_MARGIN;
									auto note2 = beam->beam_stop_note;
									if (note2)
										note2=getNoteWithOffset(elem->stop_measure_offset, elem->stop_measure_pos , measure, beam->stop_staff, beam->voice);
									
									if (note->stem_up) {
                                        x1+=LINE_H*zoom+0.5;
                                    } else if (x1>stem_x && note2->stem_up) {
                                        x1+=LINE_H*zoom;
                                    }
                                    if (note2 && note2->stem_up)
									{
                                        x2+=LINE_H*zoom;
										if (LINE_H > 8)
											x2 += 0.5;
                                    }
                                    //if (x1==x2)
                                    {
                                        if (elem->beam_type==Beam_Forward) {
                                            x2=x1+LINE_H*1.0;
                                        }else if (elem->beam_type==Beam_Backward){//Beam_Backward
                                            x1=x2-LINE_H*1.0;
                                        }
                                    }
                                    //if (j>=0)
                                    {
										y2=beam_current_pos.size.height/beam_current_pos.size.width*(x2-beam_current_pos.origin.x)*1+beam_current_pos.origin.y;
                                        if (elem->level>1) {
                                            float delta_y=sqrtf(beam_current_pos.size.height*beam_current_pos.size.height+beam_current_pos.size.width*beam_current_pos.size.width)/beam_current_pos.size.width*BEAM_DISTANCE*zoom;
                                            if (note2 && note2->stem_up)
                                                y2+=(elem->level-1)*delta_y;
                                            else
                                                y2-=(elem->level-1)*delta_y;
                                            y1=y2-beam_current_pos.size.height/beam_current_pos.size.width*(x2-x1);
                                        } else {
                                            y1=beam_current_pos.origin.y;
                                        }
                                    }
                                    LINE_W(x1+0, y1, x2+1.0, y2, BEAM_WIDTH*zoom);
                                }
								//tuplet n����
                                char tmp_tuplet[16]={0};
                                if (beam->tupletCount>0) {
                                    sprintf(tmp_tuplet, "%d",beam->tupletCount);
                                } else if (note->tupletCount>0) {
									//...
                                }

                                if (tmp_tuplet[0])
                                {
                                    int tmp_x= beam_current_pos.origin.x+beam_current_pos.size.width*0.5;
                                    int tmp_y= beam_current_pos.origin.y+beam_current_pos.size.height*0.5;
                                    if (note->stem_up)
                                        tmp_y-=LINE_H*2.5;
                                    else
                                        tmp_y+=0;
                                    TEXT(tmp_x, tmp_y, NORMAL_FONT_SIZE, tmp_tuplet);
                                }
                            }
							//��stem
                            int stem_y=y;
							if (note->note_elems.size() > 1)
							{
								if (note->stem_up && y < note_y1)
									stem_y = note_y1;
								else if (!note->stem_up && y > note_y0)
									stem_y = note_y0;
							}
							MyRect rect = beam_current_pos;
							if (beam->staff != beam->stop_staff && beam->beam_elems.size() > 1 && beam->beam_start_note->stem_up != beam->beam_stop_note->stem_up) {
								if (note->staff == 2 && beam->staff == 2)
									rect.origin.y -= (BEAM_DISTANCE+BEAM_WIDTH/2)*zoom*(beam->beam_elems.size()-1);
							}
                            drawSvgStem(rect, note, stem_x, stem_y);
                            stem_drawed=true;
                            break;
                        }
                    }
					//�����ǰnote���ڵ�ǰС������beam����,�ͼ���ǲ�����ǰһ���ӳ���beam��?
                    if (!stem_drawed)
                    {
                        int tmp_index=(note->staff-1)*2+note->voice;
                        if (tmp_index>3)
						{
                            NSLog("Error: staff(%d) or voice(%d) too big for beam-> measure %d",note->staff,note->voice, measure->number);
                            tmp_index=3;
                        }
                        MyRect tmp_beam_pos=beam_continue_pos[tmp_index];
                        
                        if (x>=tmp_beam_pos.origin.x && x<=tmp_beam_pos.origin.x+tmp_beam_pos.size.width+MEAS_LEFT_MARGIN) {
                            drawSvgStem(tmp_beam_pos, note ,x, (note->stem_up && note_y0<note_y1)?note_y0:note_y1);
                        } else if(x>=0 && x-(STAFF_HEADER_WIDTH+MARGIN_LEFT+MEAS_LEFT_MARGIN)+screen_width-MARGIN_RIGHT<=tmp_beam_pos.origin.x+tmp_beam_pos.size.width) {
                            tmp_beam_pos.origin.x-=(screen_width-MARGIN_LEFT-MARGIN_RIGHT-STAFF_HEADER_WIDTH);
                            tmp_beam_pos.origin.y+=STAFF_OFFSET[staff_count-1]+(7+GROUP_STAFF_NEXT)*LINE_H;
                            if (line_index==0)
                                tmp_beam_pos.origin.y-=screen_height-start_y;
							float zoom = 1;
							if (note->isGrace)
								zoom = YIYIN_ZOOM;
                            LINE_W(MARGIN_LEFT+STAFF_HEADER_WIDTH, tmp_beam_pos.origin.y, tmp_beam_pos.origin.x+tmp_beam_pos.size.width, tmp_beam_pos.origin.y+tmp_beam_pos.size.height, BEAM_WIDTH*zoom);
                            drawSvgStem(tmp_beam_pos, note, x, (note->stem_up && note_y0<note_y1)?note_y0:note_y1);
                        }
                    }
                }

				//����: Beam�����stem
                if (!note_tailed_drawed && !note->hideStem && note->note_type!=Note_Whole && !note->inBeam) {
					auto& elem = note->note_elems.front();
                    
                    if (note->isGrace) {		//����
						//if (note->note_type >= Note_Eight) {
						//	LINE(x+LINE_H*YIYIN_ZOOM+0.5, (y>note_y0)?y:note_y0, x+LINE_H*YIYIN_ZOOM+0.5, y-LINE_H*2.0*YIYIN_ZOOM);
						//	LINE(x+LINE_H-LINE_H*0.9, y-3, x+LINE_H+LINE_H*0.3, y-LINE_H*3.0*0.5-1);
						//} else {
						if (note->stem_up)
							LINE(x+LINE_H*YIYIN_ZOOM+0.5, note_y1, x+LINE_H*YIYIN_ZOOM+0.5, note_y0-LINE_H*3.0*YIYIN_ZOOM);
						else
							LINE(x, note_y0, x, note_y1+LINE_H*3.0*YIYIN_ZOOM);
						//}
                    } else if (note->stem_up) {
                        stem_x+=LINE_H;
						if (LINE_H > 8)
							stem_x += 0.5;

                        float tmp_y=start_y+lineToY(elem->line,note->staff+elem->offsetStaff);
                        if (note->note_elems.size()>1)
                        {
                            if (tmp_y<y)
							{
                                float tt=tmp_y;
                                tmp_y=y;
                                y=tt;
                            }
                        }
						if (y-LINE_H*3.5 > start_y+3*LINE_H+STAFF_OFFSET[note->staff-1]) {
							LINE(stem_x, tmp_y, stem_x, start_y+2*LINE_H+STAFF_OFFSET[note->staff-1]);
							y = start_y+5.5*LINE_H+STAFF_OFFSET[note->staff-1];
						} else {
							LINE(stem_x, tmp_y, stem_x, y-LINE_H*3.5);
						}
                    } else {
                        float tmp_y=start_y+lineToY(elem->line,note->staff+elem->offsetStaff);
                        if (note->note_elems.size()>1)
                        {
                            if (tmp_y>y)
							{
                                float tt=tmp_y;
                                tmp_y=y;
                                y=tt;
                            }
                        }
						//����Ҫ�ӳ������ߵĵ�2��
						if (y+LINE_H*3.5 < start_y+2*LINE_H+STAFF_OFFSET[note->staff-1]) {
							LINE(stem_x, tmp_y, stem_x, start_y+2*LINE_H+STAFF_OFFSET[note->staff-1]);
							y = start_y-1.5*LINE_H+STAFF_OFFSET[note->staff-1];
						} else {
							LINE(stem_x, tmp_y, stem_x, y+LINE_H*3.5);
						}
                    }

                    //tail
                    if (note->isGrace) {		//����
                        if (note->note_type==Note_Sixteen) {
                            TAIL_16_UP_ZOOM(x-0.5+GRACE_X_OFFSET, y+LINE_H*0.5*YIYIN_ZOOM, YIYIN_ZOOM);
                        } else if (Note_Eight == note->note_type) {
                            TAIL_EIGHT_UP(x-0.5+GRACE_X_OFFSET, y+LINE_H*0.5*YIYIN_ZOOM, YIYIN_ZOOM);
                        }
                    } else if(note->note_type==Note_Eight) {		//eighth
                        if (note->stem_up) {
                            TAIL_EIGHT_UP(stem_x-LINE_H-1, y,1);
						} else {
                            TAIL_EIGHT_DOWN(stem_x, y-1);
						}
                    } else if(note->note_type==Note_Sixteen) {		//16th
                        if (note->stem_up) {
                            TAIL_16_UP(stem_x-LINE_H-1, y);
						} else {
                            TAIL_16_DOWN(stem_x, y);
						}
                    } else if(note->note_type==Note_32) {		//32th
                        if (note->stem_up) {
                            TAIL_32_UP(stem_x-LINE_H-1, y);
						} else {
                            TAIL_32_DOWN(stem_x, y);
						}
                    } else if(note->note_type==Note_64) {		//64th
                        if (note->stem_up) {
                            TAIL_64_UP(stem_x-LINE_H-1, y);
						} else {
                            TAIL_64_DOWN(stem_x, y);
						}
                    } else if(note->note_type> Note_64) {
                        NSLog("Error: unknow beam=%d, n=%d",note->inBeam, note->note_type);
                    }
                }
				//draw art
				if (!note->note_arts.empty())
				{
					int pedal_downs = 0, pedal_ups = 0;
					float art_up_count = 0, art_down_count = 0;
					float zoom = 1;
					if (note->isGrace)
						zoom = 0.7;

					for (int nn = 0; nn < note->note_arts.size(); nn++)
					{
						auto& note_art = note->note_arts[nn];
						ArticulationType art_type = note_art->art_type;
						int art_placement_above = note_art->art_placement_above;

						float art_y = y, art_x = note_x;
						if (note->note_elems.size() > 1)
						{
							if (art_placement_above)
								art_y = note_y0;
							else
								art_y = note_y1;
						}
						art_x += note_art->offset.offset_x*OFFSET_X_UNIT;
						if (note_art->offset.offset_y != 0) {
							if (art_placement_above) {
								art_y -= note_art->offset.offset_y*OFFSET_Y_UNIT;	//+LINE_H;
								if (note->stem_up)
									art_y -= 2*LINE_H;
							} else {
								art_y += note_art->offset.offset_y*OFFSET_Y_UNIT;
							}
						} else {
							if (note->note_type <= Note_Whole) {
								if (art_placement_above)
									art_y -= 1.5*LINE_H;
								else
									art_y += 1.5*LINE_H;
							} else if (art_placement_above && (note->stem_up || note->isGrace)) {
								if (note->inBeam) {
									//art_y = beam_current_pos.origin.y-LINE_H*1.0;
									art_y = beam_current_pos.size.height/beam_current_pos.size.width*(art_x-beam_current_pos.origin.x)*1+beam_current_pos.origin.y-LINE_H*1.5;
								} else {
									art_y -= (note->isGrace) ? 1.5*LINE_H : 3*LINE_H;
									if (Articulation_Fermata == art_type || Articulation_Fermata_Inverted == art_type) {
										art_y -= 2.0*LINE_H;
									} else if (Articulation_Finger == art_type && note_art->finger.size() > 1) {
										art_y -= 1.0*LINE_H;
									} else if (note->note_type >= Note_Eight) {
										art_y -= LINE_H;
									}	
								}
								if (note_art->accidental_mark > Accidental_Normal)
									art_y -= 1.5*LINE_H;
							} else if ((!art_placement_above) && (!note->stem_up)) {
								if (note->inBeam) {
									//art_y = beam_current_pos.origin.y+LINE_H*1.5;
									art_y = beam_current_pos.size.height/beam_current_pos.size.width*(art_x-beam_current_pos.origin.x)*1+beam_current_pos.origin.y+LINE_H*1.5;
								} else {
									art_y += (note->isGrace) ? 1.5*LINE_H : 3.0*LINE_H;
									art_x += 0.2*LINE_H;
									if (Articulation_Finger == art_type && note_art->finger.size() > 1) {
										art_y += 2.0*LINE_H;
									} else if (note->note_type >= Note_Eight) {
										art_y += 1.0*LINE_H;
									}
								}
							} else {
								if (art_placement_above) {
									if (note->stem_up)
										art_y -= 3.5*LINE_H;
									else
										art_y = note_up_y-1.5*LINE_H;
									if (note_art->accidental_mark > Accidental_Normal)
										art_y -= 1.5*LINE_H;
								} else {
									if (note->stem_up)
										art_y = note_below_y+1.5*LINE_H;
									else
										art_y = note_below_y+LINE_H;
								}
							}
							float adds = 1.4;
							if (Articulation_Staccato == art_type || Articulation_Tenuto == art_type) {
								adds = 0.4;
							} else if (Articulation_Fermata == art_type || Articulation_Fermata_Inverted == art_type || Articulation_Turn == art_type || Articulation_Major_Trill == art_type || Articulation_Minor_Trill == art_type) {
								adds = 2.0;
							}
							if (art_placement_above) {
								art_y -= LINE_H*art_up_count;
								art_up_count += adds;
							} else {
								art_y += LINE_H*art_down_count;
								art_down_count += adds;
							}
						}

						int staff_start_y = start_y+STAFF_OFFSET[staff-1];
						if (Articulation_Pedal_Down == art_type || Articulation_Pedal_Up == art_type)
						{
							staff_start_y = start_y;
							art_x += 4.0*LINE_H*pedal_downs+2.0*LINE_H*pedal_ups;
							if (Articulation_Pedal_Down == art_type)
								pedal_downs++;
							else if (Articulation_Pedal_Up == art_type)
								pedal_ups++;
							if (Articulation_Pedal_Up == art_type && note->note_arts.size() > 1) {
								for (int other = 0; other < note->note_arts.size(); ++other) {
									auto nextArt = note->note_arts[nn+1];
									if (Articulation_Pedal_Down == nextArt->art_type) {
										art_x -= 2*LINE_H;
										break;
									}
								}
							}
						}

						if (!drawSvgArt(note_art, (art_placement_above == 1) ? true : false, art_x, art_y, staff_start_y)) {
							if (Articulation_Arpeggio == art_type) {
								int tmp_x = note_x+note_art->offset.offset_x*OFFSET_X_UNIT;
								if (note_art->offset.offset_x == 0)
									tmp_x -= LINE_H;
								if (Note_Whole == note->note_type)
									tmp_x -= 0.5*LINE_H;
								for (auto elem = note->note_elems.begin(); elem != note->note_elems.end(); elem++) {
									if (Accidental_Normal != (*elem)->accidental_type) {
										tmp_x -= 1.0*LINE_H;
										break;
									}
								}
								auto& elem1 = note->sorted_note_elems.front();
								auto& elem2 = note->sorted_note_elems.back();

								int tmp_y1 = start_y+lineToY(elem1->line, note->staff+elem1->offsetStaff);
								int tmp_y2 = start_y+lineToY(elem2->line, note->staff+elem2->offsetStaff);
								if (2 == note_art->arpeggiate_over_staff) {
									char tmp_buf[16] = {0};
									sprintf(tmp_buf, "%d", note->pos.tick);
									auto& notes = measure->sorted_notes[tmp_buf];
									auto& endNote = notes.back();
									if (note_art->arpeggiate_over_voice <= 1)		//over one voice
									{
										if (4 == notes.size())
											endNote = notes[2];
									}
									elem2 = endNote->note_elems.front();
									tmp_y2 = start_y+lineToY(elem2->line, note_art->arpeggiate_over_staff);
								} else {
									tmp_y2 = start_y+lineToY(elem2->line, note->staff+elem2->offsetStaff);
								}
								LINE_WAVY_VERTICAL(tmp_x, tmp_y1, tmp_y2, WAVY_LINE_WIDTH);
								//Tremolo ����
							} else if (Articulation_Tremolo_Eighth == art_type
								|| Articulation_Tremolo_Sixteenth == art_type
								|| Articulation_Tremolo_Thirty_Second == art_type
								|| Articulation_Tremolo_Sixty_Fourth == art_type)
							{
#if 1
								if (0 == note_art->tremolo_stop_note_count) {
									float tmp_x = x-LINE_H*0.5;
									float tmp_y = y+LINE_H*3;
									if (note->stem_up)
									{
										tmp_y = y-1*LINE_H;
										tmp_x += LINE_H*1;
									}
									char tremolo_str[16];
									sprintf(tremolo_str, "a%x", 8+art_type-Articulation_Tremolo_Eighth);
									GLYPH_Petrucci(tmp_x, tmp_y, GLYPH_FONT_SIZE, 0, tremolo_str);
								} else if (!note_art->tremolo_beem_mode && i < measure->notes.size()-1) {
									std::shared_ptr<OveNote>& nextNote = measure->notes[i+1];
									float tmp_x = x+LINE_H*0.5;
									float tmp_y = y+LINE_H*3.5;
									if (note->stem_up)
									{
										tmp_y = y-3*LINE_H;
										tmp_x += LINE_H*1.5;
									}
									float next_x = start_x+MEAS_LEFT_MARGIN+nextNote->pos.start_offset*OFFSET_X_UNIT;
									float next_y;
									if (nextNote->line > note->line)
										next_y = tmp_y-0.5*LINE_H;
									else
										next_y = tmp_y+0.5*LINE_H;
									for (int i = 0; i < art_type-Articulation_Tremolo_Eighth+1; ++i)
									{
										LINE_W(tmp_x, tmp_y, next_x, next_y, TREMOLO_LINE_WIDTH);
										tmp_y += 2*TREMOLO_LINE_WIDTH;
										next_y += 2*TREMOLO_LINE_WIDTH;
									}
								}
#else
								float tmp_x = x;
								float tmp_y = y+LINE_H*1.5;
								if (note->stem_up)
									tmp_y = y-3*LINE_H;
								for (int i = 0; i < art_type-Articulation_Tremolo_Eighth+1; i++)
								{
									LINE_W(tmp_x-LINE_H*0.5, tmp_y+2, tmp_x+LINE_H, tmp_y-2, WAVY_LINE_WIDTH);
									tmp_y += 4;
								}
#endif
							} else if (Articulation_Finger == art_type) {
								if (note_art->finger.size() == 1) {
									char buf[16];
									sprintf(buf, "3%x", atoi(note_art->finger.c_str()));
									GLYPH_Petrucci(art_x, art_y, GLYPH_FONT_SIZE*0.6*zoom, 0, buf);
								} else {
									//GLYPH_Petrucci(art_x, art_y, GLYPH_FONT_SIZE*0.8*zoom, 0, note_art->finger.c_str());
									TEXT(art_x, art_y-LINE_H, GLYPH_FONT_SIZE*0.8*zoom, note_art->finger.c_str());
								}
								if (note_art->alterFinger != "")
								{
									char buf[16];
									sprintf(buf, "3%x", atoi(note_art->alterFinger.c_str()));
									if (note_art->art_placement_above) {
										art_y -= 1.5*LINE_H;
										LINE_W(art_x-2, art_y+6, art_x+LINE_H, art_y+6, 1);
									} else {
										art_y += 1.5*LINE_H;
										LINE_W(art_x-2, art_y-6, art_x+LINE_H, art_y-6, 1);
									}
									GLYPH_Petrucci(art_x, art_y, GLYPH_FONT_SIZE*0.6*zoom, 0, buf);
								}
							} else {
								NSLog("Error unknow art_type=0x%x in measure=%d",art_type, measure->number);
							}
						}
						drawSvgTrill(note_art, measure, note, x, art_y);
					}
				}		//end draw art
            }
        }		//measure end

		if (!measure->numerics.empty()) {
			for (auto num = measure->numerics.begin(); num != measure->numerics.end(); num++) {
				if ((*num)->numeric_text != "") {
					int tmp_x1 = start_x+2;
					int tmp_y = start_y-LINE_H*2.0;
					int tmp_x2 = start_x;
					if ((*num)->offset_y)
						tmp_y = start_y-(*num)->offset_y*OFFSET_Y_UNIT+LINE_H;

					std::shared_ptr<OveMeasure> next_measure = nullptr;
					for (int n = 0; n < (*num)->numeric_measure_count && measure->number+n < music->measures.size(); n++)
					{
						next_measure = music->measures[measure->number+n];
						tmp_x2 += next_measure->meas_length_size*OFFSET_X_UNIT+(MEAS_LEFT_MARGIN+MEAS_RIGHT_MARGIN);
					}
					tmp_x2 -= 5;

					LINE(tmp_x1, tmp_y+LINE_H*1, tmp_x1, tmp_y);
					LINE(tmp_x1, tmp_y, tmp_x2, tmp_y);
					bool closeEnding = (Barline_Default != next_measure->right_barline);
					if (!closeEnding && next_measure->number < music->measures.size()-1)
					{
						auto& nextNextMeasure = music->measures[next_measure->number+1];
						closeEnding = (Barline_Default != nextNextMeasure->left_barline);
					}
					if (closeEnding)
						LINE(tmp_x2, tmp_y+LINE_H*1, tmp_x2, tmp_y);
					TEXT(tmp_x1, tmp_y, NORMAL_FONT_SIZE, (*num)->numeric_text.c_str());
					if (tmp_x2 >= screen_width-MARGIN_RIGHT)
					{
						tmp_x1 = MARGIN_LEFT+STAFF_HEADER_WIDTH;
						tmp_x2 = tmp_x2-(screen_width-MARGIN_RIGHT)+tmp_x1;
						tmp_y += STAFF_OFFSET[staff_count-1]+(4+GROUP_STAFF_NEXT)*LINE_H;
						LINE(tmp_x1, tmp_y, tmp_x2, tmp_y);
						LINE(tmp_x2, tmp_y+LINE_H*1, tmp_x2, tmp_y);
					}
				}
			}
		}
    }
	
	x = start_x+MEAS_LEFT_MARGIN+measure->meas_length_size*OFFSET_X_UNIT + MEAS_RIGHT_MARGIN;
	if (x>screen_width-MARGIN_RIGHT)
		x=screen_width-MARGIN_RIGHT;

	//С�ڱ߽�
	if (measure->right_barline==Barline_Double) {		//˫ϸ��
		LINE(x, start_y, x, start_y+LINE_H*(last_staff_lines-1)+STAFF_OFFSET[staff_count-1]);
		LINE(x-1.5*BARLINE_WIDTH, start_y, x-1.5*BARLINE_WIDTH, start_y+LINE_H*(last_staff_lines-1)+STAFF_OFFSET[staff_count-1]);
	} else if (measure->right_barline==Barline_RepeatRight || measure->right_barline==Barline_Final) {		//�����Ǻ�
		if (measure->right_barline==Barline_RepeatRight) {
			for (int i=0; i<staff_count; i++) {
				NORMAL_DOT(x-LINE_H*1.5, start_y+LINE_H*1.5+STAFF_OFFSET[i]);
				NORMAL_DOT(x-LINE_H*1.5, start_y+LINE_H*2.5+STAFF_OFFSET[i]);
			}
		}

		LINE(x-2*BARLINE_WIDTH, start_y, x-2*BARLINE_WIDTH, start_y+LINE_H*(last_staff_lines-1)+STAFF_OFFSET[staff_count-1]);
		LINE_W(x-0.5*BARLINE_WIDTH, start_y, x-0.5*BARLINE_WIDTH, start_y+LINE_H*(last_staff_lines-1)+STAFF_OFFSET[staff_count-1], BARLINE_WIDTH);
	} else if(measure->right_barline==Barline_Default) {		//��ϸ��
		LINE(x, start_y, x, start_y+LINE_H*(last_staff_lines-1)+STAFF_OFFSET[staff_count-1]);
	} else if(measure->right_barline!=Barline_Null) {
		NSLog("Error: unknow right_barline=%d at measure(%d)",measure->right_barline,measure->number);
	}
    return x;
}

static char tmpBuffer[1024*64];

void VmusImage::beginSvgImage(CGSize size, int startMeasure /* = 0 */)
{
    svgXmlContent = new MyString();
	svgMeasurePosContent = new MyString();
	svgForceCurveContent = new MyString();
    svgXmlJianpuContent=new MyString();
    svgXmlJianpuFixDoContent=new MyString();
    svgXmlJianwpContent = new MyString();
    svgXmlJianwpFixDoContent = new MyString();

#if 0
    char densitydpi_str[64]="";
    if(densitydpi>0)
        sprintf(densitydpi_str, ",target-densitydpi=%d", densitydpi);
#endif

	//<use xlink:href="Aloisen New.svg"/>
#ifdef NEW_PAGE_MODE
#else
	if (size.height/size.width < real_screen_size.height/real_screen_size.width)
		size.height = size.width*real_screen_size.height/real_screen_size.width;
#endif
	size.height = round(size.height);
	staff_size = size;
	page_size.width = size.width;
	page_size.height = size.width*music->xml_page_height/music->page_width;
	sprintf(tmpBuffer,"<?xml version=\"1.0\" standalone=\"yes\"?>\n"
		"<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n"
		"<html><head>\n"
		"<meta name='viewport' content=\"width=1024px, maximum-scale=2.0, minimum-scale=0.1, user-scalable=1\"/>\n"
		"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n"
		//"<meta name='viewport' content=\"width=1024, height=device-height,  maximum-scale=2.0, minimum-scale=0.1, user-scalable=1\"/>\n"
		"</head><body "bgColor">\n"
		"<svg id='svg' width='%dpx' height='%dpx' version='1.1' xmlns='http://www.w3.org/2000/svg'>\n"
		"<g id=\"myCanvas\"></g>\n",
		(int)screen_width, (int)screen_height);//, font_url];
    svgXmlContent->appendString(tmpBuffer);

	//����
    if (showJianpu)
	{
		this->svgXmlJianpuContent->appendString("<g id='jianpu' style='visibility:hidden'>\n");
		this->svgXmlJianpuFixDoContent->appendString("<g id='jianpufixdo' style='visibility:hidden'>\n");
		this->svgXmlJianwpContent->appendString("<g id='jianwp' style='visibility:hidden'>\n");
		this->svgXmlJianwpFixDoContent->appendString("<g id='jianwpfixdo' style='visibility:hidden'>\n");
	}
    
	sprintf(tmpBuffer, "<script>\n document.documentElement.style.webkitTouchCallout='none'; document.documentElement.style.webkitUserSelect='none'; var meas_start=%d; var meas_pos=[\n", startMeasure);
	svgMeasurePosContent->appendString(tmpBuffer);

	const char* pSvgFileName = ".\\Music Font.svg";
	int nFileSize = GetFileSize(pSvgFileName);
	char* pSvgBuffer = new char[nFileSize+1];
	memset(pSvgBuffer, 0, nFileSize+1);
	FILE* pSvgFile = fopen(pSvgFileName, "rb");
	fread(pSvgBuffer, 1, nFileSize, pSvgFile);
	fclose(pSvgFile);
	svgXmlContent->appendString(pSvgBuffer);
	delete []pSvgBuffer;

#ifdef NEW_PAGE_MODE
	CGSize screen_size;
	screen_size.width = screen_width;
	screen_size.height = screen_height;
	drawPageBackground(screen_size);
#else
	svgXmlContent->appendString("<g id=\"tempgroup\"></g>");
#endif
    for (int i=0; i<this->music->trackes.size()*5; i++)
	{
        sprintf(tmpBuffer, "<rect id='cursor%d'></rect>", i);
        svgXmlContent->appendString(tmpBuffer);
    }
	svgXmlContent->appendString("<rect id='progressBar'></rect>");
    svgXmlContent->appendString("<rect id='playloopA'></rect>");
    svgXmlContent->appendString("<rect id='playloopB'></rect>");
}

MyString *VmusImage::endSvgImage()
{
	if (showJianpu)
	{
		//����
		svgXmlJianpuContent->appendString("</g>\n");
		svgXmlContent->appendString(this->svgXmlJianpuContent);

		//�̶�������
		svgXmlJianpuFixDoContent->appendString("</g>\n");
		svgXmlContent->appendString(this->svgXmlJianpuFixDoContent);

		//������
		svgXmlJianwpContent->appendString("</g>\n");
		svgXmlContent->appendString(this->svgXmlJianwpContent);

		//�̶���������
		svgXmlJianwpFixDoContent->appendString("</g>\n");
		svgXmlContent->appendString(this->svgXmlJianwpFixDoContent);
	}
    
	this->svgXmlContent->appendString("</svg>");
	//FEFBEB		ǳ��ɫ
	//fefefe		ǳ��ɫ
	this->svgXmlContent->appendString(multichar2utf8("\
									  <div id='addcomment' style='border:none;width:280px;position:absolute;visibility:hidden;background:none'>\n\
									  <button id='commentA' style='width:130px;height:30px;background:#FEFBEB;color:#27AE60;font-size:medium'>���ֵ���</button>\n\
									  </div>\n\
									  \n\
									  <div id='popview' style='border:none; width:280px;position:absolute;visibility:hidden;background:none'>\n\
									  <button id='loopA' style='width:80px;height:30px;background:#FEFBEB;color:#27ae60;font-size:medium'>��ʼ||:</button>\n\
									  <button id='loopB' style='width:80px;height:30px;background:#FEFBEB;color:#c0392b;font-size:medium'>:||����</button>\n\
									  <button id='loopCancel' onclick='cancelLoopAB()' style='width:70px;height:30px;background:#FEFBEB;font-size:medium'>ȡ��</button>\n\
									  </div>\n").c_str());
	svgXmlContent->appendString(this->svgMeasurePosContent);
	svgXmlContent->appendString(svgForceCurveContent);
    svgXmlContent->appendString("</script>\n<script src='script.js'></script>\n");
	svgXmlContent->appendString("</body></html>\n");
    return svgXmlContent;
}

void VmusImage::beginSvgPage(const CGSize& size, int page)
{
	if (landPageMode) {
		sprintf(tmpBuffer, "<g transform =\"translate(%.1f,0)\">\n", size.width*page);
		svgXmlContent->appendString(tmpBuffer);
	} else {
		sprintf(tmpBuffer, "<g transform =\"translate(0,%.1f)\">\n", size.height*page);
		svgXmlContent->appendString(tmpBuffer);
	}
}

void VmusImage::drawPageBackground(const CGSize& size)
{
#define BACK_LEFT_MARGIN			20.0
#define BACK_RIGHT_MARGIN			20.0
#define BACK_TOP_MARGIN				8.0
#define BACK_BOTTOM_MARGIN	8.0

	for (int page = 0; page < music->pages.size(); ++page)
	{
		float x,y;
		if (landPageMode) {
			x = size.width*page;
			y = 0;
		} else {
			x = 0;
			y = size.height*page;
		}
		sprintf(tmpBuffer, "<rect x='%.1f' y='%.1f' width='%.1f' height='%.1f' fill='%s' stroke='%s' />\n", x+BACK_LEFT_MARGIN, y+BACK_TOP_MARGIN, size.width-BACK_LEFT_MARGIN-BACK_RIGHT_MARGIN, size.height-BACK_TOP_MARGIN-BACK_BOTTOM_MARGIN, paperColor, "#aaa");
		svgXmlContent->appendString(tmpBuffer);

		LINE_C(x+BACK_LEFT_MARGIN, y+size.height-BACK_BOTTOM_MARGIN+1, x+size.width-BACK_RIGHT_MARGIN+1, y+size.height-BACK_BOTTOM_MARGIN+1, "#000", 2);
		LINE_C(x+size.width-BACK_RIGHT_MARGIN+1, y+BACK_TOP_MARGIN, x+size.width-BACK_RIGHT_MARGIN, y+size.height-BACK_BOTTOM_MARGIN+1, "#000", 2);

		sprintf(tmpBuffer, "%d.", page+1);
		TEXT(x+BACK_LEFT_MARGIN+10, y+size.height-30, 16, tmpBuffer);
	}
}

void VmusImage::beginSvgLine(int line_num, float x, float y)
{
	sprintf(tmpBuffer, "<g id='line_%d' transform =\"translate(0, %.1f)\">\n", line_num, y);
	svgXmlContent->appendString(tmpBuffer);
}

void VmusImage::endSvgLine()
{
	svgXmlContent->appendString("</g>\n");
}

void VmusImage::endSvgPage()
{
	svgXmlContent->appendString("</g>\n");
}

void VmusImage::drawJianpu(OveMeasure *measure, int jianpu_start_x, int start_y)
{
    if (measure->sorted_duration_offset.empty())
        return;

	enum { MAX_STAFF = 20 };

    int jianpu_steps[]={
        0,-4,-1,-5,-2,1,-3,
        0,
		-4,-1,-5,-2,1,-3,0};

	for (auto it = measure->sorted_duration_offset.begin(); it != measure->sorted_duration_offset.end(); it++)
	{
		auto& notes = measure->sorted_notes[*it];
		std::sort(notes.begin(), notes.end(), [](const std::shared_ptr<OveNote>& obj1, const std::shared_ptr<OveNote>& obj2)->bool{ return obj1->note_type < obj2->note_type; });
        int offset_y[MAX_STAFF]={0,0,0,0,0, 0,0,0,0,0};
        
        for (int note_nn=0;note_nn<notes.size();note_nn++)
        {
			std::shared_ptr<OveNote>& note = notes[note_nn];
            float note_x = jianpu_start_x+MEAS_LEFT_MARGIN+note->pos.start_offset*OFFSET_X_UNIT;
            for (int elem_nn=0; elem_nn<note->note_elems.size() || elem_nn<1; elem_nn++)
			{
                if (note->staff<1 || note->staff>STAFF_COUNT)
                    continue;

                int font_size=JIANPU_FONT_SIZE+1-(int)note->note_elems.size()/2;
                int jianpu_y=start_y-3*LINE_H+STAFF_OFFSET[note->staff-1]+offset_y[note->staff-1];
                if (note->isRest) {
                    int rest_number=1;
                    if (note->note_type==Note_Half) {
                        rest_number=2;
                        if (note->isDot)
                            rest_number=3;
                    } else if (note->note_type==Note_Whole) {
                        rest_number=4;
                    }

                    int tmp_x=note_x-JIANPU_FONT_SIZE/12;
                    for (int count=0; count<rest_number; count++)
					{
                        TEXT_JIANPU(tmp_x, jianpu_y-JIANPU_FONT_SIZE/6, "0", "0", JIANPU_FONT_SIZE);
                        tmp_x+=measure->meas_length_size*OFFSET_X_UNIT/4;
                    }
                } else {
					if (note->sorted_note_elems.empty())
						note->sorted_note_elems = note->note_elems;

					std::shared_ptr<NoteElem> note_elem;
					if (note->sorted_note_elems.size()-elem_nn-1 < note->sorted_note_elems.size())
						note_elem = note->sorted_note_elems[note->sorted_note_elems.size()-elem_nn-1];

                    if (last_fifths>=-7 && last_fifths<=7)
                    {
                        //  C  | bD  |  D  | bE  |  bF |  F  | bG  |  G  | bA  |  A  | bB  | bC
                        //  C  | C#  |  D  | D#  |  E  |  F  | F#  |  G  | G#  |  A  | A#  | B
                        //  1           2           3     4           5           6          7
                        int jiappu_step_index=(note_elem->xml_pitch_step+6+jianpu_steps[last_fifths+7]);
                        int step=jiappu_step_index%7+1;//1-7
                        int octave=note_elem->xml_pitch_octave+jiappu_step_index/7-1; //0-9
                        
                        if (step>0 && step<=7)
                        {
							char num[16], numFixDo[16];
							sprintf(num, "%d", step);
							sprintf(numFixDo, "%d", note_elem->xml_pitch_step);
                            if (note->isGrace)
                                font_size=JIANPU_FONT_SIZE*0.7;
                            if (elem_nn>0)
                                font_size-=5;
                            TEXT_JIANPU(note_x-font_size/12, jianpu_y-font_size/6, num, numFixDo, font_size);

                            //Jian Wupu
                            float jianwp_y=start_y + lineToY(note_elem->line, note->staff+note_elem->offsetStaff);
                            float jianwp_x=note_x;
                            int jianwp_font_size=1.2*LINE_H;
                            if (note->note_type>Note_Half) {
                                if (note->isGrace)
								{
                                    jianwp_font_size*=0.6;
                                    jianwp_x-=LINE_H*0.2;
                                    jianwp_y-=LINE_H*0.2;
                                }
                                TEXT_JIANWP(jianwp_x, jianwp_y, step, note_elem->xml_pitch_step, jianwp_font_size, "#FFFFFF");
                            } else {
                                if (note->note_type==Note_Whole)
                                    jianwp_x=note_x+LINE_H*0.2;

                                BACK_JIANWP(jianwp_x, jianwp_y, 0.46*LINE_H, "#FFFFFF");
                                TEXT_JIANWP(jianwp_x, jianwp_y, step, note_elem->xml_pitch_step, jianwp_font_size, "#000000");
                            }
                            
                            //sharp/flat
                            if (note_elem->accidental_type==Accidental_Sharp) {
                                SHARP_JIANPU(note_x-0.4*LINE_H, jianpu_y+0.2*font_size);
                            } else if (note_elem->accidental_type==Accidental_Flat) {
                                FLAT_JIANPU(note_x-0.4*LINE_H, jianpu_y+0.2*font_size);
                            } else if (note_elem->xml_pitch_alter==0) {
                                int pitch_step=note_elem->xml_pitch_step;
                                if ((last_fifths==1 && pitch_step==4) || //G major: G A B C D E #F
                                    (last_fifths==2 && (pitch_step==1||pitch_step==4)) ||   //D major: D E #F G A B #C
                                    (last_fifths==3 && (pitch_step==1||pitch_step==4||pitch_step==5)) ||   //A major: A B #C D E #F #G
                                    (last_fifths==4 && (pitch_step!=3&&pitch_step!=6&&pitch_step!=7)) ||   //E major: E #F #G A B #C #D
                                    (last_fifths==5 && (pitch_step!=3&&pitch_step!=7)) ||   //B major: B #C #D E #F #G #A
                                    (last_fifths==6 && (pitch_step!=7)) ||   //F# major: #F #G #A B #C #D #E
                                    (last_fifths==7)    //C# major: #C #D #E #F #G #A #B
                                    ) {
                                    FLAT_JIANPU(note_x-0.4*LINE_H, jianpu_y+0.2*font_size);
                                }else if ((last_fifths==-7) || //Cb major: bC bD bE bF bG bA bB
                                          (last_fifths==-6 && pitch_step!=4) || //Gb major: bG bA bB bC bD bE F
                                          (last_fifths==-5 && pitch_step!=4 && pitch_step!=1) ||     //Db major: bD bE F bG bA bB C
                                          (last_fifths==-4 && pitch_step!=4 && pitch_step!=1 && pitch_step!=5) ||    //Ab major: bA bB C bD bE F G
                                          (last_fifths==-3 && (pitch_step==3 || pitch_step==6 || pitch_step==7)) ||    //Eb major: bE F G bA bB C D
                                          (last_fifths==-2 && (pitch_step==3 || pitch_step==7)) ||    //Bb major: bB C D eE F G A
                                          (last_fifths==-1 && (pitch_step==7))     //F major: F G A bB C D E
                                          ) {
                                    SHARP_JIANPU(note_x-0.4*LINE_H, jianpu_y+0.2*font_size);
                                }
                            }

                            //above dot
                            if (octave>4)
							{
                                for (int dots=5; dots<=octave; dots++)
								{
                                    if (note->isGrace) {
                                        DOT_JIANPU(note_x+1, jianpu_y+1.5*font_size-6*dots);
									} else {
                                        DOT_JIANPU(note_x+1.5, jianpu_y+2+JIANPU_FONT_SIZE-6*dots);
									}
                                }
                            }
                            
                            //below dot
                            if (octave<4)
							{
                                int dot_y=jianpu_y;
                                if (elem_nn==0 && note->note_type>Note_Quarter && note->note_type<Note_None)
                                    dot_y+=(note->note_type-Note_Quarter)*4;

                                for (int dots=3; dots>=octave; dots--)
								{
                                    DOT_JIANPU(note_x+1.5, dot_y+font_size+17-5*dots);
                                    offset_y[note->staff-1]-=2;
                                }
                            }
                        }
                    }
                }

                //doted
                if (note->note_type>=Note_Quarter && note->isDot>0)
                    for (int count=1; count<=note->isDot; count++)
                        DOT_JIANPU(note_x+count*0.4*font_size, jianpu_y+0.5*font_size);

                //beam
                if (elem_nn==0)
				{
                    int beam_y=jianpu_y+font_size;
                    if (note->note_type>Note_Quarter) {		// && note->note_type<Note_None)
                        int beams=note->note_type-Note_Quarter;
                        if (note->note_type>=Note_None)
                            if (note->inBeam)
                                beams=1;

                        for (int count=0; count<beams; count++)
                            LINE_JIANPU(note_x-0.1*font_size, note_x+0.4*font_size, beam_y+count*4,1);
                    } else if (note->note_type==Note_Half && !note->isRest) {
                        int tmp_x=note_x;
                        tmp_x+=measure->meas_length_size*OFFSET_X_UNIT/4;
                        LINE_JIANPU(tmp_x, tmp_x+0.4*font_size, beam_y-0.5*font_size,2);
                        if (note->isDot)
						{
                            tmp_x+=measure->meas_length_size*OFFSET_X_UNIT/4;
                            LINE_JIANPU(tmp_x, tmp_x+0.4*font_size, beam_y-0.5*font_size,2);
                            if (note->isDot>1)
                                DOT_JIANPU(tmp_x + 0.8*font_size, jianpu_y+0.5*font_size);
                        }
                    } else if (note->note_type==Note_Whole && !note->isRest) {
                        int tmp_x=note_x;
                        for (int count=0; count<3; count++)
						{
                            tmp_x+=measure->meas_length_size*OFFSET_X_UNIT/4;
                            LINE_JIANPU(tmp_x, tmp_x+0.4*font_size, beam_y-0.5*font_size,2);
                        }
                    }
					if (!note->isGrace)
						offset_y[note->staff-1]+=font_size*0.1;
                }
				if (!note->isGrace)
					offset_y[note->staff-1]-=font_size+0;
            }
			if (!note->isGrace)
				offset_y[note->staff-1]-=JIANPU_FONT_SIZE*0.5;
        }
    }
}

void VmusImage::drawSvgTempo(OveMeasure* first_measure, float start_x, float start_y)
{
	if (!first_measure->tempos.empty()) {
		if (!start_denominator)
		{
			start_numerator=first_measure->numerator;
			start_denominator=first_measure->denominator;
		}
	
		std::shared_ptr<Tempo>& tempo = first_measure->tempos.front();
		float tempo_x=start_x;	//+3*LINE_H;
		float tempo_y;
		if (tempo->offset_y) {
			tempo_y = start_y+tempo->offset_y*OFFSET_Y_UNIT;
		} else {
			tempo_y = start_y-LINE_H*2;
		}
		if (tempo->tempo_left_text != "")
		{
			//tempo_y = start_y+tempo->offset_y*OFFSET_Y_UNIT;
			CGSize tempo_size = GetFontPixelSize(tempo->tempo_left_text, tempo->font_size);
			TEXT_ATTR_FONTFAMILY(tempo_x, tempo_y-tempo_size.height, tempo->font_size, tempo->tempo_left_text.c_str(), true, false, "Courier New");
			tempo_x += tempo_size.width+1.5*LINE_H;
			tempo_y -= LINE_H;
		}

		unsigned char left_note_type=tempo->left_note_type&0x0F;
		start_tempo_num=tempo->tempo+tempo->tempo_range/2;
		start_tempo_type=1.0/4.0;

		switch (left_note_type)
		{
		case 1:		//ȫ����
			NOTE_FULL(tempo_x, tempo_y, 1);
			start_tempo_type=1;
			break;
		case 2:		//��������
			NOTE_2_UP(tempo_x, tempo_y, 1);
			start_tempo_type=1.0/2;
			break;
		case 3:		//�ķ�����
			NOTE_4_UP(tempo_x, tempo_y, 1);
			start_tempo_type=1.0/4;
			break;
		case 4:		//�˷�����
			NOTE_8_UP(tempo_x, tempo_y, 1);
			start_tempo_type=1.0/8;
			break;
		case 5:		//16������
			NOTE_16_UP(tempo_x, tempo_y, 1);
			start_tempo_type=1.0/16;
			break;
		case 6:		//32������
			NOTE_32_UP(tempo_x, tempo_y, 1);
			start_tempo_type=1.0/32;
			break;
		case 7:		//64������
			NOTE_64_UP(tempo_x, tempo_y, 1);
			start_tempo_type=1.0/64;
			break;
		default:
			NSLog("Error: unknow note_type=%d\n", left_note_type);
			break;
		}

		char sufix[64];
		char tmp_tempo[128];
		if ((tempo->left_note_type&0x30)==0x20) {
			strcpy(tmp_tempo, ". =");
			start_tempo_type*=1.5;
		} else {
			strcpy(tmp_tempo,"=");
		}

		if (tempo->tempo_range==0) {
			sprintf(sufix,"%d", tempo->tempo);
		} else {
			sprintf(sufix,"(%d-%d)", tempo->tempo, tempo->tempo+tempo->tempo_range);
		}
		strcat(tmp_tempo, sufix);
		TEXT(tempo_x+LINE_H*1.5, tempo_y-LINE_H*2.0, 2.5*LINE_H, tmp_tempo);
	}else{
		//http://zh.wikipedia.org/wiki/%E9%80%9F%E5%BA%A6_(%E9%9F%B3%E6%A8%82)
		std::map<std::string, int> times;
		times["Larghissimo"]						= 35;		// �� ���˵ػ�����40 bpm �����£�
		times["Lentissimo"]							= 38;		// �� �Ȼ������
		times["Grave"]									= 40;		//׳��     ->����    ���صġ������
		times["Largo"]									= 44;		//���     ->����    ��壨�ִ�������
		times["Lento"]									= 52;		//����     ->����    ���壨40 - 60 bpm��
		times["Adagio"]								= 56;		//���    ->����    ��� �� ���壨66 - 76 bpm��
		times["Larghetto"]							= 60;		//С���    ->����    �����壨60 - 66 bpm��
		times["Cantabile"]							= 60;		//���
		times["Adagietto"]							= 65;		//����
		times["Andante"]								= 66;		//�а�      ->�����ٶȻ��˵������ٶ� �а壨76 - 108 bpm��
		times["Andantino"]							= 69;		//С�а�  ->�����ٶȻ��˵������ٶ�
		times["Moderato"]							= 88;		//�а�     ->���ٻ��˵������ٶ�
		times["Moderato Scherzando"]	= 88;		//�а�     ->���ٻ��˵������ٶ� �а壨90 - 115 bpm��
		times["Allegro moderato"]			= 98;		//��ӹ�Ŀ��  �ʶȡ����ļ���
		times["Allegretto"]							= 108;		//С���  ->�Կ�
		times["Allegro"]								= 120;		//���      ->����  ��壨120 - 168 bpm��
		times["Allegro con allegrezza"]	= 120;		// ���ֵĿ��
		times["Allegro assai"]						= 130;
		times["Allegro vivace"]					= 134;		//���õĿ����
		times["Vivace"]									= 140;		//�ٰ�       ->�ܿ�  ���ã�~140 bpm��
		times["Vivo"]									= 152;		//�ٰ�         ->�ܿ�
		times["Presto"]									= 170;		//����       ->���� ��168 - 200 bpm��
		times["Allegrissimo"]						= 191;
		times["Vivacissimo"]						= 198;
		times["Prestissimo"]						= 204;		//���  ->��� (ԼΪ200 - 208 bpm��
		bool found_expression = false;
		for (auto exp = first_measure->expressions.begin(); exp != first_measure->expressions.end(); exp++)
		{
			std::string str = (*exp)->exp_text;
			trim(str);
			if (times.find(str) != times.end())
			{
				start_tempo_num = times[str];
				start_tempo_type = 1.0/4;
				found_expression = true;
				break;
			}
		}
		if (!found_expression) {
			for (auto text = first_measure->meas_texts.begin(); text != first_measure->meas_texts.end(); text++) {
				std::string str = (*text)->text;
				trim(str);
				if (times.find(str) != times.end())
				{
					start_tempo_num = times[str];
					start_tempo_type = 1.0/4;
					break;
				}
			}
		}
	}
}

void VmusImage::drawSvgFiveLine(OveLine* line, float start_x, float start_y)
{
	for (int nn = 0; nn < STAFF_COUNT; nn++)
	{
		std::shared_ptr<LineStaff>& tmp_staff = line->staves[nn];
		if (tmp_staff->hide)
			continue;

		//�׺�
		if (Clef_Treble == tmp_staff->clef) {
			CLEF_TREBLE(start_x+LINE_H*0.5, start_y+STAFF_OFFSET[nn]+3*LINE_H, 1);
		} else if (Clef_Bass == tmp_staff->clef) {
			CLEF_BASS(start_x+LINE_H*0.5, start_y+STAFF_OFFSET[nn]+LINE_H*1, 1);
		} else if (Clef_TAB == tmp_staff->clef) {
			TEXT(start_x+5, start_y+STAFF_OFFSET[nn]-0.5*LINE_H, 2*LINE_H, "T");
			TEXT(start_x+5, start_y+STAFF_OFFSET[nn]+1.0*LINE_H, 2*LINE_H, "A");
			TEXT(start_x+5, start_y+STAFF_OFFSET[nn]+2.7*LINE_H, 2*LINE_H, "B");
		} else if (Clef_Percussion1 == tmp_staff->clef) {
			CLEF_Percussion1(start_x+LINE_H*1.5, start_y+STAFF_OFFSET[nn]+LINE_H*2, 1);
		} else {
			NSLog("Error unknown clef=%d at staff=%d", tmp_staff->clef, nn);
		}

		//���ţ�������
		STAFF_HEADER_WIDTH = 5*LINE_H+0.9*LINE_H*fabs(static_cast<float>(line->fifths));
		last_fifths = line->fifths;
		drawSvgDiaohaoWithClef(tmp_staff->clef, line->fifths, start_x+0, start_y+STAFF_OFFSET[nn], false);
		//������
		int staff_line_count = 5;
		if (Clef_TAB == tmp_staff->clef)
			staff_line_count = 6;

		for (int staff_line = 0; staff_line < staff_line_count; staff_line++)
			LINE(start_x, start_y+STAFF_OFFSET[nn]+LINE_H*staff_line, screen_width-MARGIN_RIGHT, start_y+STAFF_OFFSET[nn]+LINE_H*staff_line);
	}
}

void VmusImage::drawSvgMusic()
{
	if (music->pages.size() == 0)
		return;
	
	std::shared_ptr<OvePage>& page = music->pages.front();
	if (!page)
		return;

	if (!measure_pos->empty())
		measure_pos->clear();

    this->minNoteValue=127;
    this->maxNoteValue=0;

    for (int i=0; i<SLUR_CONTINUE_NUM; i++)
        slur_continue_info[i].validate=false;

    OFFSET_Y_UNIT = page_height/music->page_height/**0.7*/;
    MARGIN_TOP = music->page_top_margin*OFFSET_Y_UNIT;
    
    if (this->music->version==4) {
        GROUP_STAFF_NEXT=page->staff_distance*OFFSET_Y_UNIT;
    } else {
        GROUP_STAFF_NEXT=page->staff_distance*OFFSET_Y_UNIT;
    }
    
    unsigned char last_denominator=0;
    unsigned char last_numerator=0;
    staff_images=new MyArray();

	CGSize size;
#ifdef NEW_PAGE_MODE
	if (landPageMode) {
		music->page_left_margin = 200;
		music->page_right_margin = 200;
		size.width = screen_width*music->pages.size();
		size.height = screen_height;
		beginSvgImage(size);
	} else {
		size.width = screen_width;
		size.height = screen_height*music->pages.size();
		beginSvgImage(size);
	}
#else
    if (!pageMode)
	{
		size.width = screen_width;
		size.height = screen_height*music->pages.size();
		beginSvgImage(size);
	}
#endif

	size.width = screen_width;
	size.height = screen_height;
	for (int page_num = 0; page_num < music->pages.size(); page_num++)
	{
		std::shared_ptr<OvePage>& page = music->pages[page_num];

#ifdef NEW_PAGE_MODE
		beginSvgPage(size, page_num);
#else
        if (pageMode)
		{
			std::shared_ptr<OveLine> line = nullptr;
			if (page->begin_line < music->lines.size())
				line = music->lines[page->begin_line];
			beginSvgImage(size, line->begin_bar);
        }
#endif
		
		//��ʾ����
		//if (staff_images->count==0)
		if (0 == page_num)
			drawSvgTitle();

		//����ҳ��������
		if (this->showJianpu)		//���׸��ǵı߿�
			RECT_JIANPU(MARGIN_LEFT, MARGIN_TOP, screen_width-MARGIN_RIGHT-MARGIN_LEFT, screen_height-MARGIN_TOP);

        for (int line_num=0; line_num<page->line_count; line_num++)
		{
			std::shared_ptr<OveLine> line = nullptr;
			if (line_num+page->begin_line < music->lines.size())
				line = music->lines[line_num+page->begin_line];

            float start_x=MARGIN_LEFT;
            float start_y=line->y_offset*OFFSET_Y_UNIT+MARGIN_TOP;

#ifdef NEW_PAGE_MODE
#else
			if (!pageMode)
				start_y+=page_num*screen_height;
#endif

			float line_y = start_y;
            if (line->begin_bar>=music->measures.size())
                break;

			std::shared_ptr<OveMeasure> first_measure;
			if (line->begin_bar < music->measures.size())
				first_measure = music->measures[line->begin_bar];
            STAFF_COUNT=line->staves.size();

			//�����߱���
            float max_track_name_width=0;
            if (STAFF_COUNT>0)
			{
				//����track name����󳤶�
				for (int nn=0; nn<STAFF_COUNT && nn<music->trackes.size(); nn++)
				{
					std::shared_ptr<OveTrack>& track = music->trackes[nn];
					if (track->track_name != "")
					{
						const char *name=track->track_brief_name.c_str();
						if (line_num==0 && page_num==0 && track->track_name[0])
						{
							track->track_name = "Piano";
							name=track->track_name.c_str();
						}
						
						float track_name_width=NORMAL_FONT_SIZE*strlen(name)/2;
						if (track_name_width>max_track_name_width)
							max_track_name_width=track_name_width;
					}
				}
				start_x=max_track_name_width+MARGIN_LEFT;

                for (int nn=0; nn<STAFF_COUNT; nn++)
                {
					std::shared_ptr<LineStaff>& tmp_staff = line->staves[nn];
					STAFF_OFFSET[nn]=tmp_staff->y_offset*OFFSET_Y_UNIT;
					if (nn>0)
						STAFF_OFFSET[nn]+=STAFF_OFFSET[nn-1];
				}
				drawSvgFiveLine(line.get(), start_x, start_y);

				//ÿһ�п�ͷ�ļ���֮����
				if (first_measure->denominator != last_denominator || first_measure->numerator != last_numerator)
                {
                    STAFF_HEADER_WIDTH += 3*LINE_H;
                    drawSvgTimeSignature(first_measure.get(),start_x+STAFF_HEADER_WIDTH-MEAS_LEFT_MARGIN,start_y,STAFF_COUNT);
                    last_denominator=first_measure->denominator;
                    last_numerator=first_measure->numerator;
                }

				if (!start_denominator && first_measure->denominator > 0)
				{
					start_numerator = first_measure->numerator;
					start_denominator = first_measure->denominator;
				}

				//������
                for (int i=0; i<line->staves.size(); i++)
				{
					std::shared_ptr<LineStaff>& tmp_staff = line->staves[i];
                    if (tmp_staff->hide)
                        continue;

                    int last_staff_lines=5;
                    if (tmp_staff->clef==Clef_TAB)
                        last_staff_lines=6;

                    int track_name_y=start_y+STAFF_OFFSET[i]+1*LINE_H;
                    if (tmp_staff->group_staff_count>0)
					{
                        int end_staff_index=i+tmp_staff->group_staff_count;
                        if (end_staff_index>STAFF_COUNT)
                            end_staff_index=STAFF_COUNT;

                        float group_start=STAFF_OFFSET[i];
                        float group_end=LINE_H*(last_staff_lines-1)+STAFF_OFFSET[end_staff_index]+2;
                        int group_size=(group_end-group_start)*3; //0.3;
                        //GROUP(start_x-LINE_H, start_y+group_end, group_size);
						if (line->staves.size() > 2) {
							GROUP(start_x-2*LINE_H, start_y+group_end, group_size);
						} else {
							GROUP(start_x-LINE_H, start_y+group_end, group_size);
						}
                        track_name_y+=(group_end-group_start)*0.5-NORMAL_FONT_SIZE;
                    }

                    //track name
                    if (i<music->trackes.size())
					{
						std::shared_ptr<OveTrack>& track = music->trackes[i];
                        if (line_num==0 && page->begin_line==0) {
                            if (track->track_name != "")
							{
                                float track_name_width= strlen(track->track_name.c_str())*NORMAL_FONT_SIZE/2;
                                TEXT(start_x-track_name_width-MEAS_LEFT_MARGIN, track_name_y, NORMAL_FONT_SIZE, track->track_name.c_str());
                            }
                        } else {
                            if (track->track_brief_name != "")
                                TEXT(MARGIN_LEFT, track_name_y, NORMAL_FONT_SIZE, track->track_brief_name.c_str());
                        }
                    }
                }

				//��ͷ��barline
                if (STAFF_COUNT>0)
                {
					int last_staff_lines=5;
					std::shared_ptr<LineStaff> last_staff;
					if (STAFF_COUNT-1 < line->staves.size())
						last_staff = line->staves[STAFF_COUNT-1];

                    if (last_staff->clef==Clef_TAB)
                        last_staff_lines=6;
                    LINE(start_x, start_y, start_x, start_y+STAFF_OFFSET[STAFF_COUNT-1]+(last_staff_lines-1)*LINE_H);
                }

				//С�ڱ��
				if (first_measure->show_number > 0)
				{
					char tmp_bianhao[32];
					sprintf(tmp_bianhao, "%d",first_measure->show_number);
					TEXT(start_x, start_y-LINE_H*4, NORMAL_FONT_SIZE, tmp_bianhao);
				}
            }

            if (line_num<page->line_count-1)
			{
				std::shared_ptr<OveLine> next_line;
				if (line_num+page->begin_line+1 < music->lines.size())
					next_line = music->lines[line_num+page->begin_line+1];

                GROUP_STAFF_NEXT=((next_line->y_offset-line->y_offset)*OFFSET_Y_UNIT-STAFF_OFFSET[STAFF_COUNT-1])/LINE_H-4;
            }
			
			int total_tickes=0;
			int diaohao_change=0;
			//����OFFSET_X_UNIT
			for (int nn=0; nn<line->bar_count && nn+line->begin_bar<music->measures.size(); nn++)
			{
				std::shared_ptr<OveMeasure>& tmp_measure = music->measures[line->begin_bar+nn];
				total_tickes+= tmp_measure->meas_length_size;
				
				if ((tmp_measure->key.key || tmp_measure->key.previousKey) && nn > 0)
					diaohao_change+=LINE_H*0.9*(1+fabs(static_cast<float>(tmp_measure->key.key)));
				
				if (nn>0 && (tmp_measure->numerator!=last_numerator || tmp_measure->denominator!=last_denominator))
					diaohao_change+=LINE_H;
				
				last_numerator=tmp_measure->numerator;
				last_denominator=tmp_measure->denominator;
			}
			
			float len=(screen_width-(start_x+STAFF_HEADER_WIDTH+MARGIN_RIGHT+(MEAS_LEFT_MARGIN+MEAS_RIGHT_MARGIN)*(line->bar_count)))-diaohao_change;
			OFFSET_X_UNIT = len / total_tickes;

			//����������С��
			int base_y = 0;
            float x=start_x+STAFF_HEADER_WIDTH;
			std::map<int, std::vector<float> > staff_xorder;		//1: upstaff, 2: lowstaff
			staff_xorder[1] = staff_xorder[2] = std::vector<float>();
			std::map<int, std::pair<int, int> > meas_bound;		//key:measure number, pair:measure bound
			std::pair<int, int> tmpPair;
            for (int nn=0; nn<line->bar_count && line->begin_bar+nn<music->measures.size(); nn++)
            {
				std::shared_ptr<OveMeasure>& measure = music->measures[line->begin_bar+nn];
                int key_offset=0;
				if (measure->key.key || measure->key.previousKey)
				{
					if (measure->key.key)
						key_offset = LINE_H*0.9*(1+abs(static_cast<float>(measure->key.key)));
					else
						key_offset = LINE_H*0.9*(1+abs(static_cast<float>(measure->key.previousKey)));
				}
				
				//��һҳ��һ�е��ٶ�tempos
				if (measure->tempos.size() > 0)
					drawSvgTempo(measure.get(), x, start_y);

				//����ÿһС�ڵ�λ�ã�Ϊ�˲���midiʱ������ʾ��ǰС��
				MeasurePos meas_pos;
				meas_pos.page = staff_images->count;
				meas_pos.start_x = x;
				meas_pos.start_y = line_y-LINE_H*2;
				meas_pos.height = STAFF_OFFSET[STAFF_COUNT-1]+4*LINE_H+4*LINE_H;
				this->svgMeasurePosContent->appendString("{'notes':[");

				auto& sorted_duration_offset = measure->sorted_duration_offset;
				for (auto key = sorted_duration_offset.begin(); key != sorted_duration_offset.end(); key++)
				{
					auto& notes = measure->sorted_notes[*key];
					std::shared_ptr<NotePos> note_pos = std::make_shared<NotePos>();
					meas_pos.note_pos.push_back(note_pos);
					note_pos->page = meas_pos.page;

					float note_x = x+MEAS_LEFT_MARGIN+key_offset+notes.front()->pos.start_offset*OFFSET_X_UNIT;
					float y1[MAX_POS_NUM];
					y1[0] = start_y-LINE_H*2;
					for (int k = 1; k < MAX_POS_NUM; k++)
						y1[k] = start_y+STAFF_OFFSET[k]-2*LINE_H;

					int staff_bit_mask = 0;
					for (auto note = notes.begin(); note != notes.end(); note++)
					{
						if (!(*note)->isRest)
							staff_bit_mask |= (*note)->staff;
						//ÿ������ķ��������x����
						std::shared_ptr<NoteElem> note_elem = nullptr;
						if (!(*note)->note_elems.empty())
							note_elem = (*note)->note_elems.front();
						int staff = 0;
						if (note_elem)
							staff = (*note)->staff+note_elem->offsetStaff;
						else
							staff = (*note)->staff;
						float note_y = start_y+lineToY((*note)->line, staff)-2*LINE_H;

						if ((*note)->staff <= MAX_POS_NUM)
							if (note_y < y1[(*note)->staff-1])
								y1[(*note)->staff-1] = note_y;
					}
					
					for (int k = 0; k < MAX_POS_NUM; k++)
						note_pos->start_y[k] = y1[k];

					if (staff_bit_mask & 0x1)
						staff_xorder[1].push_back(note_x);
					if (staff_bit_mask & 0x2)
						staff_xorder[2].push_back(note_x);
					note_pos->start_x = note_x;
					note_pos->width = LINE_H*1.5;
					note_pos->height = 8*LINE_H;
				}

				//begin measure: �����Ǻ�
				int last_staff_lines = 5;
				std::shared_ptr<LineStaff> last_staff;
				if (STAFF_COUNT-1 < line->staves.size())
					last_staff = line->staves[STAFF_COUNT-1];

				if (Clef_TAB == last_staff->clef)
					last_staff_lines = 6;
				int staff_count = line->staves.size();
				if (Barline_RepeatLeft == measure->left_barline) {
					for (int i = 0; i < staff_count; i++)
					{
						NORMAL_DOT(x+LINE_H*0.8, start_y+LINE_H*1.5+STAFF_OFFSET[i]);
						NORMAL_DOT(x+LINE_H*0.8, start_y+LINE_H*2.5+STAFF_OFFSET[i]);
					}
					LINE(x+1.5*BARLINE_WIDTH, start_y, x+1.5*BARLINE_WIDTH, start_y+LINE_H*(last_staff_lines-1)+STAFF_OFFSET[staff_count-1]);
					LINE_W(x-0, start_y, x-0, start_y+LINE_H*(last_staff_lines-1)+STAFF_OFFSET[staff_count-1], BARLINE_WIDTH);
				} else if (Barline_Double == measure->left_barline) {
					LINE(x-BARLINE_WIDTH, start_y, x-BARLINE_WIDTH, start_y+LINE_H*(last_staff_lines-1)+STAFF_OFFSET[staff_count-1]);
				} else if (Barline_Default != measure->left_barline) {
					printf("Error: unknown left_barline=%d at measure(%d)\n", measure->left_barline, measure->number);
				}

				//�������֮�����б仯
                if (nn>0 && (measure->numerator!=last_numerator || measure->denominator!=last_denominator))
                {
                    x+=LINE_H*0.5;
                    drawSvgTimeSignature(measure.get(),x,start_y,STAFF_COUNT);
                    x+=LINE_H*0.5;
                }
                last_numerator=measure->numerator;
                last_denominator=measure->denominator;

				//��С��
                int jianpu_start_x=x;
				tmpPair.first = x;
                x=drawSvgMeasure(measure,x,start_y,line,line_num,page->line_count);
				tmpPair.second = x;
				meas_bound[line->begin_bar+nn] = tmpPair;
				int w = MEAS_LEFT_MARGIN+measure->meas_length_size*OFFSET_X_UNIT+MEAS_RIGHT_MARGIN;
				BEGIN_MEASURE(measure->number, meas_pos.start_x, meas_pos.start_y, w, meas_pos.height);
				meas_pos.start_x += MEAS_LEFT_MARGIN;
				meas_pos.width = x-meas_pos.start_x-MEAS_RIGHT_MARGIN;
				measure_pos->push_back(meas_pos);

				for (auto& key = sorted_duration_offset.begin(); key != sorted_duration_offset.end(); ++key)
				{
					auto& notes = measure->sorted_notes[*key];
					float x = 0;
					for (auto& note = notes.begin(); note != notes.end(); ++note)
					{
						if ((*note)->isRest) {
							x = (*note)->display_note_x;
						} else {
							x = ((*note)->note_elems.empty()) ? 0 : (*note)->note_elems.front()->display_x;
							break;
						}
					}
					char tmp_buffer[16];
					sprintf(tmp_buffer, "%.0f,", x);
					svgMeasurePosContent->appendString(tmp_buffer);
				}

				sprintf(tmpBuffer, "], 'pos':{'x':%d, 'y':%d, 'w':%d, 'h':%d},'page':%d},\n", meas_pos.start_x, meas_pos.start_y, meas_pos.width, meas_pos.height, page_num);
				this->svgMeasurePosContent->appendString(tmpBuffer);
#ifdef NEW_PAGE_MODE
				if (landPageMode)
					meas_pos.start_x += music->page_width*page_num;
				else
					meas_pos.start_y += music->page_height*page_num;
#endif
				base_y = meas_pos.start_y;
				
				if (showJianpu)		//jianpu
					drawJianpu(measure.get(), jianpu_start_x, start_y);
			}
#if 0
			if (MidiEvents)
				drawForceCurve(line_num, staff_xorder, base_y, meas_bound);
#endif
        }
		
#ifdef NEW_PAGE_MODE
		endSvgPage();
#else
		if (pageMode)
		{
			svgMeasurePosContent->appendString("];var page_pos=null;");
			staff_images->addObject(endSvgImage());
		}
#endif
    }

#ifdef NEW_PAGE_MODE
	svgMeasurePosContent->appendString("];var page_pos=[");
	if (landPageMode) {
		for (int page = 0; page < music->pages.size(); ++page) {
			sprintf(tmpBuffer, "{'x':%.0f,'y':0},", size.width*page);
			svgMeasurePosContent->appendString(tmpBuffer);
		}
	} else {
		for (int page = 0; page < music->pages.size(); ++page) {
			sprintf(tmpBuffer, "{'x':0,'y':%.0f},", size.height*page);
			svgMeasurePosContent->appendString(tmpBuffer);
		}
	}
	svgMeasurePosContent->appendString("];");
	staff_images->addObject(endSvgImage());
#else
	if (!pageMode)
		staff_images->addObject(endSvgImage());
#endif
}

#define	TICK_INCR	480
void VmusImage::drawForceCurve(int line_num, std::map<int, std::vector<float> >& staff_xorder, int base_y, std::map<int, std::pair<int, int> >& meas_bound)
{
	enum { degree = 6, point_num = 10 };
	auto start_event = MidiEvents->begin();
	while (start_event != MidiEvents->end() && start_event->oveline != line_num)
		start_event++;
	if (start_event == MidiEvents->end())
		return;

	for (int staff = 1; staff <= 2; staff++) 		//only up and down two staves in one line
	{
		auto& xorder = staff_xorder[staff];
		std::vector<float> vFactor;
		std::vector<Position> vForce, vOrgForce;
		Position point, org_point;

		int index = 0, org_index = 0, fromM = -1, max_elem_vv = -1, staff_base_y = base_y, total_num = 0, start_tick = -1;
		for (auto event = start_event; event != MidiEvents->end(); event++) {
			if (line_num == event->oveline && staff == event->note_staff && 0x90 == static_cast<int>(event->evt & 0xF0) && event->vv > 0) {
				if (fromM == -1)
					fromM = event->mm;
				if (start_tick == -1)
				{
					start_tick = event->tick;
					point.x = 0;
					point.y = 0;
					total_num = 0;
				}

				auto next_event = event;
				for (next_event++; next_event != MidiEvents->end(); next_event++)
					if (staff == next_event->note_staff && 0x90 == static_cast<int>(next_event->evt & 0xF0) && next_event->vv > 0)
						break;

				if (next_event == MidiEvents->end() || next_event->oveline > line_num || next_event->mm-event->mm > 1) {
					//draw the corresponding curve
					if (index < xorder.size())
					{
						point.x += xorder[index++];
						if (max_elem_vv > event->vv) {
							point.y += max_elem_vv;
						} else {
							point.y += event->vv;
						}
						total_num++;
					}
					if (total_num)
					{
						point.x /= total_num;
						point.y /= total_num;
						vForce.push_back(point);
					}
					if (org_index < xorder.size())
					{
						org_point.x = xorder[org_index++];
						if (max_elem_vv > event->vv) {
							org_point.y = max_elem_vv;
						} else {
							org_point.y = event->vv;
						}
						vOrgForce.push_back(org_point);
					}
					start_tick = -1;
					max_elem_vv = -1;

					AdjustForcePoint(vForce);
					InterPolateDP(vForce, meas_bound[fromM].first, meas_bound[event->mm].second);

					//retrieve all the vigors in the same staff of the first line_num line
					char tmp_buf[8] = {0};
					std::string strForce = "[", strXOrder = "[";
					std::string strOrgForce = "[", strOrgXOrder = "[";
#if 0
					int nDegree = LeastSquareFit(vForce, vFactor, degree);
					for (int group = fromM; group <= event->mm; ++group) {
						strForce += "[";
						for (int x = 0; x < point_num; ++x) {
							float tmp = 0;
							for (int k = 0; k <= nDegree; ++k)
								tmp += vFactor[k]*pow((float)x, k);
							/*The force value changes in the scope[0, 127], so if there exist the case that the fitted value out of
							 *former range, then must take some measures to modify this value.
							 */
 							if (tmp < 0)
 								tmp = 0;
 							if (tmp > 127)
 								tmp = 127;
							sprintf(tmp_buf, "%d,", static_cast<size_t>(tmp));
							strForce += tmp_buf;
						}
						strForce += "],";
					}
					strForce += "]";
					sprintf(tmpBuffer, "drawVelocities(%d,%d,%d,%s);\n", fromM, event->mm, music->lines[line_num]->staves[staff-1]->y_offset, strForce.c_str());
					svgForceCurveContent->appendString(tmpBuffer);
#else
					for (auto force = vForce.begin(); force != vForce.end(); force++)
					{
						sprintf(tmp_buf, "%.1f,", force->x);
						strXOrder += tmp_buf;
						sprintf(tmp_buf, "%.1f,", force->y);
						strForce += tmp_buf;
					}
					for (auto force = vOrgForce.begin(); force != vOrgForce.end(); force++)
					{
						sprintf(tmp_buf, "%.1f,", force->x);
						strOrgXOrder += tmp_buf;
						sprintf(tmp_buf, "%.1f,", force->y);
						strOrgForce += tmp_buf;
					}
					strForce += "]";
					strXOrder += "]";
					strOrgForce += "]";
					strOrgXOrder += "]";
					staff_base_y += music->lines[line_num]->staves[staff-1]->y_offset;
					if (1 == staff)
						staff_base_y += 50;
					else		//2 == staff
						staff_base_y += 40-5;
					sprintf(tmpBuffer, "drawVelocities(%d, %s, %s, %d, 1);\n", fromM, strXOrder.c_str(), strForce.c_str(), staff_base_y);
					svgForceCurveContent->appendString(tmpBuffer);
					sprintf(tmpBuffer, "drawVelocities(%d, %s, %s, %d, 2);\n", fromM, strOrgXOrder.c_str(), strOrgForce.c_str(), staff_base_y);
					svgForceCurveContent->appendString(tmpBuffer);
#endif
					if (next_event == MidiEvents->end() || next_event->oveline > line_num)
						break;
					else		//next_event->mm-event->mm > 1
						fromM = -1;
				} else {
					if (next_event->note_index == event->note_index) {
						if ((max_elem_vv != -1 && max_elem_vv < event->vv) || max_elem_vv == -1)
							max_elem_vv = event->vv;
					} else {
						if (event->tick-start_tick < TICK_INCR && index < xorder.size())
						{
							point.x += xorder[index++];
							if (max_elem_vv > event->vv) {
								point.y += max_elem_vv;
							} else {
								point.y += event->vv;
							}
							total_num++;
						} 
						if (next_event->tick-start_tick >= TICK_INCR)
						{
							point.x /= total_num;
							point.y /= total_num;
							vForce.push_back(point);
							start_tick = -1;
						}
						if (org_index < xorder.size())
						{
							org_point.x = xorder[org_index++];
							if (max_elem_vv > event->vv) {
								org_point.y = max_elem_vv;
							} else {
								org_point.y = event->vv;
							}
							vOrgForce.push_back(org_point);
						}
						max_elem_vv = -1;
					}
				}
			}
		}
		break;
	}
}

void VmusImage::AdjustForcePoint(std::vector<Position>& vForce)
{
	if (vForce.size() < 2)
		return;

	/*the absolute of the slope of two points based on the horizontal should be controlled in the range(0, 15]/180*PI
	 *since the wave must be gently instead of fierce.
	 */
	const float theta = 15.0f;
	float average = std::accumulate(vForce.begin(), vForce.end(), 0.0f, [](float x, const Position& t)->float{ return x+t.y; })/vForce.size();
	for (int i = 1; i < vForce.size(); ++i) {
		if (atan(abs(vForce[i].y-vForce[i-1].y)/abs(vForce[i].x-vForce[i-1].x))*180/M_PI > theta) {
			float change = tan(theta/180*M_PI)*(vForce[i].x-vForce[i-1].x);
			if (vForce[i].y > vForce[i-1].y)
				vForce[i].y = vForce[i-1].y+change;
			else		//It's impossible that vForce[i].y == vForce[i-1].y because previous judgment condition
				vForce[i].y = vForce[i-1].y-change;
		}
	}

	//merge those adjacent points with the same force value
	Position p;
	std::vector<Position> tmpForce;
	for (auto first_iter = vForce.begin(); first_iter != vForce.end(); ++first_iter)
	{
		p.x = first_iter->x;
		p.y = first_iter->y;
		int total_num = 1;
		auto second_iter = first_iter+1;
		while (true)
		{
			if (second_iter == vForce.end() || second_iter->y != first_iter->y) {
				first_iter = second_iter-1;
				break;
			} else {
				p.x += second_iter->x;
				total_num++;
				second_iter++;
			}
		}
		p.x /= total_num;
		tmpForce.push_back(p);
	}
	vForce.clear();
	vForce = std::move(tmpForce);
}

void VmusImage::InterPolateDP(std::vector<Position>& vDisPoint, int start_x, int end_x)
{
	if (vDisPoint.size() < 2)
		return;

	enum { LEAST_DISTANCE = 20 };
	const float theta = 10.0/180*M_PI, ratio = 0.7;
	std::vector<Position> vInterPolate;
	for (int i = 0; i < vDisPoint.size(); ++i) {
		if (i == 0 || i == vDisPoint.size()-1) {		//start and end with oblique projectile motion
			int bound_distance;
			if (i == 0)
				bound_distance = vDisPoint[i].x-start_x;
			else		//i == vDisPoint.size()-1
				bound_distance = end_x-vDisPoint[i].x;

			if (bound_distance >= LEAST_DISTANCE) {		//construct extra point
				Position p;
				if (i == 0) {
					p.x = start_x+bound_distance*(1-ratio);
					p.y = vDisPoint[i].y-tan(theta)*bound_distance*ratio;
					vInterPolate.push_back(p);
					AddExtraPoint(vInterPolate, vDisPoint[i], OBLIQUE_PROJECTILE);
					AddExtraPoint(vInterPolate, vDisPoint[i+1], LOGISTIC_MOTION);
				} else {
					p.x = vDisPoint[i].x+bound_distance*ratio;
					p.y = vDisPoint[i].y-tan(theta)*bound_distance*ratio;
					AddExtraPoint(vInterPolate, p, OBLIQUE_PROJECTILE);
				}
			} else {
				if (i == 0) {
					vInterPolate.push_back(vDisPoint[i]);
					AddExtraPoint(vInterPolate, vDisPoint[i+1], LOGISTIC_MOTION);
				}
			}
		} else {		//the track of the middle ligature use the logistic equation
			AddExtraPoint(vInterPolate, vDisPoint[i+1], LOGISTIC_MOTION);
		}
	}

	vDisPoint.clear();
	vDisPoint = std::move(vInterPolate);
}

int VmusImage::AddExtraPoint(std::vector<Position>& vInterPolate, Position& end_p, MOTION_TYPE type)
{
	const Position start_p = vInterPolate.back();
	if (!(end_p.y-start_p.y))
		return -1;

	switch (type)
	{
	case OBLIQUE_PROJECTILE:
		{
			/*oblique projectile motion can be decomposed the uniform motion on the horizontal direction and
			 *motion of a free falling body on the vertical direction
			 */
			float t = sqrt(2*abs(end_p.y-start_p.y)/9.8);
			float v = abs(end_p.x-start_p.x)/t;
			Position p = vInterPolate.back();
			while (end_p.x-p.x > DISCREATE_POINT_DISTANCE)
			{
				p.x += DISCREATE_POINT_DISTANCE;
				if (end_p.y > start_p.y) {
					p.y = end_p.y-9.8*(end_p.x-p.x)*(end_p.x-p.x)/(2*v*v);
				} else {		//start_p.y > end_p.y
					p.y = start_p.y-9.8*(p.x-start_p.x)*(p.x-start_p.x)/(2*v*v);
				}
				vInterPolate.push_back(p);
			}
			break;
		}
	case LOGISTIC_MOTION:
		{
			float K = abs(end_p.y-start_p.y)*1.03, N0 = K-abs(end_p.y-start_p.y);
			float r = 2*log((K-N0)/N0)/(end_p.x-start_p.x);
			Position p = vInterPolate.back();
			while (end_p.x-p.x > DISCREATE_POINT_DISTANCE)
			{
				p.x += DISCREATE_POINT_DISTANCE;
				float t = K/(1+((K-N0)/N0)*exp(-r*(p.x-start_p.x)));
				if (end_p.y > start_p.y) {
					p.y = start_p.y+(t-N0);
				} else {
					p.y = start_p.y-(t-N0);
				}
				vInterPolate.push_back(p);
			}
			break;
		}
	}
	vInterPolate.push_back(end_p);
	return 0;
}

int VmusImage::LeastSquareFit(const std::vector<Position>& vPoint, std::vector<float>& vFactor, int nDegree)
{
	if (vPoint.empty())
		return -1;

	if (!vFactor.empty())
		vFactor.clear();
	if (vPoint.size() < nDegree+1)
		nDegree = vPoint.size()-1;

	std::vector<float> vInterRes;
	vInterRes.assign(nDegree+1, 0);
	vFactor.assign(nDegree+1, 0);

	//construct the matrix and result vector corresponding with linear equation group
	float* pMatrix = new float[(nDegree+1)*(nDegree+1)];
	float* pResult = new float[nDegree+1];
	pMatrix[0] = vPoint.size();
	pResult[0] = std::accumulate(vPoint.begin(), vPoint.end(), 0.0f, [](float x, const Position& p)->float{ return x+p.y; });
	for (int i = 1; i <= (nDegree<<2); i++)
	{
		float MatrixWeight = 0, VecWeight = 0;
		for (auto it = vPoint.begin(); it != vPoint.end(); it++)
		{
			float tmp = pow(it->x, i);
			MatrixWeight += tmp;
			if (i <= nDegree)
				VecWeight += tmp*it->y;
		}
		int row = (i <= nDegree) ? 0 : (i-nDegree);
		int col = (i <= nDegree) ? i : nDegree;
		while (row <= nDegree && col >= 0)
		{
			pMatrix[row*(nDegree+1)+col] = MatrixWeight;
			++row;--col;
		}
		if (i <= nDegree)
			pResult[i] = VecWeight;
	}

	//execute the LU decomposition by Doolittle formula
	for (int i = 0; i <= nDegree; i++) {
		for (int j = 0; j <= nDegree; j++) {
			int l = (i <= j) ? (i-1) : (j-1);
			float tmp = 0;
			for (int k = 0; k <= l; k++)
				tmp += pMatrix[i*(nDegree+1)+k]*pMatrix[k*(nDegree+1)+j];

			if (i <= j)		//upper triangular matrix
				pMatrix[i*(nDegree+1)+j] = pMatrix[i*(nDegree+1)+j]-tmp;
			else		//lower triangular matrix
				pMatrix[i*(nDegree+1)+j] = (pMatrix[i*(nDegree+1)+j]-tmp)/pMatrix[j*(nDegree+1)+j];
		}
	}

	//calculate the factor vector
	vInterRes[0] = pResult[0];		//first calculate the y vector that satisfied Ly=b, y=Ux
	for (int i = 1; i <= nDegree; ++i)
	{
		float tmp = 0;
		for (int j = 0; j < i; ++j)
			tmp += pMatrix[i*(nDegree+1)+j]*vInterRes[j];
		vInterRes[i] = pResult[i]-tmp;
	}
	vFactor[nDegree] = vInterRes[nDegree]/pMatrix[nDegree*(nDegree+1)+nDegree];		//then calculate the x vector, that is factors
	for (int i = nDegree-1; i >= 0; --i)
	{
		float tmp = 0;
		for (int j = nDegree; j > i; --j)
			tmp += pMatrix[i*(nDegree+1)+j]*vFactor[j];
		vFactor[i] = (vInterRes[i]-tmp)/pMatrix[i*(nDegree+1)+i];
	}

	delete[] pMatrix;
	delete[] pResult;
	return nDegree;
}