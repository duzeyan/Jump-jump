#ifndef _TOOLS_H_
#define _TOOLS_H_

#include<opencv2\opencv.hpp>
using namespace cv;

namespace jumptools {

//��ȡ����ͼ��
void GetScreen(Mat& img);

//ͼ��Ԥ����
void PreProcess(Mat& srcimg,Mat& outImg);

//������Ļ
//second ��λ����
void PushScreen(int second);

//��X����YΪ���ȶԽڵ�������� 1
//type=1  ����˵�
//type=2  ���϶˵�
//type=3  ���Ҷ˵�
void sortPointFirstXY(vector<Point2i> &v, int type);

float GetDistance(Point2i a, Point2i b);

}

#endif // !_TOOLS_H_

