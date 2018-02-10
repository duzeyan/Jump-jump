#include<opencv2\opencv.hpp>
#include"tools.h"
#include<stdlib.h>
#include<Windows.h>
#include<time.h>


#define JUMP_DEBUG       //ע�͵���ر�����д�ļ�����
#define PURPLE_THRES 40  //��ȡ��ɫ������ֵ
#define BG_THRES 30  //������ɫ��ֵ
#define BG_THRES_SHADOW 40  //������Ӱ��ɫ��ֵ
#define TIME_SCALE_NEAR 6L  //��ѹϵ��
#define TIME_SCALE_FAR 5.8L  //��ѹϵ��
#define BG_TOP_OFFSET 35 //��ȡ����ɫƫ����
int g_frame = 1;

//��ȡ���ӵ�����λ��
//posTop ����λ��
//���صײ�λ��
Point2i GetChessCenterPos(Mat img,Point2i &topPos) {
	int x, y;
	int fx, fy,cnt,maxy,miny;//ͳ������
	fx = fy = cnt=maxy=0;
	miny = img.rows;
	//1.������ɫɸѡ����
	for (y = 0; y < img.rows; y++) {
		Vec3b *ptr = img.ptr<Vec3b>(y);
		for (x = 0; x < img.cols; x++) {
			//���������ĸ�����Ϊ��ɫ
			if ((*ptr)[0] > (*ptr)[1] &&
				(*ptr)[0] > (*ptr)[2]&& //1. B�������
				((*ptr)[0]+ (*ptr)[1]+ (*ptr)[2])/3<100&& //2. �Ҷ�ֵ��С
				(*ptr)[0]-min((*ptr)[1], (*ptr)[2])>10&&//3. B���������������в��
				abs((*ptr)[1]-(*ptr)[2])<40){//4 .R��G�������첻�� (�����ƫ��ɫ)
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
	//2.����
	if (cnt == 0) {
		printf("error cnt=0");
		exit(0);
	}
	fx = fx / (cnt-2);//�����ұ߿ն�������ƫ��
	fy = fy / cnt;
	int dy = (maxy - fy)/7*6;//��������һ�������ĳ���
	fy += dy;
	(*(img.ptr<Vec3b>(fy) + fx))[0] = (*(img.ptr<Vec3b>(fy) + fx))[1] = (*(img.ptr<Vec3b>(fy) + fx))[2] = 0;
#ifdef JUMP_DEBUG
	//imshow("chess", img);
	char filename[255];
	sprintf(filename, "debugIMG/1chess-%d.png", g_frame);
	imwrite(filename,img);
#endif 
	printf("chess pos (%d,%d)\n",fx,fy);
	//3.ȡ������ 
	topPos.x = fx;
	topPos.y = miny - BG_TOP_OFFSET;
	return Point2i(fx,fy);
}



//��ȡ���ӵ�����λ��
Point2i GetBoxCenterPos(Mat img,Vec3b bgColor,Point2i posChess) {
	int x, y,dc1,dc2,halfW;
	uchar tmp;
	//1.������Ӱ��ɫ
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
	//2.ȥ����������Ŀ��
	halfW = img.cols/ 2;
	for (y = 0; y < img.rows; y++) {
		Vec3b *ptr = img.ptr<Vec3b>(y);
		for (x = 0; x < img.cols; x++) {
			//��������ֱ����Ϊ����
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
			//��������ֱ����Ϊ����
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
	//3.ͳ��ǰ����
	vector<Point2i> vTarget;
	if (posChess.x < erodeImg.cols / 2) { //Ŀ�����Ҳ�
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
	//4.Ѱ�Ҷ˵�
	//jumptools::sortPointFirstXY(vTarget, 2); //
	auto top = vTarget[0];
	//����X����
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
	if (posChess.x < erodeImg.cols / 2) {//Ŀ������
		jumptools::sortPointFirstXY(vTarget, 3);
		leftright = vTarget.back();
		//���� �����Ӱûȥ�ɾ�������Ӱ��
		for (int i = vTarget.size() - 1; i>=0;i--) {
			if (vTarget[i].y - top.y   < 30) {
				leftright = vTarget[i];
				break;
			}
		}
	}
	else {//Ŀ�������
		jumptools::sortPointFirstXY(vTarget, 1);
		//���� �����Ӱûȥ�ɾ�������Ӱ��
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
		//1.��ȡ��������
		jumptools::GetScreen(srcImg);
		clock_t t1 = clock();
		//srcImg=imread("debugIMG/case5.png");
#ifdef JUMP_DEBUG
		sprintf(filename, "debugIMG/0srcImg-%d.png", g_frame);
		imwrite(filename, srcImg);
#endif
		//2.Ԥ����
		jumptools::PreProcess(srcImg, preImg);
		//3.��ȡ���ӵײ�λ��
		auto posChess= GetChessCenterPos(preImg.clone(),topPos);
		//4.��ȡ���Ӷ�������λ��
		auto posTarget=GetBoxCenterPos(preImg.clone(), preImg.at<Vec3b>(topPos.y, topPos.x), posChess);
#ifdef JUMP_DEBUG
		circle(preImg, posChess, 4, Scalar(0, 0, 255));
		circle(preImg, posTarget, 4, Scalar(0, 0, 255));
		sprintf(filename, "debugIMG/3result-%d.png",g_frame );
		imwrite(filename, preImg);
#endif
		//5.�������ؾ���
		auto dis=jumptools::GetDistance(posChess,posTarget);
		printf("distance=%.8lf\n", dis);
		clock_t t2 = clock();
		printf("�㷨��ʱ: %.8lf ms\n", (t2-t1)*1.0/CLOCKS_PER_SEC*1000);
		//6.��������
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