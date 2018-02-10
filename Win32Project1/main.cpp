#include<opencv2\opencv.hpp>
#include"tools.h"
#include<stdlib.h>
#include<Windows.h>
#include<time.h>


#define JUMP_DEBUG       //注释掉则关闭所有写文件操作
#define PURPLE_THRES 40  //提取紫色棋子阈值
#define BG_THRES 30  //背景颜色阈值
#define BG_THRES_SHADOW 40  //背景阴影颜色阈值
#define TIME_SCALE_NEAR 6L  //按压系数
#define TIME_SCALE_FAR 5.8L  //按压系数
#define BG_TOP_OFFSET 35 //提取背景色偏移量
int g_frame = 1;

//获取棋子的中心位置
//posTop 顶端位置
//返回底部位置
Point2i GetChessCenterPos(Mat img,Point2i &topPos) {
	int x, y;
	int fx, fy,cnt,maxy,miny;//统计中心
	fx = fy = cnt=maxy=0;
	miny = img.rows;
	//1.根据紫色筛选棋子
	for (y = 0; y < img.rows; y++) {
		Vec3b *ptr = img.ptr<Vec3b>(y);
		for (x = 0; x < img.cols; x++) {
			//满足以下四个条件为紫色
			if ((*ptr)[0] > (*ptr)[1] &&
				(*ptr)[0] > (*ptr)[2]&& //1. B分量最大
				((*ptr)[0]+ (*ptr)[1]+ (*ptr)[2])/3<100&& //2. 灰度值较小
				(*ptr)[0]-min((*ptr)[1], (*ptr)[2])>10&&//3. B分量和其他分量有差距
				abs((*ptr)[1]-(*ptr)[2])<40){//4 .R、G分量差异不大 (差过大偏蓝色)
					(*ptr)[0] = (*ptr)[1] = (*ptr)[2] = 255;
					fx += x;
					fy += y;
					cnt++;
					maxy = max(maxy, y);
					miny = min(miny,y);
			}
			else {
				(*ptr)[0] = (*ptr)[1] = (*ptr)[2] = 0;
			}
			ptr++;
		}
	}
	//2.形心
	if (cnt == 0) {
		printf("error cnt=0");
		exit(0);
	}
	fx = fx / (cnt-2);//修正右边空洞带来的偏移
	fy = fy / cnt;
	int dy = (maxy - fy)/7*6;//向下修正一定比例的长度
	fy += dy;
	(*(img.ptr<Vec3b>(fy) + fx))[0] = (*(img.ptr<Vec3b>(fy) + fx))[1] = (*(img.ptr<Vec3b>(fy) + fx))[2] = 0;
#ifdef JUMP_DEBUG
	//imshow("chess", img);
	char filename[255];
	sprintf(filename, "debugIMG/1chess-%d.png", g_frame);
	imwrite(filename,img);
#endif 
	printf("chess pos (%d,%d)\n",fx,fy);
	//3.取背景点 
	topPos.x = fx;
	topPos.y = miny - BG_TOP_OFFSET;
	return Point2i(fx,fy);
}



