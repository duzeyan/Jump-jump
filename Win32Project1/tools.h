#ifndef _TOOLS_H_
#define _TOOLS_H_

#include<opencv2\opencv.hpp>
using namespace cv;

namespace jumptools {

//获取截屏图像
void GetScreen(Mat& img);

//图像预处理
void PreProcess(Mat& srcimg,Mat& outImg);

//长按屏幕
//second 单位毫秒
void PushScreen(int second);

//以X或者Y为优先对节点进行排序 1
//type=1  求左端点
//type=2  求上端点
//type=3  求右端点
void sortPointFirstXY(vector<Point2i> &v, int type);

float GetDistance(Point2i a, Point2i b);

}

#endif // !_TOOLS_H_

