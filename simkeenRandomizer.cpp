/*
 * Level randomizer for mod "simulator keen"
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <iostream>
#include <vector>
#include <cstring>
#include <ctime>
#include <tuple>
#include <functional>
#include <csignal>
#include <fstream>
#include <math.h>

using namespace std;

#define VORT_MAP_RLE_TAG 0xFEFE
#define SOLID 196
#define AIR 143
#define SPACE 156
#define OCCUPIED 169
#define JUMPTHROUGH 185
#define HAZARD 429
#define BORDER 77

#define POINT1 144
#define POINT2 145
#define POINT3 146
#define POINT4 147
#define POINT5 148
#define AMMO 149
#define EXIT11 213
#define EXIT12 164
#define EXIT13 163
#define EXIT21 150
#define EXIT22 215
#define EXIT23 151

#define DOOR1U 199
#define DOOR1L 200
#define DOOR2U 284
#define DOOR2L 285

#define KEY1 397
#define KEY2 398

#define YORP 1 // grunt becomes Simu-yorp
//#define vortikid 2 
#define MINELAYER 3 // Vortimom becomes minelayer
#define GARG 6 // Foob becomes Simu-Garg
#define MINE 8 // jack becomes mine
#define SHOT 4 // meep becomes a shot
#define PLATFORMHORIZONTAL 9
#define PLATFORMVERTICAL 10
#define SHOTRIGHT 14
#define SHOTDOWN 15
#define ARM 16
#define ARMBOTTOM 278
#define KEEN 255
// 5 vortininja
// 7 ball

struct tile
{
	uint16_t x;
	uint16_t y;
	uint16_t id;
	bool sprite = false;
} keen;

typedef struct VorticonsMap
{
	uint16_t w;
	uint16_t h;
	uint16_t planeNumber = 2;
	uint32_t blank[2] = {0,1};
	uint32_t TED[2] = {0,1};
	uint16_t planeSize;
	uint32_t u[8] = {0,1,2,3,4,5,6,7} ;
	vector<vector<vector<uint16_t>>> planes = vector<vector<vector<uint16_t>>>(2);
} VorticonsMap;

int addSeed = 1;
int seed;
int difficulty = 1;
int level = 1;
bool loop = false;
vector<int> pi;
int piDigit = 0;
vector<tuple<int, int>> armLocation = {};

// in order to keep the order of the functions intact:
VorticonsMap createBottomToTopVerticalMap(tuple<int, int> xRange, tuple<int, int> yRange , bool exit);

void checkBottom(VorticonsMap *vMap, int yLine, int x1, int x2);

/////////////////////////////////////////////////////////////
//    Saving the map
/////////////////////////////////////////////////////////////

void store_pi_digits(std::vector<int>& v) {
  // A string with the first 1000 digits of pi
  std::string pi = "31415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679821480865132823066470938446095505822317253594081284811174502841027019385211055596446229489549303819644288109756659334461284756482337867831652712019091456485669234603486104543266482133936072602491412737245870066063155881748815209209628292540917153643678925903600113305305488204665213841469519415116094330572703657595919530921861173819326117931051185480744623799627495673518857527248912279381830119491298336733624406566430860213949463952247371907021798609437027705392171762931767523846748184676694051320005681271452635608277857713427577896091736371787214684409012249534301465495853710507922796892589235420199561121290219608640344181598136297747713099605187072113499999983729780499510597317328160963185950244594553469083026425223082533446850352619311881710100031378387528865875332083814206171776691473035982534904287554687311595628638823537875937519577818577805321712268066130019278766111959092164201989";
  // A variable to keep track of the current position in the string
  int pos = 0;
  // A loop to iterate over the string and convert each char to an int and store it in the vector
  while (pos < pi.length()) {
    // Get the current char and convert it to an int using ASCII code
    char c = pi[pos];
    int n = c - '0';
    // Push the int to the vector
    v.push_back(n);
    // Increment the position
    pos++;
  }
}

int get_next_pi_digit(vector<int>& v) {
  // Increment the index
  piDigit++;
  // Check if the index is valid and return the corresponding element from the vector or -1 if invalid
  if (piDigit >= 0 && piDigit < v.size()) {
    return v[piDigit];
  } else {
    piDigit = 0;
	return v[piDigit];
  }
}

uint16_t fread_u16(FILE *f)
{
	uint16_t u;
	fread(&u,2,1,f);
	return u;
}

uint32_t fread_u32(FILE *f)
{
	uint32_t u;
	fread(&u, 4, 1, f);
	return u;
}

void fwrite_u16(FILE *f, uint16_t val)
{
	fwrite(&val, 2, 1, f);
}

void fwrite_u32(FILE *f, uint16_t val)
{
	fwrite(&val, 4, 1, f);
}

uint16_t *VORT_DecompressRLE(FILE *f, uint32_t decompSize)
{
	uint16_t *data = new uint16_t[decompSize];

	for(uint16_t *ptr = data; decompSize;)
	{
		uint16_t val = fread_u16(f);
		if (val == VORT_MAP_RLE_TAG)
		{
			uint16_t length = fread_u16(f);
			val = fread_u16(f);
			while (length--)
			{
				*(ptr++) = val;
				decompSize -= 2;
			}
		}
		else
		{
			*(ptr++) = val;
			decompSize -= 2;
		}
	}
	return data;
}

VorticonsMap VORT_LoadMap(FILE *f)
{
	int decompSize = fread_u32(f);
	uint16_t *mapData = VORT_DecompressRLE(f, decompSize);
	VorticonsMap vMap;
	uint16_t width = mapData[0], height = mapData[1];
	uint16_t mapPlaneSize = mapData[7];
	vMap.w = width;
	vMap.h = height;
	vMap.planeSize = mapPlaneSize;
	vector<vector<uint16_t>> plane = {}, plane2 = {};
	for(int i = 0; i < height; i++)
	{
		vector<uint16_t> row = {}, row2 = {};
		for(int j = 0; j < width; j++)
		{
			row.push_back(mapData[j+i*width+16]);
			row2.push_back(mapData[j+i*width+16+mapPlaneSize/2]);
		}
		plane.push_back(row);
		plane2.push_back(row2);
	}
	vMap.planes[0] = plane;
	vMap.planes[1] = plane2;
  delete mapData;
	return vMap;
}

void VORT_CompressRLE(uint16_t* data, uint32_t dataSize, FILE *f)
{
	uint16_t currentVal = 0xFFFF;
	uint16_t currentValCount = 0;
	int test = dataSize;
	while (dataSize)
	{
		uint16_t val = *(data++);
		if (val != currentVal)
		{
			if (currentValCount > 3)
			{
				fwrite_u16(f, VORT_MAP_RLE_TAG);
				fwrite_u16(f, currentValCount);
				fwrite_u16(f, currentVal);
			}
			else
			{
				for(int i = 0; i < currentValCount; ++i)
				{
					fwrite_u16(f, currentVal);
				}
			}
			currentVal = val;
			currentValCount = 1;
		}
		else
		{
			currentValCount++;
		}
		dataSize -= 2;
	}
	if (currentValCount)
	{
		if (currentValCount > 3)
		{
			fwrite_u16(f, VORT_MAP_RLE_TAG);
			fwrite_u16(f, currentValCount);
			fwrite_u16(f, currentVal);
		}
		else
		{
			for(int i = 0; i < currentValCount; ++i)
			{
				fwrite_u16(f, currentVal);
			}
		}
	}
}

void VORT_SaveMap(VorticonsMap *vMap, FILE *f)
{
	uint16_t planeSize = (vMap->w * vMap->h * 2 + 15) & ~15;
	uint32_t dataLen = planeSize*2 + 32;
	uint16_t *data = new uint16_t[dataLen];
	uint16_t width = vMap->w;
	uint16_t height = vMap->h;
	data[0] = width;
	data[1] = height;
	data[2] = vMap->planeNumber;
	data[3] = 0;
	data[4] = 1;
	data[5] = 0;
	data[6] = 1;
	data[7] = planeSize;
	for(int i = 0; i < 8; i++)
	{
		data[i+8] = i;
	}
	for(int i = 0; i < height; i++)
	{
		for(int j = 0; j < width ; j++)
		{
			data[16 + width*i + j] = vMap->planes[0].at(i).at(j);;
			data[16 + width*i + j + planeSize/2] = vMap->planes[1].at(i).at(j);
		}
	}
	fwrite_u32(f, dataLen);
	VORT_CompressRLE(data, dataLen, f);
	delete data;
}

/////////////////////////////////////////////////////
//    Initiating the map
/////////////////////////////////////////////////////

// generate an empty map to start
VorticonsMap generateBlankLevel(int x1, int x2, int y1, int y2)
{
	VorticonsMap vMap;
	//cout << y1 << " " << y2; 
	int height = seed % max(y2-y1,1) + y1;
	int width = seed % max(x2-x1,1) + x1;
	//cout << "height: " << height << " width: " << width << "\n"; 
	int planeSize = (width * height * 2 + 15) & ~15;
	vMap.h = height;
	vMap.w = width;
	vMap.planeSize = planeSize;
	vector<vector<uint16_t>> plane = {}, plane2 = {};
	plane.reserve(height);
	plane2.reserve(height);
	for(int i = 0; i < height; i++)
	{
		vector<uint16_t> row = {}, row2 = {};
		row.reserve(width);
		row2.reserve(width);
		for(int j = 0; j < width; j++)
		{
			row.push_back(BORDER);
			row2.push_back(0);
		}
		plane.push_back(row);
		plane2.push_back(row2);
	}
	vMap.planes[0] = plane;
	vMap.planes[1] = plane2;
	return vMap;
}

// randomly generate the commander keen starting position
void startPosition(VorticonsMap *vMap, uint16_t maxHeight, bool atBottom = false)
{
	int height = min(vMap->h, maxHeight);
	int xPos = min(seed % 6 + 2, vMap->w - 2);
	int yPos = min(atBottom ? maxHeight : ((seed + addSeed) % max(height - 9,1)) + 5, vMap->h - 2);
	//cout << "keen x position" << xPos << " keen y position: " << yPos << "\n";
	keen.x = xPos;
	keen.y = yPos;
	keen.id = 255;
	keen.sprite = true;
	vMap->planes[1].at(yPos).at(xPos) = KEEN;
}

/////////////////////////////////////////////////////////////////////
//   Placing the points
/////////////////////////////////////////////////////////////////////

//points option 1
vector<tile> pop1(VorticonsMap* vMap, int x1, int x2, int y1, int y2)
{
	vector<tile> points= {};
	int lineHeight = round((y1+y2)/2);
	//cout << "pop1, lineHeight: " << lineHeight << " x2: " << x2 << " y1: " << y1 << " y2: " << y2 << "\n";
	if(x2 >= vMap->w || lineHeight + 2 >= vMap->h || lineHeight <= 1) return points;
	for(int i = x1; i < x2; i++)
	{
		if(i % 2 == 1 && vMap->planes[0].at(lineHeight).at(i) == AIR)
		{
			tile point;
			point.x = i;
			point.y = lineHeight;
			point.id = POINT1;
			points.push_back(point);
		}
	}
	return points;
}

vector<tile> pop2(VorticonsMap* vMap, int x1, int x2, int y1, int y2)
{
	//cout << "place points 2 \n";
	vector<tile> points= {};
	int centreY = max((seed + addSeed) % max(abs(y2-y1) , 2) + y1 - 2, min(y2-1, 2));
	int centreX = min((seed + addSeed) % max(abs(x2-x1) , 1) + x1, x2-1);
	addSeed += centreX;
	//cout << "x and y line 334: " << centreX << " " << centreY << " y1 and y2: " << y1 << " " << y2 << "\n";
	if(centreY + 1 >= vMap->h || centreX + 1 >= vMap->w || centreY <= 2) return points;
	if(vMap->planes[0].at(centreY-1).at(centreX) == AIR)
	{
		tile point;
		point.x = centreX;
		point.y = centreY-1;
		point.id = POINT1;
		points.push_back(point);
	}
	for(int i = centreX - 1; i < centreX+2; i++)
	{
		if(vMap->planes[0].at(centreY).at(i) == AIR)
		{
			tile point;
			point.x = i;
			point.y = centreY;
			point.id = POINT1;
			points.push_back(point);
		}
	}
	if(vMap->planes[0].at(centreY+1).at(centreX) == AIR)
	{
		tile point;
		point.x = centreX;
		point.y = centreY+1;
		point.id = POINT1;
		points.push_back(point);
	}
	return points;
}

vector<tile> pop3(VorticonsMap* vMap, int x1, int x2, int y1, int y2)
{
	vector<tile> points= {};
	int xPos = round((x1+x2)/2);
	//cout << "pop3 xPos: " << xPos << " y2: " << y2 << "\n";
	if(y2 >= vMap->h) return points;
	if(vMap->planes[0].at(y2).at(xPos) == AIR)
	{
		tile point;
		point.x = xPos;
		point.y = y2;
		point.id = POINT2;
		points.push_back(point);
	}
	if(vMap->planes[0].at(y2-1).at(xPos+1) == AIR)
	{
		tile point;
		point.x = xPos+1;
		point.y = y2-1;
		point.id = POINT2;
		points.push_back(point);
	}
	//cout << "end of pop3 \n";
	return points;
}

vector<tile> pop4(VorticonsMap* vMap, int x1, int x2, int y1, int y2)
{
	vector<tile> points= {};
	int centreY = max(y1 + seed % max(abs(y1-y2),1), y2+2);
	int startX = min(x1 + 1 + (seed+addSeed) % 5, x2-2);
	addSeed += centreY;
	//cout << "pop4; centreY: "<< centreY << " startX: " << startX  << " y1: " << y1 << " y2: " << y2 << "\n";
	if(centreY + 2 >= vMap->h)
	{
		centreY = vMap->h - 3;
	}
	if(vMap->h < 4) return points;
	for(int i = startX ; i < x2; i+=3)
	{
		for(int j = centreY-1; j < centreY + 2; j++)
		{
			//cout << "pop4: y: " << j << " x: " << i << "\n";
			if(vMap->planes[0].at(j).at(i) == AIR)
			{
				tile point;
				point.x = i;
				point.y = j;
				point.id = POINT1;
				points.push_back(point);
			}
		}
	}
	//cout << "completed pop4\n";
	return points;
}

vector<tile> pop5(VorticonsMap* vMap, int x1, int x2, int y1, int y2)
{
	//cout << "pop5\n";
	vector<tile> points= {};
	int xPos = round((x1+x2)/2);
	//cout << "x: " << x1 << " " << x2 << "avg x: " << xPos << " y: " << y1-2 << "\n";
	if(vMap->planes[0].at(abs(y1-2)).at(xPos) == AIR)
	{
		//cout << "before";
		tile point;
		point.x = xPos;
		point.y = abs(y1-2);
		point.id = POINT3;
		points.push_back(point);
	}
	//cout << "after";
	return points;
}

void placePoints(VorticonsMap *vMap, tuple<int, int> x, tuple<int, int> y)
{
	vector<function<vector<tile>(VorticonsMap*,int,int,int,int)>> PointOptions = {pop1, pop2, pop3, pop4, pop5};
	int i = (seed + addSeed + int(round(1.5*addSeed))) % PointOptions.size();
	//cout << "placepoints i: " << i << "\n";
	if(get<1>(y) <= 2 || get<1>(x) + 2 >= vMap->w) return;
	vector<tile> points = PointOptions.at(i)(vMap, get<0>(x), get<1>(x), get<0>(y), get<1>(y));
	//cout << "about to place points \n";
	for(tile point: points)
	{
		//cout << "x pos: " << point.x << " y pos: " << point.y << " --- ";
		vMap->planes[0].at(point.y).at(point.x) = point.id;
	}
	addSeed += i;
	//cout << "placed points\n";
}

void placeSprites(VorticonsMap *vMap, tuple<int, int> x, tuple<int, int> y)
{
	//cout << "placesprites \n";
	int x1 = get<0>(x), x2 = get<1>(x);
	if(x2 + 2 >= vMap->w) return;
	int locy = max(get<1>(y)-1,2);
	int locx = max(x1 + (seed % max(abs(x2-x1),1)) - 2,2);
	vector<int> sprites = {YORP, 0 , YORP, 0, YORP, 0};
	if(difficulty > 1) sprites.push_back(GARG);
	if(difficulty > 1) sprites.push_back(GARG);
	if(difficulty == 2) sprites.push_back(0);
	if(difficulty > 2) sprites.push_back(MINE);
	if(level > 5) sprites.push_back(MINELAYER);
	if(difficulty == 4) sprites.push_back(GARG);
	if(difficulty == 4) sprites.push_back(MINE);
	if(level > 8) sprites.push_back(MINELAYER);
	int random = (seed + addSeed) % sprites.size();
	addSeed += random + locx;
	if(abs(keen.x - locx) > 4)
	{
		vMap->planes[1].at(locy).at(locx) = sprites.at(random);
	}
}

///////////////////////////////////////////////////////////////////
//   generating the platforms
///////////////////////////////////////////////////////////////////

void generatePlatform(VorticonsMap *vMap, int x1, int x2, int y, int block = SOLID, int spriteChance = 1)
{
	//cout << "generate a platform between: " << x1 << " and " << x2 << " at height " << y << "\n";
	for(int i = x1; i < x2; i++)
	{
		////cout << "generate a platform at :" << y << ", " << i << "\n";
		vMap->planes[0].at(y).at(i) = block;
	}
	tuple<int, int> yloc = {max(y-4,2), y-1};
	tuple<int, int> xloc = {x1, x2};
	int test =  x1+x2+y; // sometimes breaks without this ¯\_(ツ)_/¯
	////cout << "xloc: " << x1 << " " << x2 << "\n";
	////cout << "yloc: " << max(y-4,2) << " " <<  y-1 << "\n";
	addSeed += 1;
	if(x2 - x1 > 2)
	{
		int sprites = (seed + addSeed) % spriteChance;
		if(sprites != 0) return;
		yloc = {y-1, y-2};
		placeSprites(vMap, xloc, yloc); 
	}
}

// Create one platform
void platformLayout(VorticonsMap* vMap, int x1, int x2, int y1, int y2)
{
	//cout << "platoform 1";
	int distance = (seed + addSeed) % 2 + 3;
	int initX = min((seed + addSeed) % 4 + x1, x2-2);
	int lenght = min((seed + addSeed) % 10 + initX, x2-1);
	int height = y1 - distance;
	//cout << "height: " << height << ",distance: " << distance << ",y1: " << y1 << "\n";
	addSeed = (addSeed + lenght) % 7;
	generatePlatform(vMap, initX, lenght, height);
}

// create a series of platoforms
void platformLayout2(VorticonsMap* vMap, int x1, int x2, int y1, int y2)
{
	//cout << "platform layout 2, x2: " << x2 << "\n";
	int distance = max((seed + addSeed) % 6,3);
	int height = y1 - distance;
	int initX =(seed + addSeed) % 6 + x1;
	int lenght =  min((seed + addSeed) % 8 + initX + 1, x2);
	int i = 0;
	while(height > 4)
	{
		//cout << "height: " << height << ",y2: " << y2 << "\n";
		generatePlatform(vMap, min(initX + 2 * i, x2-2), min(lenght + 2 * i,x2-1), max(height, y2 - 1));
		i++;
		height -= 3;
	}
	addSeed += lenght;
}

// create a series of platforms and a wall
void platformLayout3(VorticonsMap* vMap, int x1, int x2, int y1, int y2)
{
	//cout << "platform edges" << x1 << " " << x2 << " " << y1 << " " << y2 << "\n";
	int distance = max((seed + addSeed) % 6,3);
	int height = y1 - distance;
	int initX =(seed + addSeed) % 6 + x1;
	int lenght =  seed + addSeed % 2 == 0 ? min((seed + addSeed) % 8 + initX + 1, x2) : x2-1;
	int i = 0;
	int x3 = x1;
	while(height > 4)
	{
		//cout << "before\n";
		generatePlatform(vMap, min(initX + 2 * i, x2-2), min(lenght + 2 * i,x2-1), max(height, y2 - 1));
		//cout << "after\n";
		x3 = min(lenght + 2 * i,x2);
		tuple<int, int> x = {x1, x2};
		tuple<int, int> y = {y1, y2};
		//cout << "about to place points \n";
		placePoints(vMap, x, y);
		i++;
		height -= 3;
	}
	int maxHeight = (seed + addSeed) % 2 == 0 ? max((seed + addSeed) % 7, y2) : y2 - 1;
	bool check = true;
	//cout << "maxHeight: " << maxHeight << "\n";
	for(int j = y1-3; j > maxHeight; j--)
	{
		if(!check || vMap->planes[0].at(j-3).at(x3-2) == AIR)
		{
			generatePlatform(vMap, x3-2, x3-1, j);
			check = false;
		}
	} 
	addSeed += lenght;
}

// create a series of platforms connected by vertical walls
void platformLayout4(VorticonsMap* vMap, int x1, int x2, int y1, int y2)
{
	//cout << "layout4\n";
	// Variable "distance" is the height of the lowest platform from the floor at height "y1", placing that platform at height "height"
	// the leftmost block of this block is placed at x-coordinate "initX", while the rightmost block of the platform is at "initX" + "length"
	// The loop creates continues to create platforms until the height comes to close to the ceiling
	// a bool 'first" indicates whether it is the lowest platform, if it is not, a vertical wall is made between them.
	int distance = max((seed + addSeed) % 6,3);
	int height = y1 - distance;
	int initX =(seed + addSeed) % 6 + x1;
	int lenght =  (seed + addSeed) % 2 == 0 ? min((seed + addSeed) % 8 + initX + 1, x2) : x2-1;
	////cout << "initX: " << initX << " length: " << lenght << "y2:" << y2 << "\n";
	int i = 0;
	bool first = true;
	while(height > y2+1)
	{
		int start = initX + 2 * i;
		int end = min(lenght + 2 * i, x2);
		////cout << "start: " << start << " end: " << end << " height: " << height << "\n";
		generatePlatform(vMap, min(start, x2-2), min(end,x2-1), max(height, y2 - 1));
		tuple<int, int> x = {x1, x2};
		tuple<int, int> y = {y1, y2};
		placePoints(vMap, x, y);
		//cout << "before if\n";
		if(!first && vMap->w > max(start, end)) 
		{
			for(int j = 0; j < 3; j++)
			{
				int middle = (start + end) / 2;
				//cout << "start end: " << start << " " << end << "middle: " << middle << " j+height: " << j+height << "\n";
				generatePlatform(vMap, middle, middle + 1, j + height);
			}
		}
		//cout << "after if \n";
		first = false;
		i++;
		height -= 3;
	}
	addSeed += lenght;
}

void platformLayout5(VorticonsMap* vMap, int x1, int x2, int y1, int y2)
{
	//cout << "platformLayout 5 \n";
	// Check if the block is broad enough for this kind of layout
	int xRange = x2-x1;
	if(abs(y2-y1) < 6) return;
	if(x1+8 >= vMap->w) return;
	if(y2-6<4) return;

	//Check if platforms to the left
	bool platformsLeft = false;
	int xEnd = max(x1-6, 3);
	int yEnd = min(y2+6, y1);
	if(y2-y1 > 8)
	{
		for(int x=x1; x > xEnd; x--)
		{
			for(int y=y2; y < yEnd; y++)
			{
				if(vMap->planes.at(0).at(y).at(x) == SOLID || vMap->planes.at(0).at(y).at(x) == JUMPTHROUGH)
				{
					platformsLeft = true;
				}
			}
		}
	}
	//Create the planned platforms
	//cout << "creating planned plaftorms\n";
	int number = max((xRange - 2)/6 - 1,1);
	for(int i = 0; i < number; i++)
	{
		generatePlatform(vMap, x1+2+i*6, x1+6+i*6, y2-2, JUMPTHROUGH);
		generatePlatform(vMap, x1+5+i*6, x1+9+i*6, y2-6);
		tuple<int, int> x = {x1+2+i*6, x1+2+(i+1)*6};
		tuple<int, int> y = {y2-2, y2+2};
		placePoints(vMap, x, y);
		if(y2+5 >= vMap->h) break;
		//cout << "placing hazards; y2; " << y2 << " x1: " << x1 << " i: " << i << "\n";
		vMap->planes.at(0).at(y2+5).at(x1+5+i*6) = SOLID;
		vMap->planes.at(0).at(y2+5).at(x1+6+i*6) = HAZARD;
		vMap->planes.at(0).at(y2+5).at(x1+7+i*6) = HAZARD;
		vMap->planes.at(0).at(y2+5).at(x1+8+i*6) = SOLID;
		//cout << " and done with placing them\n";
		if(x1+12+i*6 < x2)
		{
			generatePlatform(vMap, x1+8+i*6, x1+12+i*6, y2-2, JUMPTHROUGH);
		}
	}
	//cout << "middle of platform5 function \n";
	// make an upwards stair if no platforms to the left
	if(!platformsLeft)
	{
		int height = vMap->h-1;
		int ypos_btm = height;
		for(int y = height; y > y2 ; y--)
		{
			if(vMap->planes.at(0).at(y).at(x1) == AIR)
				break;
			ypos_btm=y-4;
		}
		do
		{
			vMap->planes.at(0).at(ypos_btm).at(x1) = JUMPTHROUGH;
			vMap->planes.at(0).at(ypos_btm).at(x1+1) = JUMPTHROUGH;
			tuple<int, int> x = {x1, x1+3};
			tuple<int, int> y = {ypos_btm-5, ypos_btm-1};
			placePoints(vMap, x, y);
			ypos_btm -= 6;
		}
		while(ypos_btm > y2+4);
	}
	//cout << "done with platform 5 \n";
}

void platformLayout6(VorticonsMap* vMap, int x1, int x2, int y1, int y2)
{
	//cout << "platformLayout 6 \n";
	int xRange = x2-x1;

	//Check if platforms to the left
	bool platformsLeft = false;
	int xEnd = max(x1-6, 2);
	int yEnd = min(y2+6, y1);
	if(y2+3 >= vMap->h) y2 -=2;
	if(y1-y2 > 8)
	{
		for(int x=x1; x > xEnd; x--)
		{
			for(int y=y2; y < yEnd; y++)
			{
				if(vMap->planes.at(0).at(y).at(x) == SOLID || vMap->planes.at(0).at(y).at(x) == JUMPTHROUGH)
				{
					platformsLeft = true;
				}
			}
		}
	}
	//Create the planned platforms
	int number = max((xRange - 2)/4 - 1,1);
 	for(int i = 0; i < number; i++)
	{
		generatePlatform(vMap, x1+1+i*4, x1+3+i*4, y2+2, JUMPTHROUGH);
		tuple<int, int> x = {x1+2+i*6, x1+2+(i+1)*6};
		tuple<int, int> y = {y2-2, y2+2};
		placePoints(vMap, x, y);
	}
	//cout << "created platforms \n";
	// make an upwards stair if no platforms to the left
	if(!platformsLeft)
	{
		int height = vMap->h-1;
		int ypos_btm = height;
		for(int y = height; y > y2 ; y--)
		{
			if(vMap->planes.at(0).at(y).at(x1) == AIR)
				break;
			ypos_btm=y-4;
		}
		do
		{
			vMap->planes.at(0).at(ypos_btm).at(x1) = JUMPTHROUGH;
			vMap->planes.at(0).at(ypos_btm).at(x1+1) = JUMPTHROUGH;
			tuple<int, int> x = {x1, x1+3};
			tuple<int, int> y = {ypos_btm-5, ypos_btm-1};
			placePoints(vMap, x, y);
			ypos_btm -= 6;
		}
		while(ypos_btm > y2+2);
	}
}

// Create one platform that can be jumped through
void platformLayout7(VorticonsMap* vMap, int x1, int x2, int y1, int y2)
{
	int distance = (seed + addSeed) % 2 + 3;
	int initX = min((seed + addSeed) % 4 + x1, x2-2);
	int lenght = min((seed + addSeed) % 10 + initX, x2-1);
	int height = y1 - distance;
	//cout << "platform 7, height: " << height << ",distance: " << distance << ",y1: " << y1 << "\n";
	addSeed = (addSeed + lenght) % 7;
	generatePlatform(vMap, initX, lenght, height, JUMPTHROUGH);
}

// Create a series of platform with one hazard at the bottom
void platformLayout8(VorticonsMap* vMap, int x1, int x2, int y1, int y2)
{
	if(y2+3 >= vMap->h) return;
	y1 -= 2;
	x1 += 1;
	//cout << "platform8 y1:" << y1 << " y2: " << y2 << " x1: " << x1 << "\n";
	vMap->planes.at(0).at(y1).at(x1+1) = SOLID;
	vMap->planes.at(0).at(y1-1).at(x1+1) = HAZARD;
	for(int y=y1; y >= y2 ; y--)
	{
		vMap->planes.at(0).at(y).at(x1) = SOLID;
	}
	int xEnd = min(x1 + 8, x2-2);
	for(int x = x1+1 ; x < xEnd ; x++)
	{
		vMap->planes.at(0).at(y2).at(x) = JUMPTHROUGH;
	}
	tuple<int, int> xr = {x1+1, x2};
	tuple<int, int> yr = {y2-3, y2};
	placePoints(vMap, xr, yr);
	//cout << "placed points in layout 8\n";
	if(y2 + 6 < y1)
	{
		//cout << "if \n";
		for(int x = x1+1 ; x < xEnd ; x++)
		{
			vMap->planes.at(0).at(y2+3).at(x) = JUMPTHROUGH;
			yr = {y2, y2+3};
			placePoints(vMap, xr, yr);
		}
		y2 += 3;
	}
	//cout << "platformlayout 8; y2: " << y2 << "\n";
	if(vMap->planes.at(0).at(y2+2).at(x1+1) == AIR)
	{
		vMap->planes.at(0).at(y2+1).at(x1+1) = POINT4;
		vMap->planes.at(0).at(y2+2).at(x1+1) = JUMPTHROUGH;
	}
	xEnd = min(xEnd + 1, x2-2);
	//cout << "platformlayout 8 do while loop \n";
	do
	{
		//cout << "while loop y2: " << y2 << " xEnd: " << xEnd << " x2: " << x2 << "\n";
		vMap->planes.at(0).at(y2+3).at(xEnd) = JUMPTHROUGH;
		y2 += 3;
	}
	while(y2 + 2 < y1);
}

// create a platform with one garg and a point4 item, check if it's required to create platforms to reach
void platformLayout9(VorticonsMap* vMap, int x1, int x2, int y1, int y2)
{
	if(x2 < 5) return;
	//cout << "platform 9\n";
	// check if walls on the left side
	bool left = false;
	int y0 = y2;
	for(int y = y2; y < y1; y++)
	{
		if(vMap->planes.at(0).at(y).at(x1-1) == SOLID)
		{
			left = true;
			y0 = y;
		}
	}

	// if not, check if walls on the right side
	bool right = false;
	
	// check if extra platforms are needed
	bool extraLeft = true;
	bool extraRight = true;
	//cout << "start sides construction \n";
	if(!left)
	{
		for(int y = y2; y < y1; y++)
		{
			if(vMap->planes.at(0).at(y).at(x2) == SOLID)
			{
				right = true;
				y0 = y;
			}
		}
	}
	// construct the platform if there is a wall
	// left side
	else
	{
		extraLeft = false;
		generatePlatform(vMap, x1-1, x1+5, y0+3);
		vMap->planes.at(0).at(y0+2).at(x1+4) = SOLID;
		vMap->planes.at(0).at(y0+1).at(x1+4) = SOLID;
		vMap->planes.at(0).at(y0+2).at(x1-1) = SOLID;
		vMap->planes.at(0).at(y0+1).at(x1-1) = SOLID;
		vMap->planes.at(0).at(y0).at(x1-1) = SOLID;
		generatePlatform(vMap, x1+2, x1+5, y0);
		vMap->planes.at(0).at(y0+2).at(x1+3) = POINT4;
		vMap->planes.at(1).at(y0+1).at(x1+1) = GARG;

		int checkX = min(x2-1, x1+9);
		bool overTheEdge = false;
		for(int y=y0-1; y>y2-1; y--)
		{
			if(vMap->planes.at(0).at(y).at(x1-1) == AIR)
			{
				overTheEdge = true;
			}
		}
		if(overTheEdge)
		{
			extraRight = false;
		}
		else
		{
			for(int y=y2; y<y1; y++)
			{
				for(int x=x1+5; x<checkX; x++)
				{
					if(vMap->planes.at(0).at(y).at(x) == SOLID || vMap->planes.at(0).at(y).at(x) == JUMPTHROUGH)
					{
						extraRight = false;
					}
				}
			}
		}
	}
	//cout << "between left and right side: left/right:" << left << " " << right << "\n";
	// right side
	if(right)
	{
		extraRight = false;
		generatePlatform(vMap, x2-6, x2-1, y0+3);
		vMap->planes.at(0).at(y0+2).at(x2-4) = SOLID;
		vMap->planes.at(0).at(y0+1).at(x2-4) = SOLID;
		vMap->planes.at(0).at(y0+2).at(x2-1) = SOLID;
		vMap->planes.at(0).at(y0+1).at(x2-1) = SOLID;
		vMap->planes.at(0).at(y0).at(x2-1) = SOLID;
		generatePlatform(vMap, x2-6, x2-1, y0);
		vMap->planes.at(0).at(y0+2).at(x2-3) = POINT4;
		vMap->planes.at(1).at(y0+1).at(x2-1) = GARG;

		bool overTheEdge = false;
		for(int y=y0-1; y>y2-1; y--)
		{
			if(vMap->planes.at(0).at(y).at(x2-1) == AIR)
			{
				overTheEdge = true;
			}
		}
		if(overTheEdge)
		{
			extraLeft = false;
		}
		else
		{
			int checkX = min(x1, x2-7);
			for(int y=y2; y<y1; y++)
			{
				for(int x=checkX; x<x2-4; x++)
				{
					if(vMap->planes.at(0).at(y).at(x) == SOLID || vMap->planes.at(0).at(y).at(x) == JUMPTHROUGH)
					{
						extraLeft = false;
					}
				}
			}
		}
	}
	int xMiddle = min(x1 + (seed+addSeed) % max(x2-x1,1) + 1, x2-5);
	bool middle = false;
	// construct platform in mid-air if no walls
	if(!left && !right)
	{
		middle = true;
		generatePlatform(vMap, xMiddle, xMiddle+5, y2+3);
		vMap->planes.at(0).at(y2+2).at(xMiddle) = SOLID;
		vMap->planes.at(0).at(y2+1).at(xMiddle) = SOLID;
		vMap->planes.at(0).at(y2+2).at(xMiddle + 4) = SOLID;
		vMap->planes.at(0).at(y2+1).at(xMiddle + 4) = SOLID;
		vMap->planes.at(0).at(y2+2).at(xMiddle + 2) = POINT4;
		vMap->planes.at(1).at(y2+1).at(xMiddle + 1) = GARG;
		//cout << "middle thingy, x2: " << x2 << " xMiddle: " << xMiddle <<  "\n" ;
		y0 = y2+2;
		int checkX = min(xMiddle + 8, x2+1);
		int checkY = y2 + 7;
		if(xMiddle + 4 < x2-2)
		{
			for(int y=y2; y<checkY; y++)
			{
				for(int x=xMiddle+5; x<checkX; x++)
				{
					if(vMap->planes.at(0).at(y).at(x) == SOLID || vMap->planes.at(0).at(y).at(x) == JUMPTHROUGH)
					{
						extraRight = false;
					}
				}
			}
		}
		else
		{
			extraRight = false;
		}
		//cout << "got here\n";
		if(!extraRight)
		{
			
			int checkX = max(x1, xMiddle-4);
			for(int y=y2; y<checkY; y++)
			{
				for(int x=checkX; x<xMiddle; x++)
				{
					if(vMap->planes.at(0).at(y).at(x) == SOLID || vMap->planes.at(0).at(y).at(x) == JUMPTHROUGH)
					{
						////cout << "false3 x: " << x << " y: " << y << "\n";
						extraLeft = false;
					}
				}
			}	
		}
		else
		{
			////cout << "false1";
			extraLeft = false;
		}
	}
	//cout << "constrcut the extra platforms\n";

	// construct these platforms
	if(extraLeft) 
	{
		int yBottom = y0+3;
		int check = vMap->h - 2;
		int xTarget = middle ? max(xMiddle - 3, x1): x1;
		for(int y=yBottom; y < check; y++)
		{
			yBottom = y;
			if (vMap->planes.at(0).at(y).at(xTarget) == SOLID || vMap->planes.at(0).at(y).at(xTarget) == JUMPTHROUGH || vMap->planes.at(0).at(y).at(xTarget+1) == SOLID || vMap->planes.at(0).at(y).at(xTarget+1) == JUMPTHROUGH)
			{
				break;
			}
		}
		while (y0 + 5 < yBottom)
		{
			generatePlatform(vMap, xTarget, xTarget+2, y0+4, JUMPTHROUGH);
			y0 += 4;
		}
	}
	if(extraRight) 
	{
		int yBottom = y0+3;
		int check = vMap->h - 2;
		int xTarget = middle ? min(xMiddle + 3, x2-1): x2-2;
		for(int y=yBottom; y < check; y++)
		{
			yBottom = y;
			if(vMap->planes.at(0).at(y).at(xTarget) == SOLID || vMap->planes.at(0).at(y).at(xTarget) == JUMPTHROUGH || vMap->planes.at(0).at(y).at(xTarget+1) == SOLID || vMap->planes.at(0).at(y).at(xTarget+1) == JUMPTHROUGH)
			{
				break;
			}
		}
		////cout << "xTarget : " << xTarget << " y0: " << y0+5 << " yBottom: " << yBottom <<  "\n";
		while (y0 + 5 < yBottom)
		{
			generatePlatform(vMap, xTarget, xTarget+2, y0+4, JUMPTHROUGH);
			y0 += 4;
		}
	}
}

// create a series of platforms using the createbottomtotop function
void platformLayout10(VorticonsMap* vMap, int x1, int x2, int y1, int y2)
{
	tuple<int, int> x = {x2-x1+4, x2-x1+5};
	tuple<int, int> y = {y1-y2+4, y1-y2+5};
	VorticonsMap vM = createBottomToTopVerticalMap(x, y, false);
	for(int x = x1; x < x2; x++)
	{
		int x_ = x-x1+2;
		for(int y = y2 ; y < y1; y++)
		{
			vMap->planes.at(0).at(y).at(x) = vM.planes.at(0).at(y-y2+2).at(x_);
		}
	}
	addSeed += 1;
}

void placePlatforms(VorticonsMap* vMap, int x1, int x2, int yPos, int yTop, int chance = 0, bool ten = true)
{
	if(yTop <= 2 || x2 + 2 >= vMap->w) return;
	vector<function<void(VorticonsMap*,int,int,int,int)>>platformOptions = {platformLayout,platformLayout2,platformLayout3, platformLayout4, platformLayout5, platformLayout6, platformLayout7, platformLayout8, platformLayout9, platformLayout10};
	int platforms = chance + (seed + addSeed) % (platformOptions.size());
	if(yPos > 6)
	{
		int i = platformOptions.size() - platforms - 1;
		//cout << "placeplatforms i: " << i << " platforms: " << platforms << " size: " << platformOptions.size() << "\n";
		int j = ten ? abs(i-1): i;
		if(yTop + 2 < vMap->h) yTop++;
		platformOptions.at(j)(vMap, x1, x2, yPos, yTop);
	}
	addSeed += 1;
}

//////////////////////////////////////////////////////////////////
//    place hazards
//////////////////////////////////////////////////////////////////

// one lone hazard on the floor
vector<tile> haz1(VorticonsMap *vMap, int x1, int x2, int y1, int y2)
{
	vector<tile> hazards = {};
	int xLocation = x1 + seed % max(abs(x2-x1),1);
	//cout << "\n haz1: " << x1 << " " << x2 << " " << y2 << " location: " << xLocation <<"\n" ;
	if(1 >= min(min(x1,x2),min(y1,y2))) return hazards;
	tile hazard;
	hazard.x = xLocation;
	hazard.y = y2;
	hazard.id = HAZARD;
	hazards.push_back(hazard);
	return hazards;
}

// place one hazard in the air
vector<tile> haz2(VorticonsMap *vMap, int x1, int x2, int y1, int y2)
{
	vector<tile> hazards = {};
	int xLocation = x1 + seed % max(abs(x2-x1),1);
	int yLocation = y1 + seed % max(abs(y2-y1-4), 1);
	if(yLocation + 2 >= vMap->h) return hazards;
	tile hazard;
	hazard.x = xLocation;
	hazard.y = yLocation;
	hazard.id = HAZARD;
	hazards.push_back(hazard);
	return hazards;
}

// place multiple hazards on the floor
vector<tile> haz3(VorticonsMap *vMap, int x1, int x2, int y1, int y2)
{
	////cout << "hazard 3";
	vector<tile> hazards = {};
	int xLocation = x1 + seed % max(abs(x2-x1),1);
	int xEnd = (x2 - xLocation) < 6 ? x2-1 : 4;
	for(int i = xLocation; i < xEnd; i++)
	{
		tile hazard;
		hazard.x = i;
		hazard.y = y2;
		hazard.id = HAZARD;
		hazards.push_back(hazard);
	}
	return hazards;
}

// Two hazards in the sky next to each other
vector<tile> haz4(VorticonsMap *vMap, int x1, int x2, int y1, int y2)
{
	vector<tile> hazards = {};
	int xLocation = x1 + seed % max(abs(x2-x1),1);
	int yLocation = y1 + seed % max(abs(y2-y1-4), 1);
	int neighbour = (seed % 2); 
	int direction = ((seed + addSeed) % 2) == 0 ? -1 : 1;
	if(yLocation + 2 >= vMap->h) return hazards;
	tile hazard1;
	hazard1.x = xLocation;
	hazard1.y = yLocation;
	hazard1.id = HAZARD;
	hazards.push_back(hazard1);
	tile hazard2;
	hazard2.x = xLocation + neighbour*direction;
	hazard2.y = yLocation + (1-neighbour)*direction;
	hazard2.id = HAZARD;
	hazards.push_back(hazard2);
	return hazards;
}

// Two hazards in the sky with a point item in between
vector<tile> haz5(VorticonsMap *vMap, int x1, int x2, int y1, int y2)
{
	vector<tile> hazards = {};
	int xLocation = x1 + seed % max(abs(x2-x1),1);
	int yLocation = y1 + seed % max(abs(y2-y1-4), 1);
	int neighbour = (seed % 2);
	//cout << "haz5 x: " << xLocation << " y: " << yLocation << "\n";
	if(yLocation + 2 >= vMap->h) return hazards;
	tile point;
	point.x = xLocation;
	point.y = yLocation;
	point.id = POINT3;
	hazards.push_back(point);
	tile hazard1;
	hazard1.x = xLocation - 1;
	hazard1.y = yLocation + (1-neighbour);
	hazard1.id = HAZARD;
	hazards.push_back(hazard1);
	tile hazard2;
	hazard2.x = xLocation + 1;
	hazard2.y = yLocation + (1-neighbour);
	hazard2.id = HAZARD;
	hazards.push_back(hazard2);
	return hazards;
}

// A homing mine
vector<tile> haz6(VorticonsMap *vMap, int x1, int x2, int y1, int y2)
{
	//cout << "haz6 \n";
	vector<tile> hazards = {};
	int xLocation = x1 + seed % max(abs(x2-x1),1);
	int yLocation = max(y1, y2-12);
	tile hazard;
	hazard.x = xLocation;
	hazard.y = yLocation;
	hazard.id = MINE;
	hazard.sprite = true;
	hazards.push_back(hazard);
	return hazards;
}

//A single vertical shot
vector<tile> haz7(VorticonsMap *vMap, int x1, int x2, int y1, int y2)
{
	//cout << "haz7 \n";
	vector<tile> hazards = {};
	if(x1 < 10) return hazards;
	int xLocation = x2;
	int yLocation = y1 + seed % max(abs(y2-y1),1);
	tile hazard;
	hazard.x = xLocation;
	hazard.y = yLocation;
	hazard.id = SHOT;
	hazard.sprite = true;
	hazards.push_back(hazard);
	return hazards;
}

//two vertical shots
vector<tile> haz8(VorticonsMap *vMap, int x1, int x2, int y1, int y2)
{
	//cout << "haz8 \n";
	vector<tile> hazards = {};
	if(x1 < 10) return hazards;
	int xLocation = x2;
	int yLocation = y1 + seed % max(abs(y2-y1),1);
	int yLocation2 = yLocation + 1 + seed % max(abs(y2-yLocation-1),1);
	if(yLocation2 + 2 >= vMap->h) return hazards;
	tile hazard;
	hazard.x = xLocation;
	hazard.y = yLocation;
	hazard.id = SHOT;
	hazard.sprite = true;
	hazards.push_back(hazard);
	tile hazard2;
	hazard2.x = xLocation;
	hazard2.y = yLocation2;
	hazard2.id = SHOT;
	hazard2.sprite = true;
	hazards.push_back(hazard2);
	return hazards;
}

//A high wall of hazards
vector<tile> haz9(VorticonsMap *vMap, int x1, int x2, int y1, int y2)
{
	//cout << "haz9 \n";
	vector<tile> hazards = {};
	int xLocation = x1 + seed % max(abs(x2-x1),1);
	int maxY = y2;
	int spacing = 8;
	if(y2 <= 8)
	{
		spacing = y2 - 2;
	}
	for(int y=y2; y > y2-spacing; y--)
	{
		if(vMap->planes.at(0).at(y).at(xLocation) == AIR)
		{
			maxY = min(y2, y);
		}
	}
	//cout << "between loops \n";
	for(int y=maxY+3; y <= y2; y++)
	{
		tile hazard;
		hazard.x = xLocation;
		hazard.y = y;
		hazard.id = HAZARD;
		hazards.push_back(hazard);
	}
	return hazards;
}

//Place a gun that shoots down vertically
vector<tile> haz10(VorticonsMap *vMap, int x1, int x2, int y1, int y2)
{
	vector<tile> hazards = {};
	int xStart = x1 + seed % max(abs(x2-x1)/2,1);
	int yStart = y2-2;
	int xLoc = xStart, yLoc = yStart;
	bool breakThis = false;
	for(int x=xStart; x < x2; x++)
	{
		xLoc = x;
		for(int y=yStart; y > y1; y--)
		{
			yLoc = y;
			if(vMap->planes.at(0).at(y).at(x) == SOLID && vMap->planes.at(0).at(y+1).at(x) == AIR && vMap->planes.at(0).at(y+2).at(x) == AIR && vMap->planes.at(0).at(y+3).at(x) == AIR)
			{
				breakThis = true;
				break;
			}
		}
		if(breakThis) break;
	}
	if(breakThis)
	{
		tile hazard;
		hazard.x = xLoc;
		hazard.y = yLoc+2;
		hazard.id = SHOTDOWN;
		hazard.sprite = true;
		hazards.push_back(hazard);
		tile hazard2;
		hazard2.x = xLoc;
		hazard2.y = yLoc+1;
		hazard2.id = 334;
		hazard2.sprite = false;
		hazards.push_back(hazard2);
	}
	return hazards;
}

//Place a gun that shoots right horizontally
vector<tile> haz11(VorticonsMap *vMap, int x1, int x2, int y1, int y2)
{
	vector<tile> hazards = {};
	int xStart = x1 - 2;
	int yStart = y1 - seed % max(abs(y2-y1)/2,1);
	int xLoc = xStart, yLoc = yStart;
	bool breakThis = false;
	for(int y=yStart; y > y2; y--)
	{
		yLoc = y;
		for(int x=xStart; x < x2; x++)
		{
			xLoc = x;
			if(vMap->planes.at(0).at(y).at(x) == SOLID && vMap->planes.at(0).at(y).at(x + 1) == AIR && vMap->planes.at(0).at(y).at(x + 2) == AIR && vMap->planes.at(0).at(y).at(x + 3) == AIR)
			{
				breakThis = true;
				break;
			}
		}
		if(breakThis) break;
	}
	if(breakThis)
	{
		tile hazard;
		hazard.x = xLoc+2;
		hazard.y = yLoc;
		hazard.id = SHOTRIGHT;
		hazard.sprite = true;
		hazards.push_back(hazard);
		tile hazard2;
		hazard2.x = xLoc+1;
		hazard2.y = yLoc;
		hazard2.id = 333;
		hazard2.sprite = false;
		hazards.push_back(hazard2);
	}
	return hazards;
}

void placeHazards(VorticonsMap *vMap, tuple<int, int> x, tuple<int, int> y)
{
	if(get<1>(y) <= 2 || get<1>(x) + 2 >= vMap->w) return;
	vector<function<vector<tile>(VorticonsMap*,int,int,int,int)>> options = {haz1, haz2};
	if(difficulty != 1)
	{
		options.push_back(haz3);
		options.push_back(haz4);
		options.push_back(haz5);
	}
	if(difficulty == 3)
	{
		options.push_back(haz6);
		options.push_back(haz6);
		options.push_back(haz7);
		options.push_back(haz8);
	}
	if(difficulty == 4)
	{
		options.push_back(haz7);
		options.push_back(haz8);
		options.push_back(haz9);
	}
	int  i = (seed + addSeed + int(round(1.5*addSeed))) % options.size();
	addSeed += i+1;
	//cout << "placehazards i: " << i << "\n"; // i = 0-3
	int y2 = get<1>(y);
	if(y2 + 5 < vMap->h) y2+= 2;
	vector<tile> hazards = options.at(i)(vMap, get<0>(x), get<1>(x), get<0>(y), get<1>(y));
	if(level > 8)
	{
		vector<tile> hazards2;
		if(addSeed % 2 == 0) hazards2 = haz10(vMap, get<0>(x), get<1>(x), get<0>(y), get<1>(y));
		else hazards2 = haz11(vMap, get<0>(x), get<1>(x), get<0>(y), get<1>(y));
		for(tile hazard: hazards2) hazards.push_back(hazard);
	}
	for(tile hazard: hazards)
	{
		if(hazard.y+3 < (vMap->h)) // don't add hazards in bottomless pits
			vMap->planes[hazard.sprite].at(hazard.y).at(hazard.x) = hazard.id;
			////cout << "here";
	}
}


//////////////////////////////////////////////////////////////////
//   Different types of areas
//////////////////////////////////////////////////////////////////

tuple<int, int> ChooseNextStart(int x2, int yPos, int yBottom)
{
	tuple<int, int> next;
	if(seed % 10 > 8) 
	{
		int y = min(yPos + 1, yBottom - 1);
		next = {x2, y};
	}
	else
	{
		int nextSegmentY = min(max(yPos + (seed + addSeed) % 8 - 4, 4), yBottom - 1);
		next =  {x2, nextSegmentY};
	}
	return next;
}

void createTheSolidBlocks(VorticonsMap *vMap, int x1, int x2, int y1, int y2, int block = SOLID)
{
	if(difficulty == 3)
	{
		if(y1 < vMap->h - 3)
		{
			generatePlatform(vMap, x1, x2, y1, block);
		}
		else
		{
			int y = vMap->h - 3;
			vMap->planes[1].at(y).at(x1 + 2) = PLATFORMHORIZONTAL;
			vMap->planes[0].at(y).at(2) = SOLID;
			vMap->planes[0].at(y).at(vMap->w - 2) = SOLID;
		}
	}
	else
	{
		for(int x = x1; x < x2; x++)
		{
			for(int y=y1; y < y2; y++)
			{
				vMap->planes.at(0).at(y).at(x) = SOLID;
			}
		}
	}
}

tuple<int, int> AreaWithJumpThroughBlock(VorticonsMap *vMap, int x1, int yPos, int x2, int height, bool first, int yTop = 2,  int yArea = 1)
{
	// this is quite the arcane problem, but sigabrt error get thrown when filling in the last tile in each rectangle
	// whether inside or outside a loop, so I skip over this one tile and start with it in the next segment.
	// as a bonus, it prevents wide bottomless pits
	vMap->planes.at(0).at(height-1).at(x1-1) = SOLID;
	// check if there's a floor
	int floor = vMap->h;
	if( yPos + 3 >= floor)
	{
		yPos = floor - 5;
	}


	// create the solid undergrouund for this area
	createTheSolidBlocks(vMap, x1, x2, yPos + 2, height);

	// choose a form, stairs or rectangle
	bool rectangle = (seed + addSeed) % 4 > 1 ? true : false;
	int yTopOfBlocks = yPos;
	// create rectangle
	if(rectangle)
	{
		int width = (seed + addSeed) % 4 + 1;
		int xStart = max(x2 - 1 - width - (seed + addSeed) % max(x2-x1,3), x1+1);
		int xStop;
		int height2 = max(yPos - (seed + addSeed) % max(yPos-yTop,3), yTop+2);
		yTopOfBlocks = height2-1;
		if(xStart + width + 1 >= vMap->w) xStop = x2-2; else xStop = xStart+width;
		////cout << "height: " << height2 << " xStart: " << xStart << " width: " << width << "\n";
		for(int y = height2; y < yPos + 2; y++)
		{
			generatePlatform(vMap, xStart, xStop, y, JUMPTHROUGH, 6);
		}
	}
	// create stairs
	else
	{
		int steps = seed % 2 + 1;
		int maxHeight = max((x2-x1)/steps-2,2);
		int height2 = (seed+addSeed) % maxHeight + 1;
		int yEnd = max(yPos - height2 - 1, yTop+2);
		int i = 0;
		yTopOfBlocks = yEnd-1;
		//cout << "maxHeight: " << maxHeight << " height: " << height2 << " steps: " << steps << " yPos: " << yPos << "\n";
		for(int y = yPos + 1; y > yEnd; y--)
		{
			int xStart = x1+1+steps*i;
			int xEnd = x1+steps*(2+height2-i);
			generatePlatform(vMap, xStart, min(xEnd, x2-1), y, JUMPTHROUGH, 6);
			i++;
		}
	}
	////cout << "yTopofBlocks: " << yTopOfBlocks << "yTop:" << yTop << "\n";;
	// x and y positions between which points, hazards and sprites must be placed.
	if(yTopOfBlocks - yTop > 4 && x2 - x1 > 6)
	{
		tuple<int, int> x = {x1+2, x2-2}, y = {yTop, yTopOfBlocks - 1};
		placePoints(vMap, x, y);
		placeSprites(vMap, x, y);
		// place some platforms in the sky
		placePlatforms(vMap, x1, x2, yPos, yTop+1);
		placeHazards(vMap, x, y);
		////cout << "test2\n";
	}
	// prepare next area
	return ChooseNextStart(x2, yPos, height);
}

tuple<int, int> blockedArea(VorticonsMap *vMap, int x1, int yPos, int x2, int height, bool first, int yTop = 2,  int yArea = 1)
{
	//cout << "blocked Area \n";
	// this is quite the arcane problem, but sigabrt error get thrown when filling in the last tile in each rectangle
	// whether inside or outside a loop, so I skip over this one tile and start with it in the next segment.
	// as a bonus, it prevents wide bottomless pits
	vMap->planes.at(0).at(height-1).at(x1-1) = SOLID;

	int xStart;
	if(!first)
	{
		xStart = x1;
		for(int y=vMap->h-4; y > yTop; y--)
		{
			yPos = y - 1;
			if(vMap->planes.at(0).at(y).at(x1-1) != SOLID)
			{
				break;
			}
		}
	}
	else
	{
		xStart = keen.x + 2;
		yPos = keen.y;
	}
	createTheSolidBlocks(vMap, x1, x2, yPos + 2, height);
	int i = 0;
	int yJump = max(yPos - (seed + addSeed) % 5, yTop + 2);
	int xJump = (seed + addSeed) % 3 + 1;
	int xJump2 = (seed + addSeed) % 3 + 2;
	addSeed += yJump;
	bool onOff = false;
	////cout << "yPos: " << yPos << " yJump: " << yJump << "\n"; 
	////cout << "blocked area" << " numbers :" << x2 << " " << xPos << " " << yPos << " " << xJump << " " << xJump2 << " " << yJump << "\n";
	while(xStart < x2)
	{
		if(i!=0)i--;
		if(i==0)
		{
			////cout << "switch, xPos:" << xPos << "x2:" << x2 << "line 684 \n";
			onOff = !onOff;
			tuple<int, int> x, y;
			if(xStart > x1 + 1)
			{
				if(onOff)
				{
					i = xJump2;
					x = {xStart-xJump, xStart-1}, y = {yPos+1, yJump};
					////cout << "x range: " << xStart-xJump << " " << xStart-1 << " y range: " << yPos+1 << " " << yJump << "\n";
					placePoints(vMap, x, y);
				}
				else
				{
					i = xJump;
					x = {xStart-xJump2+1, xStart-1}, y = {yPos, max(yPos - 2,yTop)};
					if(xJump > 2)placeSprites(vMap, x, y);
				}
			}
		}
		if(onOff) 
		{
			createTheSolidBlocks(vMap, xStart, xStart + 1, yJump, yPos + 2, JUMPTHROUGH);
			if(difficulty == 3) vMap->planes[0].at(yPos + 2).at(xStart) = AIR;
		}
		else if (difficulty == 3)
		{
			vMap->planes[0].at(yPos + 2).at(xStart) = JUMPTHROUGH;
		}
		xStart++;
	}
	////cout << "placing stuff \n";
	tuple<int, int> x = {max(keen.x+1, x1), x2-2}, y = {yTop, yJump-3};
	placePoints(vMap, x, y);
	placeSprites(vMap, x, y);
	////cout << "placed two things \n";
	if(yJump > 5)
	{
		////cout << "yTop: " << yTop << " yJump: " << yJump << "\n";
		
		// place some platforms in the sky
		placePlatforms(vMap, x1, x2, yPos, yTop);
		placeHazards(vMap, x, y);
	}
	return ChooseNextStart(x2, yPos, height);
}

tuple<int, int> enclosedArea(VorticonsMap *vMap, int x1, int yPos, int x2, int height, bool first, int yTop,  int yArea)
{
	//cout << "enclosed Area \n";

	// this is quite the arcane problem, but sigabrt error get thrown when filling in the last tile in each rectangle
	// whether inside or outside a loop, so I skip over this one tile and start with it in the next segment.
	// as a bonus, it prevents wide bottomless pits
	vMap->planes.at(0).at(height-1).at(x1-1) = SOLID;

	// height until the solid top solid tiles stop
	int randomY = seed % 7 + 3;
	int y1 = max(yPos - randomY, yTop);

	// If the first area is this one, create some extra solid tiles at the left border
	if(x1 < 5 && keen.x > 3 && !loop)
	{
	  	int x3 = keen.x - 1;
		for(int i = x1;  i < x3; i++)
		{
			for(int j = y1 ; j < yPos + 2; j++)
			{
				vMap->planes[0].at(j).at(i) = SOLID;
			}
		}
	}

	// create the bottom and top solid tiles
	
	createTheSolidBlocks(vMap, x1, x2, yPos + 2, height);
	for(int i = x1;  i < x2; i++)
	{
		for(int j = yTop; j < y1; j++)
		{
			if(i+first != x1 && ( yArea%2 != 0 || i < (vMap->w - 5)))
				vMap->planes[0].at(j).at(i) = SOLID;
		}
	}
	tuple<int, int> x = {x1+2, x2-2}, y = {y1, yPos - 1};
	//cout << "line 1537 \n";
	placePoints(vMap, x, y);
	placeHazards(vMap, x, y);
	placeSprites(vMap, x, y);
	//cout << "line 1541 \n";
	//create mangling arms at difficulty 4
	if(difficulty == 4)
	{
		int spacing = min(4 + seed % 3, yPos - 1);
		int xLocation = x2 - 2 -(seed + addSeed) % max(abs(x1-x2)/2,1);
		//cout << "xLocation: " << xLocation << "mapSize" << vMap->h << " " << vMap->w << "\n";
		//cout << "yPos: " << yPos << " spacing: " << spacing << "\n";
		vMap->planes.at(1).at(yPos - spacing).at(xLocation) = ARM;
		vMap->planes.at(0).at(yPos - spacing).at(xLocation) = 278;
		armLocation.push_back({xLocation, yPos - spacing});
		for(int y = yPos; y >= yPos - spacing; y--)
		{
			vMap->planes.at(0).at(y).at(xLocation - 2) = AIR;
			vMap->planes.at(0).at(y).at(xLocation - 1) = AIR;
			vMap->planes.at(0).at(y).at(xLocation) = AIR;
			vMap->planes.at(0).at(y).at(xLocation + 1) = AIR;
			vMap->planes.at(0).at(y).at(xLocation + 2) = AIR;
		}
		for(int y = yPos - spacing - 1; y >= y1; y--)
		{
			vMap->planes.at(0).at(y).at(xLocation - 1) = SOLID;
			vMap->planes.at(0).at(y).at(xLocation) = SOLID;
			vMap->planes.at(0).at(y).at(xLocation + 1) = SOLID;
		}
		if(level > 8 && x1 > 4)
		{
			for(int y = yPos + 2; y < vMap->h-2; y++)
			{
				vMap->planes.at(0).at(y).at(xLocation - 3) = AIR;
				vMap->planes.at(0).at(y).at(xLocation - 2) = AIR;
			}
		}
		int xLocation2 = xLocation;
		if(xLocation + 6 < x2) xLocation2 += 5;
		else if(xLocation - 6 >= x1) xLocation2 -= 5;
		if(xLocation2 != xLocation && level > 8 && xLocation2 > 7)
		{
			vMap->planes.at(1).at(yPos - spacing).at(xLocation2) = ARM;
			vMap->planes.at(0).at(yPos - spacing).at(xLocation2) = 278;
			armLocation.push_back({xLocation2, yPos - spacing});
			for(int y = yPos; y >= yPos - spacing; y--)
			{
				vMap->planes.at(0).at(y).at(xLocation2 - 2) = AIR;
				vMap->planes.at(0).at(y).at(xLocation2 - 1) = AIR;
				vMap->planes.at(0).at(y).at(xLocation2) = AIR;
				vMap->planes.at(0).at(y).at(xLocation2 + 1) = AIR;
				vMap->planes.at(0).at(y).at(xLocation2 + 2) = AIR;
			}
			for(int y = yPos - spacing - 1; y >= y1; y--)
			{
				vMap->planes.at(0).at(y).at(xLocation2 - 1) = SOLID;
				vMap->planes.at(0).at(y).at(xLocation2) = SOLID;
				vMap->planes.at(0).at(y).at(xLocation2 + 1) = SOLID;
			}
		} 
	}
	tuple<int, int> next = {x2, yPos};
	return next;
}

tuple<int, int> flatArea(VorticonsMap *vMap, int x1, int yPos, int x2, int height, bool first, int yTop, int yArea)
{
	// this is quite the arcane problem, but sigabrt error get thrown when filling in the last tile in each rectangle
	// whether inside or outside a loop, so I skip over this one tile and start with it in the next segment.
	// as a bonus, it prevents wide bottomless pits
	vMap->planes.at(0).at(height-1).at(x1-1) = SOLID;


	// create the solid undergrouund for this area
	createTheSolidBlocks(vMap, x1, x2, yPos + 2, height);

	// x and y positions between which points, hazards and sprites must be placed.
	tuple<int, int> x = {x1+2, x2-2}, y = {yTop, yPos - 1};
	placePoints(vMap, x, y);
	placeSprites(vMap, x, y);
	// place some platforms in the sky
	placePlatforms(vMap, x1, x2, yPos, yTop+1);
	placeHazards(vMap, x, y);
	return ChooseNextStart(x2, yPos, height);
}

tuple<int, int> StairsArea(VorticonsMap *vMap, int x1, int yPos, int x2, int height, bool first, int yTop, int yArea)
{
	if(yPos > yTop+8 && !first) 
	{
		// this is quite the arcane problem, but sigabrt error get thrown when filling in the last tile in each rectangle
		// whether inside or outside a loop, so I skip over this one tile and start with it in the next segment.
		// as a bonus, it prevents wide bottomless pits
		for(int x = x1-1; x < x2; x++ )
		{
			vMap->planes.at(0).at(height-1).at(x) = SOLID;
		}

		int stairHeight = min(yPos - (seed % 7), yTop+3);
		tuple<int, int> x = {x1+2, x2-2}, y = {yTop, stairHeight};
		placePoints(vMap, x, y);
		x = {x1+2, x1+5}, y = {stairHeight, yPos};
		placePoints(vMap, x, y);
		placeSprites(vMap, x, y);
		placePlatforms(vMap, x1, x2, yPos, yTop+1);
		placeHazards(vMap, x, y);
		for(int i = height - 2; i > stairHeight; i--)
		{
			int x = x2 - 1;
			if (i > yPos) x = x1;
			else x = x1 - (i - yPos) + 5;
			for(int j = x; j < x2; j++)
			{
				if(i != x2-1 || j != stairHeight-1)
					vMap->planes[0].at(i).at(j) = SOLID;
				if(i <= yPos) 
				{
					vMap->planes[0].at(i-1).at(j) = AIR;
					vMap->planes[0].at(i-2).at(j) = AIR;
					vMap->planes[0].at(i-3).at(j) = AIR;
				}
			}
		}
	}
	else
		flatArea(vMap, x1, yPos, x2, height, first, yTop, yArea);
	return ChooseNextStart(x2, yPos, height);
}

tuple<int, int> descendingArea(VorticonsMap *vMap, int x1, int yPos, int x2, int height, bool first, int yTop, int yArea)
{
	if((yPos + 7) <= height) 
	{
		// this is quite the arcane problem, but sigabrt error get thrown when filling in the last tile in each rectangle
		// whether inside or outside a loop, so I skip over this one tile and start with it in the next segment.
		// as a bonus, it prevents wide bottomless pits
		vMap->planes.at(0).at(height-1).at(x1-1) = SOLID;

		// Decide the y-axis thickness for the wall and void 
		int heightWall = (seed + addSeed) % 2 + 1;
		int heightVoid = (seed + addSeed) % 3 + 2;
		addSeed += (heightWall + heightVoid);

		// select platforms above the area
		placePlatforms(vMap, x1, x2-2, yPos, yTop+1);

		// calculate the number of areas
		int thicknessOfLayers = heightWall + heightVoid;
		int numberOfLayers = int((height - yPos)/thicknessOfLayers);

		// The first two colums are solid wall
		for(int i = yPos + 2; i < height ; i++)
		{
			vMap->planes[0].at(i).at(x1) = SOLID;
			vMap->planes[0].at(i).at(x1+1) = SOLID;
		}

		// iterate for each area
		for(int i = 0; i < numberOfLayers; i++)
		{

			// create the first solid "roof" of the area
			int wallThickness = yPos + 2 + i*(thicknessOfLayers);
			for(int j = wallThickness; j < wallThickness + heightWall; j++)
			{
				int width = x2 - thicknessOfLayers + i;
				for(int k = x1+2; k < width; k++)
				{
					vMap->planes[0].at(j).at(k) = SOLID;
				}
			}

			// create the actual area space
			int thickness = yPos + 2 + (i+1)*heightWall + i*heightVoid;
			int end = min(thickness + heightVoid, height - 2);
			for(int j =thickness; j < end; j++)
			{
				int width = x2 - thicknessOfLayers + i;
				for(int k = x1+2; k < width; k++)
				{
					vMap->planes[0].at(j).at(k) = AIR;
				}
				tuple<int, int> x = {x1+2, max(width, x1+3)}, y = {thickness, thickness + heightVoid};
				placePoints(vMap, x, y);
			}

			// if its the bottom area, create a floor
			if(i == numberOfLayers - 1)
			{
				yPos = min(yPos + 1 + (i+1)*(heightWall + heightVoid), height - 1);
				for(int j = yPos; j < height; j++)
				{
					for(int k = x1+2; k <= x2; k++)
					{
						////cout << "x: " << k << " y: " << j << " --- ";
						vMap->planes[0].at(j).at(k) = SOLID;
						if(vMap->planes.at(0).at(j-2).at(k) == SOLID)
							vMap->planes[0].at(j-1).at(k) = SOLID;
					}
				}
			}
		}
	}

	// if not high enough, create a flat area instead
	else
	{
		flatArea(vMap, x1, yPos, x2, height, first, yTop, yArea);
	}
	return ChooseNextStart(x2, yPos, height);
}

// create an area with a hole in the middle
tuple<int, int> holedArea(VorticonsMap *vMap, int x1, int yPos, int x2, int height, bool first, int yTop, int yArea)
{
	//cout << "holed Area \n";
	int holeStart = x1 + min((addSeed + seed) % (max(abs(x2-x1),1) + 1), x2-3);
	int holeEnd = min(holeStart + min(min((addSeed + seed) % 7, x2-1), yPos - 2),x2-1);
	addSeed += holeEnd;
	bool withHazard = seed % 2 == 0 ? true : false;
	createTheSolidBlocks(vMap, x1, holeStart, yPos + 2, height);
	if(withHazard && difficulty != 3)
	{
		for(int i = holeStart;  i < holeEnd; i++)
		{
			for(int j = yPos + 2; j < height; j++)
			{
				vMap->planes[0].at(j).at(i) = j == (yPos + 2) ? AIR : HAZARD;
			}
		}
	}
	createTheSolidBlocks(vMap, holeEnd, x2, yPos + 2, height);
	//cout << "created solid blocks \n";
	tuple<int, int> x = {x1+2, x2-2}, y = {yTop, yPos - 1};
	// << "x1, x2 " << x1+2 << " " << x2-2 << "\n";
	placePoints(vMap, x, y);
	placePlatforms(vMap, x1, x2, yPos, yTop+1);
	//cout << "placed things \n";
	if(difficulty != 3) 
	{
		checkBottom(vMap, yPos+2, x1, x2);
	}
	else if (holeEnd - holeStart > 3)
	{
		vMap->planes[1].at(yPos + 2).at(holeStart + 1) = PLATFORMHORIZONTAL;
	}
	return ChooseNextStart(x2, yPos, height);
}

// create an area with a hole in the middle with a moving platform, Difficulty 3 only
tuple<int, int> holedArea2(VorticonsMap *vMap, int x1, int yPos, int x2, int height, bool first, int yTop, int yArea)
{
	if(height - 1 > yPos && x1 > 5)
	{
		yPos -= 3;
	}
	int holeStart = x1, holeEnd = x2-1;
	if(x2-x1 < 9 && x1 > 5)
	{
		vMap->planes[0].at(yPos + 2).at(x2 - 1) = SOLID;
	}
	else
	{
		holeStart = min(x1 + 1 + (addSeed + seed) % (abs(x2-x1-4) + 2), x1+4);
		holeEnd = min(x2 - (addSeed + seed) % 5 + 2,x2-1);
		addSeed += holeEnd;
		////cout << "holeStart: " << holeStart << " holeEnd: " << holeEnd << "\n";
		generatePlatform(vMap, x1, holeStart, yPos + 2);
		generatePlatform(vMap, holeEnd, x2, yPos + 2);
	}
	vMap->planes[1].at(yPos + 2).at(holeStart + 1) = PLATFORMHORIZONTAL;
	vMap->planes[0].at(yPos + 2).at(1) = SOLID;
	vMap->planes[0].at(yPos + 2).at(vMap->w - 1) = SOLID;
	tuple<int, int> x = {x1+2, x2-2}, y = {yTop, yPos - 1};
	// << "x1, x2 " << x1+2 << " " << x2-2 << "\n";
	placePoints(vMap, x, y);
	placePlatforms(vMap, x1, x2, yPos, yTop+1);
	return ChooseNextStart(x2, yPos, height);
}

// Create an area with a bent in it, Difficulty 3 only
tuple<int, int> bentArea(VorticonsMap *vMap, int x1, int yPos, int x2, int height, bool first, int yTop, int yArea)
{
	if(yPos + 3 > height && x1 > 5)
	{
		yPos -= 2;
	}
	int b1, b2 = x2;
	if(x2 - x1 > 8)
	{
		b1 = x1 + 4 + (seed + addSeed) % 3;
		b2 = b1 + 2 + (seed + addSeed) % 2;
	}
	else
	{
		b1 = x2 - 4;
	}
	bool bottomless = (seed + addSeed) % 2 ? true : false;
	int yBottom = min(yPos + 5, height - 1);
	generatePlatform(vMap, x1, b1, yPos + 2);
	generatePlatform(vMap, b2 - 5, b2, yPos - 2);
	generatePlatform(vMap, b2 - 5, b2 + 1, yPos - 3, HAZARD);
	if(!bottomless) generatePlatform(vMap, b1, x2, yBottom);

	int depth = yBottom - yPos;
	for(int i = 0; i < depth; i++)
	{
		if(b2 != x2)
		{
			vMap->planes[0].at(yPos - 2 + i).at(b2) = SOLID;
		}
		vMap->planes[0].at(yPos + 2 + i).at(b1) = SOLID;
	}
	if(bottomless) vMap->planes[1].at(yBottom).at(b1+2) = PLATFORMHORIZONTAL;
	vMap->planes[0].at(yBottom).at(1) = SOLID;
	vMap->planes[0].at(yBottom).at(vMap->w - 1) = SOLID;
	return ChooseNextStart(x2, yBottom - 1, height);
}

// Create an area with a vertical moving platform, Difficulty 3 only
tuple<int, int> vPlatformArea(VorticonsMap *vMap, int x1, int yPos, int x2, int height, bool first, int yTop, int yArea)
{
	createTheSolidBlocks(vMap,x1,x2 - 1, yPos + 2, yPos + 3);
	int middle = x1+(x2-x1)/2;
	vMap->planes[1].at(yPos).at(middle) = PLATFORMVERTICAL;
	vMap->planes[0].at(yTop+3).at(middle) = SOLID;
	vMap->planes[0].at(yTop+3).at(middle+1) = SOLID;
	vMap->planes[0].at(vMap->h - 2).at(middle) = SOLID;
	vMap->planes[0].at(vMap->h - 2).at(middle+1) = SOLID;
	tuple<int, int> x = {x1+2, x2-2}, y = {yTop, yPos - 1};
	placePoints(vMap, x, y);
	placeSprites(vMap, x, y);
	// place some platforms in the sky
	placePlatforms(vMap, x1, x2, yPos, yTop+1);
	placeHazards(vMap, x, y);
	return ChooseNextStart(x2, yPos, height);
}

// place the exit
void placeExitForLinear(VorticonsMap *vMap, int x1, int x2, int yPos, int y2, int yTop=2)
{
	yPos = max(min(vMap->h - 4, yPos),yTop+4);
	// place solid under exit

	////cout << "yPos: " << yPos << ", yTop: " << yTop << "\n";
	for(int x=x1 ; x<x2 ; x++)
	{
		for(int y=yTop ; y<y2 ; y++)
		{
			if(y <= yPos)
				vMap->planes[0].at(y).at(x) = AIR;
			if(y > yPos)
				vMap->planes[0].at(y).at(x) = SOLID;
		}
	}
	
	// place exit tiles
	vMap->planes[0].at(yPos-2).at(x1) = EXIT11;
	vMap->planes[0].at(yPos-1).at(x1) = EXIT21;
	vMap->planes[0].at(yPos).at(x1) = EXIT21;

	vMap->planes[0].at(yPos-2).at(x1+1) = EXIT12;
	vMap->planes[0].at(yPos-1).at(x1+1) = EXIT22;
	vMap->planes[0].at(yPos).at(x1+1) = EXIT22;

	vMap->planes[0].at(yPos-2).at(x1+2) = EXIT13;
	vMap->planes[0].at(yPos-1).at(x1+2) = EXIT23;
	vMap->planes[0].at(yPos).at(x1+2) = EXIT23;

	vMap->planes[0].at(vMap->h-3).at(x1-1) = SOLID;
}

// Checks that the bottom could be traversed if it is a bottomeless pit by checking the ceiling height
// vMap : the map that's being checked
// yLine : the y-value of the row that's being checked in vMap
// x1 and x2 : the x values between which the values are checked
void checkBottom(VorticonsMap *vMap, int yLine, int x1=2, int x2 = 0)
{
	if(difficulty == 3 || yLine < 3) return;
	// check the entire length of the map if x2 is 0, vMap->w couldn't be included in the definition above
	if( x2 == 0)
	{
		x2 = vMap->w - 2;
	}
	//cout << "checking bottom, yline: " << yLine << "\n";
	// create a shortcut
	vector<uint16_t> bottomRow= vMap->planes[0].at(yLine);
	int lastX = x1; // reminder of the location of the last solid tile to determine how broad the gap is
	int lastMaxX = 2; // reminder of high the ceiling was for the previous x-value, to determine if the ceiling is suddenly lower
	bool first = true;
	for(int x = x1; x < x2 ; x++)
	{
		//cout << "iterating in checkbottom; x= " << x << "\n";
		if(bottomRow.at(x) != SOLID)
		{
			// check if the passage is actually passable
			int tileType = vMap->planes.at(0).at(yLine-1).at(x);
			int tileType2 = vMap->planes.at(0).at(yLine-2).at(x);
			if(tileType == SOLID || tileType == HAZARD || tileType2 == SOLID || tileType2 == HAZARD)
			{
				continue;
			}
			// it's passable, the ceiling is at least 2 high
			int maxX = 2;
			// check how high the ceiling is, up to 6 tiles
			//cout << "before2 ";
			for(int y= max(yLine-3, 4); y > max(yLine-6, 2); y--)
			{
				int tileType3 = vMap->planes.at(0).at(y).at(x);
				if(tileType3 != SOLID && tileType3 != HAZARD)
				{
					maxX++;
				}
			}
			//cout << "after2 \n";
			if(lastMaxX < maxX && !first)
			{
				maxX = lastMaxX;
			}
			first = false;
			int recover = 0;

			if(difficulty == 4)
			{
				int hazards = 0;
				//cout << "maxX: " << maxX << "\n";
				for(int y=yLine-maxX; y>max(yLine-maxX-5,2); y--)
				{
					int tileType = vMap->planes.at(0).at(y).at(x);
					if(tileType == SOLID)
					{
						hazards = y;
						break;
					}
				}
				//cout << " after\n";
				////cout << "hazards: " << hazards << " maxX: " << maxX << "\n";
				if(hazards != 0 && maxX > 3)
				{
					int prevTile = vMap->planes.at(0).at(yLine-maxX).at(x-1);
					if(prevTile == SOLID || prevTile == HAZARD)
						vMap->planes.at(0).at(yLine-maxX).at(x) = HAZARD;
					else
						vMap->planes.at(0).at(yLine-maxX).at(x) = SOLID;
					for(int y=yLine-maxX-1; y>hazards;y--)
					{
						vMap->planes.at(0).at(y).at(x) = SOLID;
					}
					maxX -= 1;
					recover++;
				}
			}
			// if the gap is wider than the ceiling is tall OR if the ceiling is lower than the previous tile, then put in a solid tile
			//cout << "before 3 ";
			if(x - lastX >= maxX || lastMaxX > maxX + recover)
			{
				lastX = x;
				vMap->planes.at(0).at(yLine).at(x) = SOLID;
			}
			lastMaxX = maxX + recover; // reminder of the ceiling height gets updated
			//cout << "after 3 \n";
		}
		else
		{
			lastX = x; // reminder of the last solid tile gets updated
			first = true;
		}
	}
	//cout << "end of 'checkBottom'\n";
}

/////////////////////////////////////////////////////////////////////////////////////
//   Dividing the map into segments
/////////////////////////////////////////////////////////////////////////////////////

void on_sigabrt (int signum)
{
	//cout << "signal caught" << signum << "\n";
  	signal (signum, SIG_DFL);
}

// fill a rectangle with elements
tuple<int, int> nextSegment(VorticonsMap *vMap, tuple<int, int> rec, int height, bool first, int xPos = 2, int yPos = keen.y, int yTop = 2, int yArea = 1)
{
	tuple<int, int> a;
	int x1 = get<0>(rec);
	int x2 = get<1>(rec);
	int dely =  height;
	vector<function<tuple<int,int>(VorticonsMap*, int, int, int, int, bool, int, int)>>options = {AreaWithJumpThroughBlock, blockedArea, holedArea,flatArea, enclosedArea, StairsArea, descendingArea}; //name of area generating functions
	if(difficulty == 3)
	{
		options.push_back(holedArea2); //7
		options.push_back(bentArea); //8
		options.push_back(vPlatformArea); // 9
	}
	while(x1 < x2)
	{
		int i = (addSeed + seed) % options.size();
		addSeed += i + 1;
		signal(SIGABRT, &on_sigabrt);
		if (difficulty < 2)
			yPos = min(yPos, height-3);
		else
			yPos = min(yPos, height-2);
		// this is quite the arcane problem, but sigabrt error get thrown when filling in the last tile in each rectangle
		// whether inside or outside a loop, so I skip over this one tile and start with it in the next segment.
		// as a bonus, it prevents wide bottomless pits
		if(difficulty != 3)vMap->planes.at(0).at(height-1).at(x1-1) = SOLID;

		//cout << "option: " << i << " x1, x2: " << x1 << " " << x2 << "yTop, Bottom and Pos" << yTop << " " << height << " " << yPos << "\n";
		// i
		a = options.at(i)(vMap, x1, yPos, x2, height, first, yTop, yArea);//Call the possible terrain generators from the vector and apply the variables. Get back information for the next loop
		if (yPos > height-3 && difficulty < 3)
		{
			x1 = get<0>(a);
			yPos = min(get<1>(a), height-4);
		}
		else
		{
			x1 = get<0>(a);
			yPos = get<1>(a);
		}
		a = {x2, yPos};
	}
	return a;
}

// fill a rectangle in basic background blocks
tuple<int, int> firstArea(VorticonsMap *vMap , tuple<int, int> rectangle,int height, int xPos = 0, int yPos = keen.y, bool first = true, int yTop = 2, int yArea = 1)
{
	tuple<int, int> a;
	////cout << "creating air" << " yTop: " << yTop << " height: " << height << "\n";
	for(int i = yTop; i < height; i++)
	{
		for(int j = get<0>(rectangle); j < get<1>(rectangle); j++)
		{
			////cout << "x, y: " << j << " " << i << "\n";
			vMap->planes[0].at(i).at(j) = AIR;
		}
	}
	////cout << "created air \n";
	if(first)	a = nextSegment(vMap, rectangle, height, first, xPos, yPos, yTop = yTop, yArea = yArea);
	else {
		////cout << "first segment";
		a = nextSegment(vMap, rectangle, height, first, xPos, yPos, yTop, yArea = yArea);
	}
	return a;
}

//divide the level in rectangulars
VorticonsMap createLinearHorizontalMap(tuple<int, int> xRange = {40,60}, tuple<int, int> yRange = {20,30}, bool exit = true)
{
	// generate outline
	VorticonsMap vMap = generateBlankLevel(get<0>(xRange),get<1>(xRange),get<0>(yRange),get<1>(yRange));
	uint16_t height = vMap.h;
	uint16_t width = vMap.w;
	startPosition(&vMap, height);
	vector<int> topLeft;
	uint16_t boxWidth = 2;
	uint16_t boxes = 1;
	while(boxWidth != width - 6)
	{
		int x = boxWidth;
		topLeft.push_back(x);
		boxWidth += min((seed % max(width/2 - 13,1)) + 7, width - boxWidth- 2);
		if (boxWidth + 7 > width) boxWidth = width - 6;
		boxes++;
	}
	int x = boxWidth;
	topLeft.push_back(x);
	uint16_t done = 0;
	vector<int> currentBoxes = topLeft;
	int length = topLeft.size() - 1;
	tuple<int, int> a;
	tuple<int, int> rec = {topLeft[0], topLeft[1]};
	a = firstArea(&vMap, rec, height - 2);
	for(int i = 1; i < length; i++)
	{
		tuple<int, int> rec = {topLeft[i], topLeft[i+1]};
		a = firstArea(&vMap, rec, height - 2,get<0>(a), get<1>(a), false);
	}
	if(exit)placeExitForLinear(&vMap, width-6, width-2, get<1>(a), height-2);
	else{
		for(int x = width-6; x < width-2; x++)
		{
			for(int y = 2; y <= get<1>(a)+1; y++)
			{
				vMap.planes.at(0).at(y).at(x) = AIR;
			}
			for (int y=get<1>(a)+1; y<height-2;y++)
			{
				vMap.planes.at(0).at(y).at(x) = SOLID;
			}
		}
	}
	checkBottom(&vMap, height - 3);
	return vMap;
}

// create zigzags for vertical map
VorticonsMap createZigZagVerticalMap(tuple<int, int> xRange = {25,40}, tuple<int, int> yRange = {40,70}, bool exit = true)
{
	// generate outline
	VorticonsMap vMap = generateBlankLevel(get<0>(xRange),get<1>(xRange),get<0>(yRange),get<1>(yRange));
	uint16_t height = vMap.h;
	uint16_t width = vMap.w;
	vector<int> layer;
	uint16_t layerHeight = 2;
	uint16_t layers = 0;
	while(layerHeight < (height - 2))
	{
		int y = layerHeight;
		int p = 3;
		layer.push_back(y);
		//cout << "layerheight: " << layerHeight << "first thing: " << layerHeight + (seed % max(height/2 - 15,3)) + 8 << "second thing: " << height - layerHeight - 2 << "\n";
		layerHeight = min(layerHeight + (seed % max(height/2 - 15,5)) + 14, height - 2);
		if (layerHeight + 12 > height) layerHeight = height - 2;
		layers++;
	}
	int y = layerHeight;
	layer.push_back(y);
	//layer.push_back(height - 2);
	startPosition(&vMap, layer[1]);
	tuple<int, int> a = {keen.x, keen.y};
	for(int i = 0; i < layers; i++)
	{
		vector<int> topLeft;
		uint16_t boxWidth = 2 + 3*(i%2);
		uint16_t boxes = 1;
		while(boxWidth < (width - 5 + 3*(i%2)))
		{
			////cout << "in the while loop, boxWidth : " << boxWidth << "\n"; 
			int x = boxWidth;
			int p = 3;
			topLeft.push_back(x);
			boxWidth += min((seed % max(width/2 - 13,5)) + 7, width - boxWidth- 2);
			if (boxWidth + 7 > width) boxWidth = width - 5 + 3*(i%2);
			boxes++;
		}
		int x = boxWidth;
		topLeft.push_back(x);
		uint16_t done = 0;
		vector<int> currentBoxes = topLeft;
		int length = topLeft.size() - 1;
		tuple<int, int> rec = {topLeft[0], topLeft[1]};
		for(int j = 0; j < length; j++)
		{
			////cout << "layer[i+1] AKA height: " << layer[i+1] << "\n";
			tuple<int, int> rec = {topLeft[j], topLeft[j+1]};
			a = firstArea(&vMap, rec, layer[i+1],get<0>(a), get<1>(a), false, layer[i], i+1);
		}
		////cout << "built segments \n";
		int extraBlankSpaces = i % 2 == 1 ? 2 : topLeft[length];
		for (int j = extraBlankSpaces; j < extraBlankSpaces + 3; j++)
		{
			for(int k = layer[i]; k < layer[i+1]; k++)
			{
				vMap.planes.at(0).at(k).at(j) = AIR;
			}
		}
		if( i != 0)
		{
			int q = 3;
			////cout << "width: " << width << "\n";
			for(int x = 5 - 3*(i%2);x < width - 2 - 3*(i%2); x++)
			{
				vMap.planes.at(0).at(layer[i]-1).at(x) = SOLID;
				if(vMap.planes.at(0).at(layer[i]-2).at(x) == AIR)
				{
					//cout << "placing a hazard at x and y: " << x << " " << layer[i]-2  << " \n";
					vMap.planes.at(0).at(layer[i]-2).at(x) = HAZARD;
					if(x % 4 == 2 || x == width - 3)
					{
						vMap.planes.at(0).at(layer[i]-3).at(x) = SOLID;
					}
					
				}
			}
			int extraBlankSpaces2 = i % 2 != 1 ? 5 : topLeft[length];
			////cout << "extrablankspace2: " << extraBlankSpaces2;
			for(int x = extraBlankSpaces2 - 3; x < extraBlankSpaces2; x++)
			{
				for(int y = layer[i]+1; y < get<1>(a)+1;y++)
				{
					vMap.planes.at(0).at(y).at(x) = AIR;
					if(y = get<1>(a))
						vMap.planes.at(0).at(y+2).at(x) = SOLID;
				}
			}
		}
		if( i+1 == layers && exit)
		{	
			if(i%2 == 0)
				placeExitForLinear(&vMap, width-6, width-2, get<1>(a), layer[i+1],layer[i]);
			else
				placeExitForLinear(&vMap, 2, 6, get<1>(a), layer[i+1],layer[i]);
		}
		if( i < layers)
		{
			int new_yPos = max(layer[i+1] + seed % (layer[i+2]-layer[i+1]-6), layer[i+1]+3); 
			addSeed += new_yPos;
			a = {get<0>(a), new_yPos};
		}
		////cout << "layer i : " << i << "\n";
	}
	//cout << "out of the loop \n";
	//checkBottom(&vMap, height - 3);
	return vMap;
}

// create a vertical map with exit at the bottom and start at the top
//divide the level in rectangulars
VorticonsMap createTopToBottomVerticalMap(tuple<int, int> xRange = {25,40}, tuple<int, int> yRange = {40,70}, bool exit = true)
{
	// generate outline
	VorticonsMap vMap = generateBlankLevel(get<0>(xRange),get<1>(xRange),get<0>(yRange),get<1>(yRange));
	uint16_t height = vMap.h;
	uint16_t width = vMap.w;

	// divide the map in several vertically stacked layers
	vector<int> layer;
	uint16_t layerHeight = 2;
	uint16_t layers = 0;
	while(layerHeight < (height - 2))
	{
		int y = layerHeight;
		int p = 3;
		layer.push_back(y);
		////cout << "layerheight: " << layerHeight << "first thing: " << layerHeight + (seed % (height/2 - 15)) + 8 << "second thing: " << height - layerHeight - 2 << "\n";
		layerHeight = min(layerHeight + (seed % 10) + 3, height - 2);
		if (layerHeight + 12 > height) layerHeight = height - 2;
		layers++;
	}
	int yLast = layerHeight;
	layer.push_back(yLast);

	for(int x = 2; x < (width - 2); x++) {
		for(int y = 2; y < (height - 2) ; y++) {
			vMap.planes.at(0).at(y).at(x) = AIR;
		}
	}

	// generate the start position with three solid tiles under it
	startPosition(&vMap, layer[1]);
	int startX = keen.x;
	int startY = keen.y;
	vMap.planes.at(0).at(startY+2).at(startX) = SOLID;
	vMap.planes.at(0).at(startY+2).at(startX - 1) = SOLID;
	vMap.planes.at(0).at(startY+2).at(startX + 1) = SOLID;

	// for the top layer with the start position, generating platforms
	int length = layer.size()-1;
	for(int i = 0; i < length; i++)
	{
		int segmentWidth = seed % 9 + 4; // value between 4 and 13
		int rest = width % max(segmentWidth,2);
		int width2 = width - rest - keen.x;
		int horizontalLayers = width2/segmentWidth;
		for(int j = 0; (j+1)*segmentWidth < width - 2 - rest; j++)
		{
			int x1 = segmentWidth*j+keen.x+1;
			int x2 = segmentWidth*(j+1)+keen.x;
			int midPoint = segmentWidth*2/3;
			tuple<int, int> x = {x1, x2-2}, y = {layer[i], layer[i+1]};
			placePoints(&vMap, x, y);
			placePlatforms(&vMap, x1, x2, layer[i], layer[i+1], true);
			addSeed += midPoint;
		}
	}

	int bottomY = height - 3;
	for(int i = 2; i < width - 2; i++)
	{
		vMap.planes.at(0).at(bottomY-2).at(i) = AIR;
		vMap.planes.at(0).at(bottomY-1).at(i) = AIR;
		vMap.planes.at(0).at(bottomY).at(i) = SOLID;
		vMap.planes.at(0).at(bottomY+1).at(i) = SOLID;
	}
	tuple<int, int> x = {2, width-2}, y = {bottomY, bottomY-3};
	placePoints(&vMap, x, y);
	placePoints(&vMap, x, y);
	placePoints(&vMap, x, y);

	int exitLocation = (seed + addSeed) % max((width - 8) + 2,4);
	if(exit)placeExitForLinear(&vMap, exitLocation+1, exitLocation+5, bottomY-1, bottomY, bottomY - 10);
	return vMap;
}

// Vertical map with start position below and exit above
VorticonsMap createBottomToTopVerticalMap(tuple<int, int> xRange = {25,40}, tuple<int, int> yRange = {40,70}, bool exit=true)
{
	// generate outline
	VorticonsMap vMap = generateBlankLevel(get<0>(xRange),get<1>(xRange),get<0>(yRange),get<1>(yRange));
	uint16_t height = vMap.h;
	uint16_t width = vMap.w;

	// Make all tiles passable
	for(int x = 2; x < (width - 2); x++) 
	{
		for(int y = 2; y < (height - 2) ; y++) 
		{
			vMap.planes.at(0).at(y).at(x) = AIR;
		}
	}
	//cout << "put in air\n";
	
	// create solid tiles on the bottom
	int bottomY = height - 3;
	if(exit)
	{
		for(int i = 2; i < width - 2; i++)
		{
			vMap.planes.at(0).at(bottomY-2).at(i) = AIR;
			vMap.planes.at(0).at(bottomY-1).at(i) = AIR;
			vMap.planes.at(0).at(bottomY).at(i) = SOLID;
			vMap.planes.at(0).at(bottomY+1).at(i) = SOLID;
		}
	}

	//cout << "placed the solid tiles, bottomY: " << bottomY << "\n";
	// generate the start position at the bottom of the map
	startPosition(&vMap, bottomY - 2, true);
	//cout << "got start position";
	int startX = keen.x;
	int startY = keen.y;
	
	vector<tuple<tuple<int, int>, tuple<int, int>, bool>> rectangles;

	// initial position of first platform
	int platformY = bottomY - 3;
	int platformX = 2 + (seed + addSeed) % max(width - 4, 2);
	addSeed += platformX;
	//cout << "prepared for platform creation\n";
	// create a bunch of platforms
	while(platformY > 5)
	{
		int platformX2 = min(platformX + 1 + (seed + addSeed) % 7, width - 3 );
		generatePlatform(&vMap, platformX, platformX2, platformY);
		int x1 = 3;
		int x2 = platformX;
		int x3 = platformX2;
		int x4 = width - 3;
		int y1 = platformY;
		int y2 = platformY - 3 - (seed + addSeed) % 4;
		bool direction;
		if(platformX < width/4) direction = false;
		else if (platformX > width*3/4) direction = true;
		else direction = addSeed % 2 ? true : false;
		addSeed += x2+x3+1;
		if(direction) platformX = max(platformX - (seed + addSeed) % 7 , 3);
		else platformX = min(platformX + (seed + addSeed) % 7 , width - 3);
		platformY = y2;
		tuple<int, int> y = {y1, y2};
		tuple<int, int> left = {x1, x2};
		tuple<int, int> center = {x2, x3};
		tuple<int, int> right = {x3, x4};
		tuple<tuple<int, int>, tuple<int, int>, bool> rec1 = {y, left, false};
		tuple<tuple<int, int>, tuple<int, int>, bool> rec2 = {y, center, true};
		tuple<tuple<int, int>, tuple<int, int>, bool> rec3 = {y, right, false};
		if(x2 > 8) 			rectangles.push_back(rec1);
							rectangles.push_back(rec2);
		if(x3 < width - 8)	rectangles.push_back(rec3);
	}
	
	// prepare the exit location, which will be located on a platofrm
	int exitX1 = 3; // leftmost X-coordinate of the platform
	int exitX2 = 6; // rightmost X-coordinate of the platform
	int exitY = height - 2; // Y coordinate of the platform

	int highestY = height;
	int highestX = 9;
	////cout << "made the rectangles \n";
	// generate extra platforms
	for(tuple<tuple<int, int>, tuple<int, int>, bool> rectangle : rectangles)
	{
		int x1 = get<0>(get<1>(rectangle));
		int x2 = get<1>(get<1>(rectangle));
		int y = get<0>(get<0>(rectangle)) - 1;
		int y2 = get<1>(get<0>(rectangle));
		////cout << "dimensions: " << width << "x" << height << "\n";
		////cout << "xRange: " << x1 << " " << x2 << " y: " << y << " " << y2 << "\n";
		if(x2 - x1 > 3)
		{
			//cout << "y: " << y << "\n";
			placePlatforms(&vMap, x1, x2-2, y, y2, 0, false);
			////cout << "after platform\n";
			if(get<2>(rectangle))
			{

				////cout << "true\n";
				// ensure passibility
				for(int i = x1; i < x2; i++)
				{
					if(vMap.planes.at(0).at(y).at(i) == SOLID)
						vMap.planes.at(0).at(y).at(i) = AIR;
					if(vMap.planes.at(0).at(y-1).at(i) == SOLID)
						vMap.planes.at(0).at(y-1).at(i) = AIR;
				}

				// update most recent values for the "center" platform
				if(exitY > y)
				{
					exitX1 = x1;
					exitX2 = x2;
					exitY = y + 1;
				}
			}
			if((x2-x1)>2 && (y - y2) > 4) {
				//cout << "placing hazards\n";
				placeHazards(&vMap, get<1>(rectangle), get<0>(rectangle));
			}
			if(highestY > y && !get<2>(rectangle))
			{
				highestY = y;
				highestX = x1;
			}
		}
	}
	//cout << "made all the platforms \n";
	if(highestY < vMap.h - 3 && highestX < vMap.w - 3)
	{
		vMap.planes.at(0).at(highestY-1).at(highestX+1) = POINT4;
		vMap.planes.at(0).at(highestY-1).at(highestX+2) = HAZARD;
		if(vMap.planes.at(0).at(highestY+1).at(highestX+1) == AIR && vMap.planes.at(0).at(highestY+2).at(highestX+1) == AIR)vMap.planes.at(0).at(highestY).at(highestX+1) = SOLID;
		if(vMap.planes.at(0).at(highestY+1).at(highestX+2) == AIR && vMap.planes.at(0).at(highestY+2).at(highestX+2) == AIR)vMap.planes.at(0).at(highestY).at(highestX+2) = SOLID;
	}
	//cout << "finished off \n";
	if(exit)placeExitForLinear(&vMap, exitX1, max(exitX2, exitX1+3), exitY - 1, exitY+1, exitY-6);
	//cout << "placed the exit\n";
	return vMap;
}

// create a normal horizontally linear map with some extra platforms on top
VorticonsMap createDoubleLayeredHorizontalMap(tuple<int, int> xRange = {40,70}, tuple<int, int> yRange = {40,70}, bool exit=true)
{
	// generate outline
	VorticonsMap vMap = generateBlankLevel(get<0>(xRange),get<1>(xRange),get<0>(yRange),get<1>(yRange));
	uint16_t height = vMap.h;
	uint16_t width = vMap.w;

	int split = 25 + (seed + addSeed) %  max(height - 35,5);
	// Create Bottom
	tuple<int, int> xSizes = {width-9, width-9};
	tuple<int, int> ySizes = {height-split+4, height-split+4};
	VorticonsMap vM = createLinearHorizontalMap(xSizes, ySizes);
	//cout << "height - split" << height-split << " created the horizontal part\n";
	for(int x = 10; x < width ; x++)
	{
		int x_ = x - 9;
		for(int y = split; y < height - 2; y++)
		{
			//cout << "x: " << x_ << " y: " << y << "\n";
			vMap.planes.at(0).at(y).at(x) = vM.planes.at(0).at(y-split+4).at(x_);
			vMap.planes.at(1).at(y).at(x) = vM.planes.at(1).at(y-split+4).at(x_);
		}
	}
	//cout << "put the horizontal part in \n";
	// create left
	xSizes = {13 , 14};
	VorticonsMap vM2 = createBottomToTopVerticalMap(xSizes, ySizes, false);
	//cout << "created the bottom vertical part\n";
	for(int x = 2; x < 11; x++)
	{
		for(int y = split; y < height - 2; y++)
		{
			vMap.planes.at(0).at(y).at(x) = vM2.planes.at(0).at(y-split+5).at(x);
		}
	}
	//cout << "put the vertical part in\n";
	int ySolid = height - 3;
	int xSolid = 2;
	while(vMap.planes.at(0).at(ySolid).at(xSolid) != SOLID)
	{
		vMap.planes.at(0).at(ySolid).at(xSolid++) = SOLID;
	}
	if(keen.y < height - 6) vMap.planes.at(1).at(height-6).at(5) = GARG;
	for(int x=4; x < 8; x++)
	{
		vMap.planes.at(0).at(height-6).at(x) = POINT2;
	}
	int yCheck = height - 7;
	bool onOff = false;
	while(vMap.planes.at(0).at(yCheck).at(5) != BORDER)
	{
		if(vMap.planes.at(0).at(yCheck).at(5) == SOLID)
		{
			onOff = true;
		}
		if(vMap.planes.at(0).at(yCheck).at(5) != SOLID && onOff)
		{
			onOff = false;
			tuple<int, int> xForSprite = {3,7};
			tuple<int, int> yForSprite = {yCheck,yCheck-3};
			placeSprites(&vMap, xForSprite, yForSprite);
		}
		yCheck--;
	}
	//cout << "put in some extras\n";
	int x1 = 2;
	int x2 = x1 + width/4 + (seed + addSeed) % 2*width/3;
	int x3 = min(x2 + width/4 + (seed + addSeed) % width/3, width - 15);
	int x4 = width - 2;
	//cout << "xs: " << x1 << " " << x2 << " " << x3 << " " << x4 << " split + 4: " << split + 4 << "\n";
	vector<int> xs = {x1, x2, x3, x4};
	for(int i = 0; i < 3; i++)
	{
		xSizes = {xs[i+1] - xs[i] + 4, xs[i+1] - xs[i] + 4};
		ySizes = {split + 10, split + 10};
		//cout << "xSizes: " << get<0>(xSizes) << " " << get<1>(xSizes) << " - ";
		VorticonsMap vM_Top = createBottomToTopVerticalMap(xSizes, ySizes, false);
		//cout << "split: " << split << "created a vertical map - ";
		for(int x = xs[i]; x < xs[i+1]; x++)
		{
			for(int y = 2; y < split; y++)
			{
				vMap.planes.at(0).at(y).at(x) = vM_Top.planes.at(0).at(y).at(x-xs[i]+2);
				vMap.planes.at(1).at(y).at(x) = vM_Top.planes.at(1).at(y).at(x-xs[i]+2);
			}
		}
		//cout << "put the vertical map in \n";
	}
	//cout << "height - split" << height-split << "\n";
	return vMap;
}

//
// Create a map that is one big loop
//

VorticonsMap createBigLoopMap(tuple<int, int> xRange = {40,70}, tuple<int, int> yRange = {40,70}, bool exit=true)
{
	// generate outline
	VorticonsMap vMap = generateBlankLevel(get<0>(xRange),get<1>(xRange),get<0>(yRange),get<1>(yRange));
	uint16_t height = vMap.h;
	uint16_t width = vMap.w;
	loop = true;

	// Split the map in different segments
	int x1 = 2;
	int x4 = width - 2;
	int x2 = x1 + 3 + (seed + addSeed) % 14;
	int x3 = x4 - 10 - seed % 7;
	addSeed += x4;
	int y1 = 2;
	int y4 = height - 2;
	int y2 = y1 + 9 + (seed + addSeed) % 9;
	int y3 = y4 - 9 - addSeed % 9;
	addSeed += y4;

	// corner top left
	for(int x = x1; x < x2; x++)
	{
		for(int y=y1; y<y2; y++)
		{
			vMap.planes.at(0).at(y).at(x) = AIR;
		}
	}

	

	// top
	tuple<int, int> xSizes = {x3-x2+3, x3-x2+2};
	tuple<int, int> ySizes = {y2-y1+4, y2-y1+5};
	//cout << "about to model the top \n";
	VorticonsMap vM_top = createLinearHorizontalMap(xSizes, ySizes, false);
	//cout << "modeled the top \n\n";
	for(int x = x2+2; x < x3 ; x++)
	{
		int x_ = x-x2;
		for(int y = y1; y < y2; y++)
		{
			vMap.planes.at(0).at(y).at(x) = vM_top.planes.at(0).at(y-y1+2).at(x_);
			int spriteValue = vM_top.planes.at(1).at(y-y1+2).at(x_);
			vMap.planes.at(1).at(y).at(x) = spriteValue == 255 ? 0 : spriteValue;
		}
	}
	int yPos = y2;
	for(int y = y2; y > y1 ; y--)
	{
		if(vMap.planes.at(0).at(y).at(x2+2) == AIR)
			break;
		yPos=y;
	}
	if(difficulty == 2)
	{
		for(int y=y1; y < y2 ; y++)
		{
			if(y < yPos-2)
			{
				vMap.planes.at(0).at(y).at(x2) = SOLID;
				vMap.planes.at(0).at(y).at(x2+1) = AIR;
			}
			if(y >= yPos)
			{
				vMap.planes.at(0).at(y).at(x2) = SOLID;
				vMap.planes.at(0).at(y).at(x2+1) = SOLID;
			}
		}
		vMap.planes.at(0).at(yPos-2).at(x2) = DOOR2U;
		vMap.planes.at(0).at(yPos-1).at(x2) = DOOR2L;
		vMap.planes.at(0).at(yPos-1).at(x2+1) = AIR;
		vMap.planes.at(0).at(yPos-2).at(x2+1) = AIR;
		vMap.planes.at(0).at(yPos-4).at(x2+1) = KEY1;
		vMap.planes.at(0).at(yPos-5).at(x2+1) = KEY2;
		vMap.planes.at(0).at(yPos).at(x1) = SOLID;
		vMap.planes.at(0).at(yPos).at(x1+1) = SOLID;
		vMap.planes.at(1).at(yPos-2).at(x1+1) = KEEN;
	}
	if (difficulty == 4)
	{
		for(int y=y1; y < y2 ; y++)
		{
			if(y < yPos-2)
			{
				vMap.planes.at(0).at(y).at(x2+1) = SOLID;
				vMap.planes.at(0).at(y).at(x2) = AIR;
			}
			if(y >= yPos)
			{
				vMap.planes.at(0).at(y).at(x2) = SOLID;
				vMap.planes.at(0).at(y).at(x2+1) = SOLID;
			}
		}
		vMap.planes.at(0).at(yPos-2).at(x2+1) = DOOR2U;
		vMap.planes.at(0).at(yPos-1).at(x2+1) = DOOR2L;
		vMap.planes.at(0).at(yPos-1).at(x2) = AIR;
		vMap.planes.at(0).at(yPos-2).at(x2) = AIR;
		vMap.planes.at(0).at(yPos-4).at(x2) = KEY1;
		vMap.planes.at(1).at(y4-9).at(x1+1) = KEEN;
	}
	// place enemies on top part
	vector<int> gargs = {};
	int xGargRange = x3 - x2;
	gargs.push_back(x2 + (seed + addSeed) % xGargRange/3);
	gargs.push_back(gargs.at(0) + (seed + 2*addSeed) % xGargRange/3);
	gargs.push_back(gargs.at(1) + (seed + 3*addSeed) % xGargRange/3);
	for(int x : gargs)
	{
		for(int y = y2; y > y1 ; y--)
		{
			if(vMap.planes.at(0).at(y).at(x) == AIR)
			{
				vMap.planes.at(1).at(y-1).at(x) = GARG;
				break;
			}
		}
	}
	// Place hazards at bottom to prevent unescapable holes
	for(int x=x1; x < x2 ; x++)
	{
		if(vMap.planes.at(0).at(y2).at(x) == AIR)
		{
			vMap.planes.at(0).at(y2).at(x) == HAZARD;
		}
	}

	// corner top right
	for(int x = x3; x < x4; x++)
	{
		for(int y=y1; y<y2; y++)
		{
			vMap.planes.at(0).at(y).at(x) = AIR;
			if(x==x3 && y % 5 == 0)
			{
				if(vMap.planes.at(0).at(y).at(x-1) == SOLID)
				{
					vMap.planes.at(0).at(y).at(x) = JUMPTHROUGH;
				}
			}
		}
	}
	// bottom
	xSizes = {x3-x2+4, x3-x2+5};
	ySizes = {y4-y3+4, y4-y3+5};
	VorticonsMap vM_btm = createLinearHorizontalMap(xSizes, ySizes, false);
	//cout << "modeled the bottom \n\n";
	for(int x = x2; x < x3 ; x++)
	{
		int x_ = x-x2+2;
		for(int y = y3; y < y4; y++)
		{
			vMap.planes.at(0).at(y).at(x) = vM_btm.planes.at(0).at(y-y3+2).at(x_);
			int spriteValue =vM_btm.planes.at(1).at(y-y3+2).at(x_);
			vMap.planes.at(1).at(y).at(x) = spriteValue == 255 ? 0 : spriteValue;
		}
	}

	vector<int> gargs2 = {};
	gargs2.push_back(x2 + (seed + addSeed) % xGargRange/3);
	gargs2.push_back(gargs.at(0) + (seed + 2*addSeed) % xGargRange/3);
	gargs2.push_back(gargs.at(1) + (seed + 3*addSeed) % xGargRange/3);
	for(int x : gargs2)
	{
		for(int y = y4; y > y3 ; y--)
		{
			if(vMap.planes.at(0).at(y).at(x) == AIR)
			{
				vMap.planes.at(1).at(y-1).at(x) = GARG;
				break;
			}
		}
	}

	// left
	if(x2-x1 > 5)
	{
		xSizes = {x2-x1+4, x2-x1+5};
		ySizes = {y3-y2+5, y3-y2+6};
		VorticonsMap vM_left = createTopToBottomVerticalMap(xSizes, ySizes, false);
		//cout << "modeled the left side \n\n";
		for(int x = x1; x < x2; x++)
		{
			for(int y = y2 ; y < y3; y++)
			{
				//cout << "y: " << y << " x: " << x << "\n";
				vMap.planes.at(0).at(y).at(x) = vM_left.planes.at(0).at(y-y2+2).at(x);
			}
		}
	}
	else
	{
		for(int x = x1; x < x2; x++)
		{
			for(int y = y2; y < y3; y++)
			{
				vMap.planes.at(0).at(y).at(x) = AIR;
			}
		}
	}
	//cout << "implemented the left side \n";
	// right
	xSizes = {x4-x3+4, x4-x3+5};
	ySizes = {y4-y2+4, y4-y2+5};
	VorticonsMap vM_right = createBottomToTopVerticalMap(xSizes, ySizes, false);
	//cout << "modeled the right side \n\n";
	for(int x = x3; x < x4; x++)
	{
		int x_ = x-x3+2;
		for(int y = y2 ; y < y4; y++)
		{
			vMap.planes.at(0).at(y).at(x) = vM_right.planes.at(0).at(y-y2+2).at(x_);
			int spriteValue = vM_right.planes.at(1).at(y-y2+2).at(x_);
			vMap.planes.at(1).at(y).at(x) = spriteValue == 255 ? 0 : spriteValue;
		}
	}
	vMap.planes.at(0).at(y2).at(x3) = SOLID;

	// center
	for(int x=x2; x<x3; x++)
	{
		for(int y=y2; y<y3; y++)
		{
			vMap.planes.at(0).at(y).at(x) = SOLID;
		}
	}

	if(difficulty == 4)
	{
		xSizes = {x3-x2+7, x3-x2+6};
		int yStart;
		int yStop;
		if(y3-y2 > 15) 
		{
			ySizes = {y3-y2-5, y3-y2-6};
			yStart = y2+5;
			yStop = y3-6;
		}
		else
		{
			ySizes = {y3-y2-1, y3-y2-2};
			yStart = y2+2;
			yStop = y3-3;
		}
		VorticonsMap vM_centr = createLinearHorizontalMap(xSizes, ySizes, false);
		//cout << "modeled center map \n\n";
		for(int x = x2+7; x < x3 ; x++)
		{
			int x_ = x-x2-3;
			for(int y = yStart; y < yStop; y++)
			{
				vMap.planes.at(0).at(y).at(x) = vM_centr.planes.at(0).at(y-yStart+2).at(x_);
				int spriteValue = vM_centr.planes.at(1).at(y-yStart+2).at(x_);
				vMap.planes.at(1).at(y).at(x) = spriteValue == 255 ? 0 : spriteValue;
			}
		}
		int ypos_centr = yStop;
		for(int y = yStop; y > y2 ; y--)
		{
			if(vMap.planes.at(0).at(y).at(x2+8) == AIR)
				break;
			ypos_centr=y;
		}
		for(int y = yStart; y < yStop; y++)
		{
			if(y < ypos_centr)
			{
				vMap.planes.at(0).at(y).at(x2+5) = AIR;
				vMap.planes.at(0).at(y).at(x2+6) = AIR;
			}
			else
			{
				vMap.planes.at(0).at(y).at(x2+5) = SOLID;
				vMap.planes.at(0).at(y).at(x2+6) = SOLID;
			}
		}
		vMap.planes.at(0).at(ypos_centr-5).at(x2+5) = KEY2;
	}

	// bottom left
	for(int x=x1; x<x2; x++)
	{
		for(int y=y3; y<y4-2; y++)
		{
			vMap.planes.at(0).at(y).at(x) = AIR;
		}
		vMap.planes.at(0).at(y4-2).at(x) = SOLID;
		vMap.planes.at(0).at(y4-1).at(x) = SOLID;
	}

	// place wall around exit
	int ypos_btm = y4-3;
	for(int y = y4-3; y > y3 ; y--)
	{
		if(vMap.planes.at(0).at(y).at(x2+2) == AIR)
			break;
		ypos_btm=y-4;
	}
	ypos_btm = min(ypos_btm, y4-9);
	for(int x=x1; x<x1+7; x++)
	{
		for(int y=y4-3; y >= ypos_btm; y--)
		{
			vMap.planes[0].at(y).at(x) = AIR;
		}
		vMap.planes[0].at(y4-7).at(x) = SOLID;
	}
	vMap.planes[0].at(y4-7).at(x1+6) = AIR;
	// place exit tiles
	vMap.planes[0].at(y4-5).at(x1+1) = EXIT11;
	vMap.planes[0].at(y4-4).at(x1+1) = EXIT21;
	vMap.planes[0].at(y4-3).at(x1+1) = EXIT21;

	vMap.planes[0].at(y4-5).at(x1+2) = EXIT12;
	vMap.planes[0].at(y4-4).at(x1+2) = EXIT22;
	vMap.planes[0].at(y4-3).at(x1+2) = EXIT22;

	vMap.planes[0].at(y4-5).at(x1+3) = EXIT13;
	vMap.planes[0].at(y4-4).at(x1+3) = EXIT23;
	vMap.planes[0].at(y4-3).at(x1+3) = EXIT23;

	vMap.planes[0].at(y4-6).at(x1+5) = SOLID;
	vMap.planes[0].at(y4-5).at(x1+5) = SOLID;
	vMap.planes[0].at(y3+6).at(x3-1) = SOLID;
	vMap.planes[0].at(y4-4).at(x1+5) = DOOR1U;
	vMap.planes[0].at(y4-3).at(x1+5) = DOOR1L;
	return vMap;
}

tuple<int, bool> solidShape1(int top, int bottom, int left, int right, int diagNW, int diagNE, int diagSE, int diagSW)
{
	int tile;
	bool solid = true;
	//topleft corner
	if(top != SOLID && left != SOLID && bottom == SOLID && right == SOLID && diagSE == SOLID)
		tile = 182;
	//topmiddle
	else if(top != SOLID && left == SOLID && bottom == SOLID && right == SOLID && diagSE == SOLID && diagSW == SOLID)
		tile = 183;
	//topright corner
	else if(top != SOLID && left == SOLID && bottom == SOLID && right != SOLID && diagSW == SOLID)
		tile = 184;
	//leftmiddle
	else if(top == SOLID && left != SOLID && bottom == SOLID && right == SOLID && diagSE == SOLID && diagNE == SOLID)
		tile = 195;
	//center solid
	else if(top == SOLID && left == SOLID && bottom == SOLID && right == SOLID && diagNW == SOLID && diagSE == SOLID && diagSW == SOLID && diagNE == SOLID)
		tile = 196;
	//rightmiddle
	else if(top == SOLID && left == SOLID && bottom == SOLID && right != SOLID && diagNW == SOLID && diagSW == SOLID)
		tile = 197;
	//bottomleft
	else if(top == SOLID && left != SOLID && bottom != SOLID && right == SOLID && diagNE == SOLID)
		tile = 208;
	//bottommiddle
	else if(top == SOLID && left == SOLID && bottom != SOLID && right == SOLID && diagNE == SOLID && diagNW == SOLID)
		tile = 209;
	//bottomright
	else if(top == SOLID && left == SOLID && bottom != SOLID && right != SOLID && diagNW == SOLID)
		tile = 210;
	//Inside corners
	//bottomright
	else if(top == SOLID && left == SOLID && bottom == SOLID && right == SOLID && diagSE == SOLID && diagSW == SOLID && diagNE == SOLID)
		tile = 221;
	//bottomleft
	else if(top == SOLID && left == SOLID && bottom == SOLID && right == SOLID && diagNW == SOLID && diagSE == SOLID && diagSW == SOLID)
		tile = 222;
	//topright
	else if(top == SOLID && left == SOLID && bottom == SOLID && right == SOLID && diagNW == SOLID && diagSE == SOLID && diagNE == SOLID)
		tile = 234;
	//topleft
	else if(top == SOLID && left == SOLID && bottom == SOLID && right == SOLID && diagNW == SOLID && diagSW == SOLID && diagNE == SOLID)
		tile = 235;
	else 
	{
		tile = 211;
		solid = false;
	}
	return {tile, solid};
}

int solidShape2(int top, int bottom, int left, int right, int diagNW, int diagNE, int diagSE, int diagSW)
{
	int tile;
	// solid stops
	//left
	if(top != SOLID && left != SOLID && bottom != SOLID && right == SOLID)
		tile = 165;
	//right
	else if(top != SOLID && left == SOLID && bottom != SOLID && right != SOLID)
		tile = 167;
	//up
	else if(top != SOLID && left != SOLID && bottom == SOLID && right != SOLID)
		tile = 180;
	//down
	else if(top == SOLID && left != SOLID && bottom != SOLID && right != SOLID)
		tile = 193;
	//threeway corners
	//no up
	else if(top != SOLID && left == SOLID && bottom == SOLID && right == SOLID && (diagSW != SOLID || diagSE != SOLID))
		tile = 194;
	//no left
	else if(top == SOLID && left != SOLID && bottom == SOLID && right == SOLID && (diagSE != SOLID || diagNE != SOLID))
		tile = 155;
	//no down
	else if(top == SOLID && left == SOLID && bottom != SOLID && right == SOLID && (diagNE != SOLID || diagNW != SOLID))
		tile = 168;
	//no right
	else if(top == SOLID && left == SOLID && bottom == SOLID && right != SOLID && (diagNW != SOLID || diagSW != SOLID))
		tile = 181;
	//fourway
	else if(top == SOLID && left == SOLID && bottom == SOLID && right == SOLID && ((diagNW != SOLID && diagSE != SOLID) || (diagNE != SOLID && diagSW != SOLID)) )
		tile = 154;
	//middleHorizontal
	else if(left == SOLID && right == SOLID && (bottom != SOLID || top != SOLID))
		tile = 166;
	//middleVertical
	else if(top == SOLID && bottom == SOLID && (left != SOLID || right != SOLID))
		tile = 204;
	//topleft corner
	else if(bottom == SOLID && right == SOLID && ((diagSW != SOLID || diagSE != SOLID) && (diagSE != SOLID || diagNE != SOLID)) || (top != SOLID && left != SOLID) )
		tile = 178;
	//topright corner
	else if(left == SOLID && bottom == SOLID && ((diagSW != SOLID || diagSE != SOLID) && (diagNW != SOLID || diagSW != SOLID)) || ( top != SOLID && right != SOLID) )
		tile = 179;
	//bottomleft corner
	else if(top == SOLID && right == SOLID && ((diagNE != SOLID || diagNW != SOLID) && (diagSE != SOLID || diagNE != SOLID)) || (bottom != SOLID && left != SOLID))
		tile = 191;
	//bottomright corner
	else if(top == SOLID && left == SOLID && ((diagNE != SOLID || diagNW != SOLID) && (diagNW != SOLID || diagSW != SOLID)) || (bottom != SOLID && right != SOLID))
		tile = 192;
	else 
	{
		tile = 205;
	}
	return tile;
}

int backGround1(void)
{
	int randomDigit = get_next_pi_digit(pi);
	switch(randomDigit)
	{
		case 0: return 156;
		case 1: return 169;
		case 2: return 217;
		case 3: return 218;
		case 4: return 219;
		case 5: return 220;
		case 6: return 232;
		case 7: return 233;
		case 8: return 245;
		case 9: return 258;
		default: return 156;
	}
}

VorticonsMap marsLevel(VorticonsMap* vMap)
{
	vector<vector<uint16_t>> tileMap = vMap->planes[0];
	int height = vMap->h-2;
	int width = vMap->w-2;
	tileMap.at(keen.y+2).at(keen.x) = SOLID;
	tileMap.at(keen.y+1).at(keen.x) = AIR;
	tileMap.at(keen.y).at(keen.x) = AIR;
	VorticonsMap newMap = generateBlankLevel(width+2,width+2,height+2,height+2);
	newMap.planes[1] = vMap->planes[1];
	vector<tuple<int, int>> ornaments = {};
	vector<tuple<int, int, int>> support = {};
	for(int x = 2; x < width; x++)
	{
		for(int y = 2; y < height; y++)
		{
			int top = tileMap.at(y-1).at(x);
			int bottom = tileMap.at(y+1).at(x);
			int left = tileMap.at(y).at(x-1);
			int right = tileMap.at(y).at(x+1);
			int tileValue = tileMap.at(y).at(x);
			int diagNW = tileMap.at(y-1).at(x-1);
			int diagNE = tileMap.at(y-1).at(x+1);
			int diagSE = tileMap.at(y+1).at(x+1);
			int diagSW = tileMap.at(y+1).at(x-1);
			if(tileValue == SOLID)
			{
				tuple<int, bool> tileData = solidShape1(top==BORDER?196:top,bottom==BORDER?196:bottom,left==BORDER?196:left,right==BORDER?196:right,diagNW==BORDER?196:diagNW,diagNE==BORDER?196:diagNE,diagSE==BORDER?196:diagSE,diagSW==BORDER?196:diagSW);
				newMap.planes[0].at(y).at(x) = get<0>(tileData);
				//if(!get<1>(tileData))
				//	tileMap.at(y).at(x) = OCCUPIED;
			}
			else if(tileValue == HAZARD)
			{
				if((left == HAZARD || left == SOLID) && (right == SOLID || right == HAZARD) && top != HAZARD && (bottom == HAZARD || bottom == BORDER)) 
				{
					newMap.planes[0].at(y).at(x) = 429 + (x+y) % 4;
					if(left == SOLID && tileMap.at(y).at(x-2) == SOLID)
						support.push_back({303, y, x-1});
					else if(right == SOLID && tileMap.at(y).at(x+2) == SOLID)
						support.push_back({304, y, x+1});
				}
				else if((left == HAZARD || left == SOLID) && (right == SOLID || right == HAZARD) && top == HAZARD && (diagNW == HAZARD || diagNW == SOLID) && (diagNE == HAZARD || diagNE == SOLID))
				{
					newMap.planes[0].at(y).at(x) = 439;
					if(left == SOLID && tileMap.at(y).at(x-2) == SOLID)
						support.push_back({290, y, x-1});
					else if(right == SOLID && tileMap.at(y).at(x+2) == SOLID)
						support.push_back({291, y, x+1});
				}
				else if(bottom != SOLID)
					newMap.planes[0].at(y).at(x) = 223;
				else if(bottom == SOLID)
					newMap.planes[0].at(y).at(x) = 250;
			}
			else if(tileValue == JUMPTHROUGH)
			{
				if((top != JUMPTHROUGH && top != SOLID && top != OCCUPIED) && (bottom != JUMPTHROUGH && bottom != SOLID) && (left == JUMPTHROUGH || right == JUMPTHROUGH))
				{
					int diagNW = tileMap.at(y-1).at(x-1);
					int diagNE = tileMap.at(y-1).at(x+1);
					int diagSE = tileMap.at(y+1).at(x+1);
					int diagSW = tileMap.at(y+1).at(x-1);
					if((left == JUMPTHROUGH && right == JUMPTHROUGH) || (left == SOLID && right == JUMPTHROUGH) || (left == JUMPTHROUGH && right == SOLID) && diagNW != SOLID && diagNW != JUMPTHROUGH && diagNW != OCCUPIED && diagNE != SOLID && diagNE != OCCUPIED && diagNE != JUMPTHROUGH && diagSW != SOLID && diagSW != JUMPTHROUGH&& diagSE != SOLID && diagSE != JUMPTHROUGH)
					{
						newMap.planes[0].at(y).at(x) = 241;
					}
					else if(left == JUMPTHROUGH && diagNW != SOLID && diagNW != JUMPTHROUGH && diagNW != OCCUPIED && diagSW != SOLID && diagSW != JUMPTHROUGH)
					{
						bool check = true;
						int yCheck = y+1;
						while(tileMap.at(yCheck).at(x) == AIR)
						{
							yCheck++;
							if(tileMap.at(yCheck).at(x) != SOLID && tileMap.at(yCheck).at(x) != BORDER && tileMap.at(yCheck).at(x) != AIR && tileMap.at(yCheck).at(x) != JUMPTHROUGH)
							{
								check = false;
								break;
							}
						}
						if(check == true)
						{
							newMap.planes[0].at(y).at(x) = 242;
							support.push_back({254, y + 1, x});
							for(int i = y + 2; i < yCheck; i++)
							{
								support.push_back({255, i, x});
							}
						}
						else
						{
							newMap.planes[0].at(y).at(x) = 244;
						}
					}
					else if(right == JUMPTHROUGH && diagNE != SOLID && diagNE != JUMPTHROUGH && diagNE != OCCUPIED && diagSE != SOLID && diagSE != JUMPTHROUGH)
					{
						bool check = true;
						int yCheck = y+1;
						while(tileMap.at(yCheck).at(x) == AIR)
						{
							yCheck++;
							if(tileMap.at(yCheck).at(x) != SOLID && tileMap.at(yCheck).at(x) != BORDER && tileMap.at(yCheck).at(x) != AIR && tileMap.at(yCheck).at(x) != JUMPTHROUGH)
							{
								check = false;
								break;
							}
						}
						if(check == true)
						{
							newMap.planes[0].at(y).at(x) = 240;
							support.push_back({254, y + 1, x});
							for(int i = y + 2; i < yCheck; i++)
							{
								support.push_back({255, i, x});
							}
						}
						else
						{
							newMap.planes[0].at(y).at(x) = 243;
						}
					}
					else
					{
					newMap.planes[0].at(y).at(x) = 185;
					}
				}
				else
				{
					newMap.planes[0].at(y).at(x) = 185;
				}
			}
			else if(tileValue == AIR && tileMap.at(y+1).at(x) == SOLID)
			{
				if ((seed + x + y) % 15 == 3)
				{
					newMap.planes[0].at(y).at(x) = 201;
				}
				else if ((seed + 2*x + y) % 15 == 5)
				{
					newMap.planes[0].at(y).at(x) = 227;
				}
				else
				{
					newMap.planes[0].at(y).at(x) = tileMap.at(y).at(x);
					if(tileMap.at(y-1).at(x) == AIR && tileMap.at(y-1).at(x+1) == AIR && tileMap.at(y-1).at(x+2) == AIR && tileMap.at(y).at(x+1) == AIR && tileMap.at(y).at(x+2) == AIR && tileMap.at(y+1).at(x+2) == SOLID && tileMap.at(y+1).at(x+1) == SOLID)
					{
						ornaments.push_back({y,x});
					}
				}
			}
			else if(tileValue >= POINT1 && tileValue <= POINT5)
			{
				if (get_next_pi_digit(pi) % 10 == 0 && tileValue == POINT1) 
				{
					newMap.planes[0].at(y).at(x) = AMMO;
					int diagNE = tileMap.at(y-1).at(x+1);
					if( diagNE != SOLID && diagNE != HAZARD && diagNE != JUMPTHROUGH && top != SOLID && top != HAZARD && top != JUMPTHROUGH)
					{
						newMap.planes[0].at(y-1).at(x) = 206;
						tileMap.at(y-1).at(x+1) = 207;
					}
				}
				else
				{
					if((x + 2*y)%3 == 0) tileValue++;
					newMap.planes[0].at(y).at(x) = tileValue;
				}
			}
			else
			{
				newMap.planes[0].at(y).at(x) = tileMap.at(y).at(x);
			}
		}
	}
	tuple<int, int> ornamentPlace = ornaments[seed % ornaments.size()];
	int x = get<1>(ornamentPlace);
	int y = get<0>(ornamentPlace);
	newMap.planes[0].at(y).at(x+1) = 202;
	newMap.planes[0].at(y).at(x+2) = 203;
	newMap.planes[0].at(y-1).at(x+1) = 189;
	newMap.planes[0].at(y-1).at(x+2) = 190;
	for(tuple<int, int, int> supportTile: support)
	{
		newMap.planes[0].at(get<1>(supportTile)).at(get<2>(supportTile)) = get<0>(supportTile);
	}
	return newMap;
}

VorticonsMap spaceLevel(VorticonsMap* vMap)
{
	vector<vector<uint16_t>> tileMap = vMap->planes[0];
	int height = vMap->h-2;
	int width = vMap->w-2;
	tileMap.at(keen.y+2).at(keen.x) = SOLID;
	tileMap.at(keen.y+1).at(keen.x) = AIR;
	tileMap.at(keen.y).at(keen.x) = AIR;
	// no 3x3 solid blocks:
	vector<tuple<int, int>> blocks = {};
	vector<tuple<int, int, int>> columns = {};
	for(int y=2; y < height;y++)
	{
		for(int x=2; x < width;x++)
		{
			if(tileMap.at(y).at(x) == SOLID)
			{
				if(tileMap.at(y-1).at(x) == SOLID)
				if(tileMap.at(y+1).at(x) == SOLID)
				if(tileMap.at(y).at(x-1) == SOLID)
				if(tileMap.at(y).at(x+1) == SOLID)
				if(tileMap.at(y-1).at(x-1) == SOLID)
				if(tileMap.at(y-1).at(x+1) == SOLID)
				if(tileMap.at(y+1).at(x+1) == SOLID)
				if(tileMap.at(y+1).at(x-1) == SOLID)
				{
					blocks.push_back({y,x});
				}
			}
		}
	}
	for(tuple<int, int> coord: blocks)
	{
		tileMap.at(get<0>(coord)).at(get<1>(coord)) = AIR;
	} 
	tileMap.at(keen.y+2).at(keen.x) = SOLID;
	VorticonsMap newMap = generateBlankLevel(width+2,width+2,height+2,height+2);
	newMap.planes[1] = vMap->planes[1];
	for(int x = 1; x < width+1; x++)
	{
		for(int y = 1; y < height+1; y++)
		{
			int top = tileMap.at(y-1).at(x);
			int bottom = tileMap.at(y+1).at(x);
			int left = tileMap.at(y).at(x-1);
			int right = tileMap.at(y).at(x+1);
			int tileValue = tileMap.at(y).at(x);
			if(tileValue == SOLID)
			{
				int diagNW = tileMap.at(y-1).at(x-1);
				int diagNE = tileMap.at(y-1).at(x+1);
				int diagSE = tileMap.at(y+1).at(x+1);
				int diagSW = tileMap.at(y+1).at(x-1);
				newMap.planes[0].at(y).at(x) = solidShape2(top,bottom,left,right,diagNW,diagNE,diagSE,diagSW);
				bool check = true;
				int yCheck = y+1;
				while(tileMap.at(yCheck).at(x) == AIR)
				{
					yCheck++;
					if(tileMap.at(yCheck).at(x) != SOLID && tileMap.at(yCheck).at(x) != BORDER && tileMap.at(yCheck).at(x) != AIR)
					{
						check = false;
						break;
					}
				}
				if(check && yCheck - y > 3 && get_next_pi_digit(pi) % 5 == 0 && x % 2 ==0)
				{
					columns.push_back({x,y+1, yCheck});
				}
			}
			else if(tileValue == HAZARD)
			{
				if(bottom != SOLID)
					newMap.planes[0].at(y).at(x) = 279;
				else
					newMap.planes[0].at(y).at(x) = 266;
			}
			else if(tileValue == JUMPTHROUGH)
			{
				newMap.planes[0].at(y).at(x) = 212; //188;
			}
			else if(tileValue == AIR)
			{
				int backgroundTile = backGround1();
				if(backgroundTile == 217 && top == AIR && left == AIR && tileMap.at(y-1).at(x-1) == AIR && get_next_pi_digit(pi) % 2 == 0)
				{
					newMap.planes[0].at(y-1).at(x-1) = 263;
					newMap.planes[0].at(y-1).at(x) = 264;
					newMap.planes[0].at(y).at(x-1) = 276;
					newMap.planes[0].at(y).at(x) = 277;
					tileMap.at(y-1).at(x-1) = 263;
					tileMap.at(y-1).at(x) = 264;
					tileMap.at(y).at(x-1) = 276;
					tileMap.at(y).at(x) = 277;
				}
				else if(backgroundTile == 218 && top == AIR && left == AIR && tileMap.at(y-1).at(x-1) == AIR && get_next_pi_digit(pi) % 2 == 0)
				{
					newMap.planes[0].at(y-1).at(x-1) = 247;
					newMap.planes[0].at(y-1).at(x) = 248;
					newMap.planes[0].at(y).at(x-1) = 260;
					newMap.planes[0].at(y).at(x) = 261;
					tileMap.at(y-1).at(x-1) = 247;
					tileMap.at(y-1).at(x) = 248;
					tileMap.at(y).at(x-1) = 260;
					tileMap.at(y).at(x) = 261;
				}
				else newMap.planes[0].at(y).at(x) = backgroundTile;
			}
			else if(tileValue >= POINT1 && tileValue <= POINT5)
			{
				int i = get_next_pi_digit(pi) % 2 + 1;
				if (get_next_pi_digit(pi) % 10 == 0 && tileValue == POINT1) 
				{
					newMap.planes[0].at(y).at(x) = i*13 + AMMO;
					int diagNE = tileMap.at(y-1).at(x+1);
					if( diagNE != SOLID && diagNE != HAZARD && diagNE != JUMPTHROUGH && top != SOLID && top != HAZARD && top != JUMPTHROUGH)
					{
						newMap.planes[0].at(y-1).at(x) = 206;
						tileMap.at(y-1).at(x+1) = 207;
					}
				}
				else
				{
					if((x + 2*y)%3 == 0) tileValue++;
					newMap.planes[0].at(y).at(x) = tileValue + 13*i;
				}
			}
			else if(tileValue == EXIT11) newMap.planes[0].at(y).at(x) = 329;
			else if(tileValue == EXIT12) newMap.planes[0].at(y).at(x) = 316;
			else if(tileValue == EXIT13) newMap.planes[0].at(y).at(x) = 315;
			else if(tileValue == EXIT23) newMap.planes[0].at(y).at(x) = 317 + 13*(y%2);
			else
			{
				newMap.planes[0].at(y).at(x) = tileMap.at(y).at(x);
			}
		}
	}
	for(tuple<int, int, int> column: columns)
	{
		int x = get<0>(column);
		int yStart = get<1>(column);
		int yEnd = get<2>(column);
		newMap.planes[0].at(yStart).at(x) = 246;
		newMap.planes[0].at(yStart+1).at(x) = 259;
		for(int i= yStart+2; i<yEnd; i++)
		{
			newMap.planes[0].at(i).at(x) = 272;
		}
	}
	return newMap;
}
VorticonsMap iceLevel(VorticonsMap* vMap)
{
	vector<vector<uint16_t>> tileMap = vMap->planes[0];
	int height = vMap->h-2;
	int width = vMap->w-2;
	VorticonsMap newMap = generateBlankLevel(width+2,width+2,height+2,height+2);
	newMap.planes[1] = vMap->planes[1];
	tileMap.at(keen.y+2).at(keen.x) = SOLID;
	tileMap.at(keen.y+1).at(keen.x) = AIR;
	tileMap.at(keen.y).at(keen.x) = AIR;
	vector<tuple<int, int>> ornaments = {};
	for(int x = 2; x < width; x++)
	{
		for(int y = 2; y < height; y++)
		{
			int top = tileMap.at(y-1).at(x);
			int bottom = tileMap.at(y+1).at(x);
			int left = tileMap.at(y).at(x-1);
			int right = tileMap.at(y).at(x+1);
			int tileValue = tileMap.at(y).at(x);
			if(tileValue == SOLID)
			{
				int diagNW = tileMap.at(y-1).at(x-1);
				int diagNE = tileMap.at(y-1).at(x+1);
				int diagSE = tileMap.at(y+1).at(x+1);
				int diagSW = tileMap.at(y+1).at(x-1);
				tuple<int, bool> tileData = solidShape1(top==BORDER?196:top,bottom==BORDER?196:bottom,left==BORDER?196:left,right==BORDER?196:right,diagNW==BORDER?196:diagNW,diagNE==BORDER?196:diagNE,diagSE==BORDER?196:diagSE,diagSW==BORDER?196:diagSW);
				if(get<0>(tileData) == 211) newMap.planes[0].at(y).at(x) = 278;
				else newMap.planes[0].at(y).at(x) = get<0>(tileData) + 110;
			}
			else if(tileValue == JUMPTHROUGH)
			{
				newMap.planes[0].at(y).at(x) = 274;
			}
			else if(tileValue == HAZARD)
			{
				if((left == HAZARD || left == SOLID) && (right == SOLID || right == HAZARD) && top != HAZARD && (bottom == HAZARD || bottom == BORDER)) 
					newMap.planes[0].at(y).at(x) = 429 + (x+y) % 4;
				else if((left == HAZARD || left == SOLID) && (right == SOLID || right == HAZARD) && top == HAZARD)
					newMap.planes[0].at(y).at(x) = 439;
				else if(bottom == SOLID && top != SOLID)
					newMap.planes[0].at(y).at(x) = 273;
				else if(bottom != SOLID && top == SOLID)
					newMap.planes[0].at(y).at(x) = 275;
				else
					newMap.planes[0].at(y).at(x) = 420;
			}
			else if(tileValue == AIR)
			{
				newMap.planes[0].at(y).at(x) = 390;
				if(top == AIR && bottom == SOLID && right == AIR && tileMap.at(y+1).at(x+1) == SOLID)
				if(get_next_pi_digit(pi) % 10 == 0)
					ornaments.push_back({x,y});
			}
			else if(tileValue >= POINT1 && tileValue <= POINT5)
			{
				if (get_next_pi_digit(pi) % 10 == 0 && tileValue == POINT1) newMap.planes[0].at(y).at(x) = 396;
				else 
				{
					if((x + 2*y)%3 == 0) tileValue++;
					newMap.planes[0].at(y).at(x) = tileValue + 247;
				}
			}
			else if(level == 10)
			{
				if(tileValue == EXIT11) newMap.planes[0].at(y).at(x) = 390;
				else if(tileValue == EXIT12) 
				{
					newMap.planes[0].at(y).at(x) = 390;
					newMap.planes[1].at(y).at(x) = 13; // The Mangling heart to win the game
				}
				else if(tileValue == EXIT13) newMap.planes[0].at(y).at(x) = 390;
				else if(tileValue == EXIT21) newMap.planes[0].at(y).at(x) = AMMO + 247;
				else if(tileValue == EXIT22) newMap.planes[0].at(y).at(x) = 274;
				else if(tileValue == EXIT23) newMap.planes[0].at(y).at(x) = 390;
				else
				{
					newMap.planes[0].at(y).at(x) = tileValue;
				}
			}
			else if(tileValue == EXIT11) newMap.planes[0].at(y).at(x) = 349;
			else if(tileValue == EXIT12) newMap.planes[0].at(y).at(x) = 337;
			else if(tileValue == EXIT13) newMap.planes[0].at(y).at(x) = 336;
			else if(tileValue == EXIT23) newMap.planes[0].at(y).at(x) = 350;
			else
			{
				newMap.planes[0].at(y).at(x) = tileValue;
			}
		}
	}
	for(tuple<int, int> coord: ornaments)
	{
		int x = get<0>(coord);
		int y = get<1>(coord);
		if((x+y)%2==0)
		{
			newMap.planes[0].at(y).at(x) = 310;
			newMap.planes[0].at(y-1).at(x) =  297;
		}
		else
		{
			newMap.planes[0].at(y).at(x) = 295;
			newMap.planes[0].at(y).at(x+1) =  308;
		}
	}
	for(tuple<int, int> coord : armLocation)
	{
		int yStart = get<1>(coord);
		int xloc = get<0>(coord);
		int tile = tileMap.at(yStart).at(xloc);
		while(tile != SOLID && tile != BORDER && tile != JUMPTHROUGH)
		{
			yStart++;
			tile = tileMap.at(yStart).at(xloc);
		}
		//cout << "bottom arm tile: x: " << xloc << " y: " << yStart << "\n";
		if(tile == SOLID) newMap.planes[0].at(yStart).at(xloc) = ARMBOTTOM;
		else 
		{
			newMap.planes[0].at(yStart-1).at(xloc-1) = ARMBOTTOM;
			newMap.planes[0].at(yStart-1).at(xloc) = ARMBOTTOM;
			newMap.planes[0].at(yStart-1).at(xloc+1) = ARMBOTTOM;
		}
	}
	armLocation = {};
	return newMap;
}

VorticonsMap magicTooling(VorticonsMap* vMap)
{
	if(difficulty < 3)
		return marsLevel(vMap);
	else if(difficulty == 3)
	{
		return spaceLevel(vMap);
	}
	else
	{
		return iceLevel(vMap);
	}
}

int main()
{
	store_pi_digits(pi);
	srand((unsigned)time(0));
	seed = rand() % 10000;
	cout << "seed: " << seed << "\n";
	for(level = 1; level < 11; level++)
	{
		addSeed = 0;
		//ofstream out_file;
		//out_file.open("seed.txt");
		//out_file << seed << endl;
		//out_file.close();
		vector<function<VorticonsMap(tuple<int, int>, tuple<int, int>, bool)>> mapCreators = {createLinearHorizontalMap};
		VorticonsMap vm;
		if(difficulty == 1)
		{
			vm = createLinearHorizontalMap();
		}
		else if(difficulty == 3)
		{
			vm = createLinearHorizontalMap({55,90},{20,30});
		}
		else if(difficulty == 2 || difficulty == 4)
		{
			mapCreators.push_back(createLinearHorizontalMap);
			mapCreators.push_back(createLinearHorizontalMap);
			mapCreators.push_back(createZigZagVerticalMap);
			mapCreators.push_back(createTopToBottomVerticalMap);
			mapCreators.push_back(createBottomToTopVerticalMap);
			mapCreators.push_back(createBigLoopMap);
			mapCreators.push_back(createDoubleLayeredHorizontalMap);
			tuple<int, int> xRange, yRange;
			int i = seed % mapCreators.size();
			if(i > 2)
			{
				if(i < 6)
				{
					xRange = {25, 40};
					yRange = {35, 60};
				}
				else
				{
					xRange = {40, 60};
					yRange = {40, 60};
				}
			}
			else
			{
				if(difficulty == 2)
				{
					xRange = {40, 70};
					yRange = {25, 35};
				}
				else
				{
					xRange = {50, 85};
					yRange = {30, 40};
				}
			}
			vm = mapCreators.at(i)(xRange, yRange, true);
		}
		VorticonsMap newLevel = magicTooling(&vm);
		string levelNameStart = "Level";
		string levelName;
		if (level < 10)
		{
			levelName = levelNameStart + "0" + to_string(level) + ".CK3";
		}
		else 
		{
			levelName = levelNameStart + "16.CK3";
		}
		FILE *g = fopen(levelName.c_str(), "wb");
		VORT_SaveMap(&newLevel, g);
		fclose(g);
		if (level % 2 == 0 && level < 8)
		{
			difficulty++;
		}
		seed++;
	}
  	return 0;
}