//获取盒子的中心位置
Point2i GetBoxCenterPos(Mat img,Vec3b bgColor,Point2i posChess) {
	int x, y,dc1,dc2,halfW;
	uchar tmp;
	//1.计算阴影颜色
	Vec3b shadow = bgColor;
	tmp=min(bgColor[0], bgColor[1]);
	tmp= min(tmp, bgColor[2]);
	if (tmp == bgColor[0]) {
		shadow[0] -= 60;
		shadow[1] -= 70;
		shadow[2] -= 70;
	}
	if (tmp == bgColor[1]) {
		shadow[0] -= 70;
		shadow[1] -= 60;
		shadow[2] -= 70;
	}
	if (tmp == bgColor[2]) {
		shadow[0] -= 70;
		shadow[1] -= 70;
		shadow[2] -= 60;
	}
	//2.去背景，保留目标
	halfW = img.cols/ 2;
	for (y = 0; y < img.rows; y++) {
		Vec3b *ptr = img.ptr<Vec3b>(y);
		for (x = 0; x < img.cols; x++) {
			//棋子以下直接置为背景
			if (y > posChess.y) {
				(*ptr)[0] = (*ptr)[1] = (*ptr)[2] = 0;
				ptr++;
				continue;
			}
			if (posChess.x < halfW) {
				if (x < posChess.x + 10) {
					(*ptr)[0] = (*ptr)[1] = (*ptr)[2] = 0;
					ptr++;
					continue;
				}
			}
			else {
				if (x > posChess.x - 10) {
					(*ptr)[0] = (*ptr)[1] = (*ptr)[2] = 0;
					ptr++;
					continue;
				}
			}
			//棋子以下直接置为背景
			dc1 =dc2= 0;
			for (int c = 0; c < 3; c++) {
				dc1 += abs(bgColor[c] - (*ptr)[c]);
				dc2 += abs(shadow[c] - (*ptr)[c]);
			}
			if (dc1 < BG_THRES||dc2<BG_THRES_SHADOW) {
				(*ptr)[0] = (*ptr)[1] = (*ptr)[2] = 0;
			}
			else {
				(*ptr)[0] = (*ptr)[1] = (*ptr)[2] = 255;

			}
			ptr++;
		}
	}
	Mat erodeImg;
	Mat element = getStructuringElement(MORPH_RECT,
		Size(5, 5));
	erode(img, erodeImg, element);
#ifdef JUMP_DEBUG
	//imshow("remove bg", erodeImg);
	char filename[255];
	sprintf(filename, "debugIMG/2removeBG-%d.png", g_frame);
	imwrite(filename, erodeImg);
#endif
	//3.统计前景点
	vector<Point2i> vTarget;
	if (posChess.x < erodeImg.cols / 2) { //目标在右侧
		for (y = 0; y < posChess.y; y++) {
			Vec3b *ptr = erodeImg.ptr<Vec3b>(y)+int(posChess.x);
			for (x = posChess.x; x < erodeImg.cols; x++) {
				if ((*ptr)[0] == 255) {
					vTarget.push_back(Point(x,y));
				}
				ptr++;
			}
		}
	}else {
		for (y = 0; y < posChess.y; y++) {
			Vec3b *ptr = erodeImg.ptr<Vec3b>(y);
			for (x =0; x < posChess.x; x++) {
				if ((*ptr)[0] == 255) {
					vTarget.push_back(Point(x, y));
				}
				ptr++;
			}
		}
	}
	//4.寻找端点
	//jumptools::sortPointFirstXY(vTarget, 2); //
	auto top = vTarget[0];
	//修正X坐标
	int xx = top.x;
	int cnt = 1;
	for (auto &p : vTarget) {
		if (p.y == top.y) {
			xx += p.x;
			cnt++;
		}
	}
	top.x = xx / cnt;
	Point2i leftright;
	if (posChess.x < erodeImg.cols / 2) {//目标在右
		jumptools::sortPointFirstXY(vTarget, 3);
		leftright = vTarget.back();
		//后验 解决阴影没去干净带来的影响
		for (int i = vTarget.size() - 1; i>=0;i--) {
			if (vTarget[i].y - top.y   < 30) {
				leftright = vTarget[i];
				break;
			}
		}
	}
	else {//目标在左边
		jumptools::sortPointFirstXY(vTarget, 1);
		//后验 解决阴影没去干净带来的影响
		for (auto &p : vTarget) {
			if (p.y-top.y   < 30) {
				leftright = p;
				break;
			}
		}
	}
	return Point2i(top.x,leftright.y);
}

int main() {
	Mat srcImg,preImg;
	Point2i topPos;
	char filename[255];
	g_frame = 1;
	while (1) {
		
		printf("=============Frame %d==============\n",g_frame);
		//1.获取截屏数据
		jumptools::GetScreen(srcImg);
		clock_t t1 = clock();
		//srcImg=imread("debugIMG/case5.png");
#ifdef JUMP_DEBUG
		sprintf(filename, "debugIMG/0srcImg-%d.png", g_frame);
		imwrite(filename, srcImg);
#endif
		//2.预处理
		jumptools::PreProcess(srcImg, preImg);
		//3.获取棋子底部位置
		auto posChess= GetChessCenterPos(preImg.clone(),topPos);
		//4.获取盒子顶部中心位置
		auto posTarget=GetBoxCenterPos(preImg.clone(), preImg.at<Vec3b>(topPos.y, topPos.x), posChess);
#ifdef JUMP_DEBUG
		circle(preImg, posChess, 4, Scalar(0, 0, 255));
		circle(preImg, posTarget, 4, Scalar(0, 0, 255));
		sprintf(filename, "debugIMG/3result-%d.png",g_frame );
		imwrite(filename, preImg);
#endif
		//5.计算像素距离
		auto dis=jumptools::GetDistance(posChess,posTarget);
		printf("distance=%.8lf\n", dis);
		clock_t t2 = clock();
		printf("算法耗时: %.8lf ms\n", (t2-t1)*1.0/CLOCKS_PER_SEC*1000);
		//6.计算力度
		int N = 0;
		double randScale= rand() % (60) + 980;//0.98~1.03
		if (dis > 100) {
			N = dis*TIME_SCALE_FAR*randScale/1000;
		}
		else {
			N = dis*TIME_SCALE_NEAR*randScale / 1000;
		}
		jumptools::PushScreen(N);
		g_frame++;
		int sleepTIme=rand() % (1400 - 900 + 1) + 900;
		printf("randScale=%.4lf \n", randScale/1000);
		printf("sleepTIme=%d ms\n", sleepTIme);
		Sleep(sleepTIme);
	}
};