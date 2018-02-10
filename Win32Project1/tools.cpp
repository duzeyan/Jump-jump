#include"tools.h"
#include<string.h>

#define SAVE_PATH_PC "F:\\tmpData\\jumpScreenCap.png"  //PC上临时存储地址
#define SAVE_PATH_PHONE "/sdcard/jumpScreenCap.png"   //安卓手机中临时存储地址
#define CMD_SAVE   "adb shell screencap -p "
#define CMD_PULL   "adb pull "
#define CMD_PUSH   "adb pull "

namespace jumptools {


void  GetScreen(Mat &img) {
	char cmdstr[255];
	//1.get screen img
	sprintf(cmdstr, "adb shell screencap -p %s", SAVE_PATH_PHONE);
	system(cmdstr);
	//2.pull img from phone
	sprintf(cmdstr, "adb pull %s %s", SAVE_PATH_PHONE, SAVE_PATH_PC);
	system(cmdstr);
	//3.read img
	img = imread(SAVE_PATH_PC);
}

void PreProcess(Mat & srcimg, Mat & cutImg)
{
	const int scale = 4;
	Mat outImg;
	//1.scale img
	resize(srcimg, outImg, Size(srcimg.cols/scale, srcimg.rows/scale));
	//2.cut img
	Rect ROI(0,outImg.rows/4,outImg.cols, outImg.rows / 2);
	cutImg=outImg(ROI);
}


void PushScreen(int second)
{
	char cmdstr[255];
	int touchX = rand() % 80 + 222;
	int touchY = rand() % 85 + 333; 
	sprintf(cmdstr, "adb shell input swipe %d %d %d %d %d", touchX, touchY, touchX+1, touchY+1,second);
	system(cmdstr);
}

void sortPointFirstXY(vector<Point2i> &v, int type)
{
	if (type==1) {
		sort(v, [](Point2i& a, Point2i& b){
			if (a.x != b.x) {
				return a.x < b.x;
			}
			else {
				return a.y < b.y;
			}
		});
	}
	else if(type==2){
		sort(v, [](Point2i& a, Point2i& b) {
			if (a.y != b.y) {
				return a.y < b.y;
			}
			else {
				return a.x < b.x;
			}
		});
	}else {
		sort(v, [](Point2i& a, Point2i& b) {
			if (a.x != b.x) {
				return a.x < b.x;
			}
			else {
				return a.y > b.y;
			}
		});
	}
}

float GetDistance(Point2i a, Point2i b)
{
	auto dis = (a.x - b.x)*(a.x - b.x)*1.0 + (a.y - b.y)*(a.y - b.y)*1.0;
	return sqrtf(dis);
}



}//namespace end
