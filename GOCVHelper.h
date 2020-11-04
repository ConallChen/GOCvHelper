////////////////////////////////////////////////////////////////////////////////
////���ƣ�GOCVHelper_2020_04_20.h
////���ܣ�ͼ�����MFC��ǿ
////���ߣ�jsxyhelu(1755311380@qq.com http://jsxyhelu.cnblogs.com)
////��֯��GREENOPEN
////���ڣ�2019-04-20
//�����㷨������Opencv��ȷ���õĻ����¡�
//������Ŀ-����-��������-c/c++-Ԥ������-Ԥ������������� _CRT_SECURE_NO_WARNINGS

#include "pch.h"

#include <io.h>
#include <iostream>
#include <fstream>
#include <opencv.hpp>

#define  DIRECTION_X 0
#define  DIRECTION_Y 1
#define  VP  vector<Point>  //��VP���Ŵ��� vector<point>

using namespace std;
using namespace cv;

//��߶�ͼ����ǿ
cv::Mat multiScaleSharpen(cv::Mat Src, int Radius);
//��ȡͼƬ��ת�Ҷ�
Mat imread2gray(string path);
//���������޵�threshold
Mat threshold2(Mat src,int minvalue,int maxvalue);
//�׶����
Mat fillHoles(Mat src);
float getWhiteRate(Mat src);
Mat getInnerHoles(Mat src);
//��ñȥ���
Mat moveLightDiff(Mat src,int radius = 40);//radiusΪģ��뾶
//ͼ��ϸ��
void thin(const Mat &src, Mat &dst, const int iterations=100);//Mat ��ҪΪDEPTH_8U�Ͷ�ֵͼ 
//ʹ��rect�����͸��
Mat translucence(Mat src,Rect rect,int idepth = 90);
//ʹ��rect����������
Mat mosaic(Mat src,Rect rect,int W = 18,int H = 18);
//������ɫֱ��ͼ�ľ������
double GetHsVDistance(Mat src_base,Mat src_test1);
// ��Ƭ����
void Multiply(Mat& src1, Mat& src2, Mat& dst);
// ��ɫ����
void Color_Burn(Mat& src1, Mat& src2, Mat& dst);
// ������ǿ
void Linear_Burn(Mat& src1, Mat& src2, Mat& dst);
// ���Ͷ����
Mat EnhanceSaturation(Mat temp);
//----------------------------------------------------------------------------------------------------------------------------------------//
//��˷� 
Mat EWM(Mat m1,Mat m2);
//ͼ��ֲ��Աȶ���ǿ�㷨
Mat ACE(Mat src,int C = 4,int n=20,int MaxCG = 5);
//LN�㷨
Mat LocalNormalization(Mat float_gray,float sigma1,float sigma2);
//----------------------------------------------------------------------------------------------------------------------------------------//
//Ѱ����������
VP FindBigestContour(Mat src);
//Ѱ�ҵ�n��������
VP FindnthContour(Mat src,int ith );
//Ѱ�Ҳ����Ƴ���ɫ��ͨ����
vector<VP> connection2(Mat src,Mat& draw);
vector<VP> connection2(Mat src);
//���������������С����ѡ��
vector<VP>  selectShapeArea(Mat src,Mat& draw,vector<VP> contours,int minvalue,int maxvalue);
vector<VP>  selectShapeArea(vector<VP> contours,int minvalue,int maxvalue);
float calculateCircularity(VP contour);
vector<VP> selectShapeCircularity(vector<VP> contours,float minvalue,float maxvalue);
vector<VP> selectShapeCircularity(Mat src,Mat& draw,vector<VP> contours,float minvalue,float maxvalue);
//��������֮��ľ���
float getDistance(Point2f f1,Point2f f2);
//���ص㵽ֱ�ߣ��߶Σ��ľ���
float GetPointLineDistance(Point2f pointInput,Point2f pa,Point2f pb,Point2f& pointOut);
//����pca���������������ĽǶ�
double getOrientation(vector<Point> &pts, Mat &img);
//�������߽�������Ϊ2������
//������pts ������pa pb �����߶ζ˵㣻p1 p2 ��Ϊ���ߺ���Զ2�㣻lenght1,length2 ��Ӧ���룻img ���ڻ�ͼ
//���� �Ƿ�ָ�ɹ�
bool SplitContoursByMiddleLine(vector<Point> &pts,Mat &img,Point pa,Point pb,Point& p1,float& length1,Point& p2,float& length2);
//�����ʵ�ĳ���,����ֵΪfalse�Ļ�����ʶ�𲻳ɹ�
bool getRealWidthHeight(vector<Point> &pts,vector<Point> &resultPts, Mat &img,float& flong,float& fshort);
//ͶӰ��x��Y����,�ϲ���Ϊvup,�²���Ϊvdown,gapΪ�����
void projection2(Mat src,vector<int>& vup,vector<int>& vdown,int direction = DIRECTION_X,int gap = 10);
//�������
bool SmoothEdgeSingleChannel( Mat mInput,Mat &mOutput, double amount, double radius, uchar Threshold) ;
//----------------------------------------------------------------------------------------------------------------------------------------//
//�ݹ��ȡĿ¼��ȫ���ļ�
void getFiles(string path, vector<string>& files,string flag ="r"/*�������ݹ����ﲻдr�Ϳ���*/);
//�ݹ��ȡĿ¼��ȫ��ͼƬ
void getFiles(string path, vector<Mat>& files,string flag = "r");
//�ݹ��ȡĿ¼��ȫ��ͼƬ������
void getFiles(string path, vector<pair<Mat,string>>& files,string flag="r");
//ɾ��Ŀ¼�µ�ȫ���ļ�
void deleteFiles(string path,string flag = "r");
//��������дĿ¼�µ�csv�ļ�,��д���ļ�λ��-���ࡱ��
int writeCsv(const string& filename,const vector<pair<string,string>>srcVect,char separator=';');
//��ȡĿ¼�µ�csv�ļ�,��á��ļ�λ��-���ࡱ��
vector<pair<string,string>> readCsv(const string& filename, char separator = ';') ;
//----------------------------------------------------------------------------------------------------------------------------------------//
//C++��spilt����
void SplitString(const string& s, vector<string>& v, const string& c);
//! ͨ���ļ������ƻ�ȡ�ļ�������������׺
void getFileName(const string& filepath, string& name,string& lastname);
void getFileName(const string& filepath, string& name);
