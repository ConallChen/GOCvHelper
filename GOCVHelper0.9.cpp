//////////////////////////////////////////////////////////////////////////////
//���ƣ�GOCVHelper0.7b.cpp
//���ܣ�ͼ�����MFC��ǿ
//���ߣ�jsxyhelu(1755311380@qq.com http://jsxyhelu.cnblogs.com)
//��֯��GREENOPEN
//���ڣ�2018-06-26
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <io.h>
#include <odbcinst.h>
#include <afxdb.h>
#include "GoCvHelper.h"
#include "opencv/cv.h"
#include "atlstr.h"
RNG  rng(12345);
#define  DEBUG TRUE
//2016��1��26��GoCvHelper���string ��ز�������������������
//2016��1��28��10:45:22 GOCVHelper������ɫֱ��ͼ��CBIR��ͼ�������ȥ
//2016��8��12��08:27:03 ��ӹ���excel������غ���
//2017��6��28��11:04:35 �޸�һ�����������BUG
//2018��6��26��08:50:09 ���unicode���⣬�����ļ��������ˣ�����Ҫ�������ǽ���Ŀ����Ϊ δ���ã�
//2018��8��7��20:28:22 ����˸�Ϊ��Ч��GetPointLineDistance��
namespace GO{

#pragma region ͼ����ǿ
	//��ȡ�ҶȻ��ɫͼƬ���Ҷ�
	Mat imread2gray(string path){
		Mat src = imread(path);
		Mat srcClone = src.clone();
		if (CV_8UC3 == srcClone.type() )
			cvtColor(srcClone,srcClone,CV_BGR2GRAY);
		return srcClone;
	}

	//���������޵�threshold
	Mat threshold2(Mat src,int minvalue,int maxvalue){
		Mat thresh1;
		Mat thresh2;
		Mat dst;
		threshold(src,thresh1,minvalue,255, THRESH_BINARY);
		threshold(src,thresh2,maxvalue,255,THRESH_BINARY_INV);
		dst = thresh1 & thresh2;
		return dst;
	}

	//����Ӧ���޵�canny�㷨 
    //canny2
	Mat canny2(Mat src){
		Mat imagetmp = src.clone();
		double low_thresh = 0.0;  
		double high_thresh = 0.0;  
		AdaptiveFindThreshold(imagetmp,&low_thresh,&high_thresh);
		Canny(imagetmp,imagetmp,low_thresh,high_thresh);   
		return imagetmp;}
	void AdaptiveFindThreshold( Mat src,double *low,double *high,int aperture_size){
		const int cn = src.channels();
		Mat dx(src.rows,src.cols,CV_16SC(cn));
		Mat dy(src.rows,src.cols,CV_16SC(cn));
		Sobel(src,dx,CV_16S,1,0,aperture_size,1,0,BORDER_REPLICATE);
		Sobel(src,dy,CV_16S,0,1,aperture_size,1,0,BORDER_REPLICATE);
		CvMat _dx = dx;
		CvMat _dy = dy;
		_AdaptiveFindThreshold(&_dx, &_dy, low, high); }  
	void _AdaptiveFindThreshold(CvMat *dx, CvMat *dy, double *low, double *high){                                                                                
		CvSize size;                                                             
		IplImage *imge=0;                                                        
		int i,j;                                                                 
		CvHistogram *hist;                                                       
		int hist_size = 255;                                                     
		float range_0[]={0,256};                                                 
		float* ranges[] = { range_0 };                                           
		double PercentOfPixelsNotEdges = 0.7;                                    
		size = cvGetSize(dx);                                                    
		imge = cvCreateImage(size, IPL_DEPTH_32F, 1);                            
		// �����Ե��ǿ��, ������ͼ����                                          
		float maxv = 0;                                                          
		for(i = 0; i < size.height; i++ ){                                                                        
			const short* _dx = (short*)(dx->data.ptr + dx->step*i);          
			const short* _dy = (short*)(dy->data.ptr + dy->step*i);          
			float* _image = (float *)(imge->imageData + imge->widthStep*i);  
			for(j = 0; j < size.width; j++){                                                                
				_image[j] = (float)(abs(_dx[j]) + abs(_dy[j]));          
				maxv = maxv < _image[j] ? _image[j]: maxv;}}                                                                        
		if(maxv == 0){                                                           
			*high = 0;                                                       
			*low = 0;                                                        
			cvReleaseImage( &imge );                                         
			return;}                                                                        
		// ����ֱ��ͼ                                                            
		range_0[1] = maxv;                                                       
		hist_size = (int)(hist_size > maxv ? maxv:hist_size);                    
		hist = cvCreateHist(1, &hist_size, CV_HIST_ARRAY, ranges, 1);            
		cvCalcHist( &imge, hist, 0, NULL );                                      
		int total = (int)(size.height * size.width * PercentOfPixelsNotEdges);   
		float sum=0;                                                             
		int icount = hist->mat.dim[0].size;                                     
		float *h = (float*)cvPtr1D( hist->bins, 0 );                             
		for(i = 0; i < icount; i++){                                                                        
			sum += h[i];                                                     
			if( sum > total )                                                
				break; }                                                                        
		// ����ߵ�����                                                          
		*high = (i+1) * maxv / hist_size ;                                       
		*low = *high * 0.4;                                                      
		cvReleaseImage( &imge );                                                 
		cvReleaseHist(&hist); }     
// end of canny2

	//���׶�
    //ʹ������
	Mat fillHoles(Mat src){
		Mat dst = getInnerHoles(src);
		threshold(dst,dst,0,255,THRESH_BINARY_INV);
		dst = src + dst;
		return dst;
	}
	//���ͼ���а�ɫ�ı���
	float getWhiteRate(Mat src){
		int iWhiteSum = 0;
		for (int x =0;x<src.rows;x++){
			for (int y=0;y<src.cols;y++){
				if (src.at<uchar>(x,y) != 0)
					iWhiteSum = iWhiteSum +1;
			}
		}
		return (float)iWhiteSum/(float)(src.rows*src.cols);
	}
	//����ڲ��׶�ͼ��
	Mat getInnerHoles(Mat src){ 
		Mat clone = src.clone();
		srand((unsigned)time(NULL));  // ����ʱ������
		float fPreRate = getWhiteRate(clone);
		float fAftRate = 0;
		do {
			clone = src.clone();
			// x y ���� cols rows
			floodFill(clone,Point((int)rand()%src.cols,(int)rand()%src.rows),Scalar(255));
			fAftRate = getWhiteRate(clone);
		} while ( fAftRate < 0.6);
		return clone;
	}
   // end of fillHoles

	//��ñȥ���,radiusΪģ��뾶
	Mat moveLightDiff(Mat src,int radius){
		Mat dst;
		Mat srcclone = src.clone();
		Mat mask = Mat::zeros(radius*2,radius*2,CV_8U);
		circle(mask,Point(radius,radius),radius,Scalar(255),-1);
		//��ñ
		erode(srcclone,srcclone,mask);
		dilate(srcclone,srcclone,mask);
		dst =  src - srcclone;
		return dst;
	}

	//�� DEPTH_8U�Ͷ�ֵͼ�����ϸ��  �����Zhang���п���ϸ���㷨
    //ϸ���㷨
	void thin(const Mat &src, Mat &dst, const int iterations){
		const int height =src.rows -1;
		const int width  =src.cols -1;
		//����һ���������һ������
		if(src.data != dst.data)
			src.copyTo(dst);
		int n = 0,i = 0,j = 0;
		Mat tmpImg;
		uchar *pU, *pC, *pD;
		bool isFinished =FALSE;
		for(n=0; n<iterations; n++){
			dst.copyTo(tmpImg); 
			isFinished =FALSE;   //һ�� ���к���ɨ�� ��ʼ
			//ɨ�����һ ��ʼ
			for(i=1; i<height;  i++) {
				pU = tmpImg.ptr<uchar>(i-1);
				pC = tmpImg.ptr<uchar>(i);
				pD = tmpImg.ptr<uchar>(i+1);
				for(int j=1; j<width; j++){
					if(pC[j] > 0){
						int ap=0;
						int p2 = (pU[j] >0);
						int p3 = (pU[j+1] >0);
						if (p2==0 && p3==1)
							ap++;
						int p4 = (pC[j+1] >0);
						if(p3==0 && p4==1)
							ap++;
						int p5 = (pD[j+1] >0);
						if(p4==0 && p5==1)
							ap++;
						int p6 = (pD[j] >0);
						if(p5==0 && p6==1)
							ap++;
						int p7 = (pD[j-1] >0);
						if(p6==0 && p7==1)
							ap++;
						int p8 = (pC[j-1] >0);
						if(p7==0 && p8==1)
							ap++;
						int p9 = (pU[j-1] >0);
						if(p8==0 && p9==1)
							ap++;
						if(p9==0 && p2==1)
							ap++;
						if((p2+p3+p4+p5+p6+p7+p8+p9)>1 && (p2+p3+p4+p5+p6+p7+p8+p9)<7){
							if(ap==1){
								if((p2*p4*p6==0)&&(p4*p6*p8==0)){                           
									dst.ptr<uchar>(i)[j]=0;
									isFinished =TRUE;                            
								}
							}
						}                    
					}

				} //ɨ�����һ ����
				dst.copyTo(tmpImg); 
				//ɨ����̶� ��ʼ
				for(i=1; i<height;  i++){
					pU = tmpImg.ptr<uchar>(i-1);
					pC = tmpImg.ptr<uchar>(i);
					pD = tmpImg.ptr<uchar>(i+1);
					for(int j=1; j<width; j++){
						if(pC[j] > 0){
							int ap=0;
							int p2 = (pU[j] >0);
							int p3 = (pU[j+1] >0);
							if (p2==0 && p3==1)
								ap++;
							int p4 = (pC[j+1] >0);
							if(p3==0 && p4==1)
								ap++;
							int p5 = (pD[j+1] >0);
							if(p4==0 && p5==1)
								ap++;
							int p6 = (pD[j] >0);
							if(p5==0 && p6==1)
								ap++;
							int p7 = (pD[j-1] >0);
							if(p6==0 && p7==1)
								ap++;
							int p8 = (pC[j-1] >0);
							if(p7==0 && p8==1)
								ap++;
							int p9 = (pU[j-1] >0);
							if(p8==0 && p9==1)
								ap++;
							if(p9==0 && p2==1)
								ap++;
							if((p2+p3+p4+p5+p6+p7+p8+p9)>1 && (p2+p3+p4+p5+p6+p7+p8+p9)<7){
								if(ap==1){
									if((p2*p4*p8==0)&&(p2*p6*p8==0)){                           
										dst.ptr<uchar>(i)[j]=0;
										isFinished =TRUE;                            
									}
								}
							}                    
						}
					}
				} //һ�� ���к���ɨ�����          
				//�����ɨ�������û��ɾ���㣬����ǰ�˳�
				if(isFinished ==FALSE)
					break; 
			}
		}
	}
// end of thin

	//ʹ��rect�����͸��
	Mat translucence(Mat src,Rect rect,int idepth){
		Mat dst = src.clone();
		Mat roi = dst(rect);
		roi += Scalar(idepth,idepth,idepth);
		return dst;
	}

	//ʹ��rect�������������
	Mat mosaic(Mat src,Rect rect,int W,int H){
		Mat dst = src.clone();
		Mat roi = dst(rect);
		for (int i=W; i<roi.cols; i+=W) {
			for (int j=H; j<roi.rows; j+=H) {
				uchar s=roi.at<uchar>(j-H/2,(i-W/2)*3);
				uchar s1=roi.at<uchar>(j-H/2,(i-W/2)*3+1);
				uchar s2=roi.at<uchar>(j-H/2,(i-W/2)*3+2);
				for (int ii=i-W; ii<=i; ii++) {
					for (int jj=j-H; jj<=j; jj++) {
						roi.at<uchar>(jj,ii*3+0)=s;
						roi.at<uchar>(jj,ii*3+1)=s1;
						roi.at<uchar>(jj,ii*3+2)=s2;
					}
				}
			}
		}
		return dst;
	}


//������ɫֱ��ͼ�ľ������
double GetHsVDistance(Mat src_base,Mat src_test1){
	Mat   hsv_base;
	Mat   hsv_test1;
	///  Convert  to  HSV
	cvtColor(  src_base,  hsv_base,  COLOR_BGR2HSV  );
	cvtColor(  src_test1,  hsv_test1,  COLOR_BGR2HSV  );
	///  Using  50  bins  for  hue  and  60  for  saturation
	int  h_bins  =  50;  int  s_bins  =  60;
	int  histSize[]  =  {  h_bins,  s_bins  };
	//  hue  varies  from  0  to  179,  saturation  from  0  to  255
	float  h_ranges[]  =  {  0,  180  };
	float  s_ranges[]  =  {  0,  256  };
	const  float*  ranges[]  =  {  h_ranges,  s_ranges  };
	//  Use  the  o-th  and  1-st  channels
	int  channels[]  =  {  0,  1  };
	///  Histograms
	MatND  hist_base;
	MatND  hist_test1;
	///  Calculate  the  histograms  for  the  HSV  images
	calcHist(  &hsv_base,  1,  channels,  Mat(),  hist_base,  2,  histSize,  ranges,  true,  false  );
	normalize(  hist_base,  hist_base,  0,  1,  NORM_MINMAX,  -1,  Mat()  );
	calcHist(  &hsv_test1,  1,  channels,  Mat(),  hist_test1,  2,  histSize,  ranges,  true,  false  );
	normalize(  hist_test1,  hist_test1,  0,  1,  NORM_MINMAX,  -1,  Mat()  );
	///  Apply  the  histogram  comparison  methods
	double  base_test1  =  compareHist(  hist_base,  hist_test1,  0  );
	return base_test1;
}
// Multiply ��Ƭ����
void Multiply(Mat& src1, Mat& src2, Mat& dst)
{
	for(int index_row=0; index_row<src1.rows; index_row++)
	{
		for(int index_col=0; index_col<src1.cols; index_col++)
		{
			for(int index_c=0; index_c<3; index_c++)
				dst.at<Vec3f>(index_row, index_col)[index_c]=
				src1.at<Vec3f>(index_row, index_col)[index_c]*
				src2.at<Vec3f>(index_row, index_col)[index_c];
		}
	}
}
// Color_Burn ��ɫ����
void Color_Burn(Mat& src1, Mat& src2, Mat& dst)
{
	for(int index_row=0; index_row<src1.rows; index_row++)
	{
		for(int index_col=0; index_col<src1.cols; index_col++)
		{
			for(int index_c=0; index_c<3; index_c++)
				dst.at<Vec3f>(index_row, index_col)[index_c]=1-
				(1-src1.at<Vec3f>(index_row, index_col)[index_c])/
				src2.at<Vec3f>(index_row, index_col)[index_c];
		}
	}
}
// ������ǿ
void Linear_Burn(Mat& src1, Mat& src2, Mat& dst)
{
	for(int index_row=0; index_row<src1.rows; index_row++)
	{
		for(int index_col=0; index_col<src1.cols; index_col++)
		{
			for(int index_c=0; index_c<3; index_c++)
				dst.at<Vec3f>(index_row, index_col)[index_c]=max(
				src1.at<Vec3f>(index_row, index_col)[index_c]+
				src2.at<Vec3f>(index_row, index_col)[index_c]-1, (float)0.0);
		}
	}
}


//��˷� elementWiseMultiplication
Mat EWM(Mat m1,Mat m2){
	Mat dst=m1.mul(m2);
	return dst;
}
//ͼ��ֲ��Աȶ���ǿ�㷨
Mat ACE(Mat src,int C,int n,int MaxCG){
	Mat meanMask;
	Mat varMask;
	Mat meanGlobal;
	Mat varGlobal;
	Mat dst;
	Mat tmp;
	Mat tmp2;
	blur(src.clone(),meanMask,Size(50,50));//meanMaskΪ�ֲ���ֵ 
	tmp = src - meanMask;  
	varMask = EWM(tmp,tmp);         
	blur(varMask,varMask,Size(50,50));    //varMaskΪ�ֲ�����   
	//����ɾֲ���׼��
	varMask.convertTo(varMask,CV_32F);
	for (int i=0;i<varMask.rows;i++){
		for (int j=0;j<varMask.cols;j++){
			varMask.at<float>(i,j) =  (float)sqrt(varMask.at<float>(i,j));
		}
	}
	meanStdDev(src,meanGlobal,varGlobal); //meanGlobalΪȫ�־�ֵ varGlobalΪȫ�ֱ�׼��
	tmp2 = varGlobal/varMask;
	for (int i=0;i<tmp2.rows;i++){
		for (int j=0;j<tmp2.cols;j++){
			if (tmp2.at<float>(i,j)>MaxCG){
				tmp2.at<float>(i,j) = MaxCG;
			}
		}
	}
	tmp2.convertTo(tmp2,CV_8U);
	tmp2 = EWM(tmp2,tmp);
	dst = meanMask + tmp2;
	imshow("D����",dst);
	dst = meanMask + C*tmp;
	imshow("C����",dst);
	return dst;
}

//Local Normalization  input is 32f1u
Mat LocalNormalization(Mat float_gray,float sigma1,float sigma2){
	Mat gray, blur, num, den;
	float_gray.convertTo(float_gray, CV_32F, 1.0/255.0);
	// numerator = img - gauss_blur(img)
	boxFilter(float_gray,blur,float_gray.depth(),Size(sigma1,sigma1));
	num = float_gray - blur;
	boxFilter(num.mul(num),blur,num.depth(),Size(sigma2,sigma2));
	// denominator = sqrt(gauss_blur(img^2))
	pow(blur, 0.5, den);
	// output = numerator / denominator
	gray = num / den;
	// normalize output into [0,1]
	normalize(gray, gray, 0.0, 1.0, NORM_MINMAX, -1);
	return gray;
}
#pragma endregion ͼ����ǿ

#pragma region ͼ����
	//Ѱ����������
	VP FindBigestContour(Mat src){    
		int imax = 0; //����������������
		int imaxcontour = -1; //������������Ĵ�С
		std::vector<std::vector<Point>>contours;    
		findContours(src,contours,CV_RETR_LIST,CV_CHAIN_APPROX_SIMPLE);
		for (int i=0;i<contours.size();i++){
			int itmp =  contourArea(contours[i]);//������õ���������С
			if (imaxcontour < itmp ){
				imax = i;
				imaxcontour = itmp;
			}
		}
		return contours[imax];
	}
	//Ѱ�ҵ�nth������
	//ith = 0�������ith=1 �����2�����Դ�����
	bool sortfunction (std::vector<Point> c1,std::vector<Point> c2) { return (contourArea(c1)>contourArea(c2)); }  
	VP FindnthContour(Mat src,int ith ){    
		std::vector<std::vector<Point>>contours;    
		findContours(src,contours,CV_RETR_LIST,CV_CHAIN_APPROX_SIMPLE);
	    std::sort(contours.begin(),contours.end(),sortfunction);
		return contours[ith];
	}
	//Ѱ�Ҳ����Ƴ���ɫ��ͨ����
	vector<VP> connection2(Mat src,Mat& draw){    
		draw = Mat::zeros(src.rows,src.cols,CV_8UC3);
		vector<VP>contours;    
		findContours(src.clone(),contours,CV_RETR_LIST,CV_CHAIN_APPROX_SIMPLE);
		//���ڸ����������ɫ�Ḳ��С�������������Ƚ����������
		//ð��������С��������
		VP vptmp;
		for(int i=1;i<contours.size();i++){
			for(int j=contours.size()-1;j>=i;j--){
				if (contourArea(contours[j]) < contourArea(contours[j-1]))
				{
					vptmp = contours[j-1];
					contours[j-1] = contours[j];
					contours[j] = vptmp;
				}
			}
		}
		//��ӡ���
		for (int i=contours.size()-1;i>=0;i--){
			Scalar  color  = Scalar(rng.uniform(0,255),rng.uniform(0,255),rng.uniform(0,255));
			drawContours(draw,contours,i,color,-1);
		}
		return contours;
	}
	vector<VP> connection2(Mat src){
		Mat draw;
		return connection2(src,draw);
	}

	//���������������С����ѡ��
	vector<VP>  selectShapeArea(Mat src,Mat& draw,vector<VP> contours,int minvalue,int maxvalue){
		vector<VP> result_contours;
		draw = Mat::zeros(src.rows,src.cols,CV_8UC3);
		for (int i=0;i<contours.size();i++){ 
			double countour_area = contourArea(contours[i]);
			if (countour_area >minvalue && countour_area<maxvalue)
				result_contours.push_back(contours[i]);
		}
		for (int i=0;i<result_contours.size();i++){
			int iRandB = rng.uniform(0,255);
			int iRandG = rng.uniform(0,255);
			int iRandR = rng.uniform(0,255);
			Scalar  color  = Scalar(iRandB,iRandG,iRandR);
			drawContours(draw,result_contours,i,color,-1);
			char cbuf[100];sprintf_s(cbuf,"%d",i+1);
			//Ѱ����С����Բ,���Բ�ġ�ʹ�÷�ɫ��ӡ������Ŀ
			float radius;
			Point2f center;
			minEnclosingCircle(result_contours[i],center,radius);
			putText(draw,cbuf,center, FONT_HERSHEY_PLAIN ,5,Scalar(255-iRandB,255-iRandG,255-iRandR),5);
		}
		return result_contours;
	}
	vector<VP>  selectShapeArea(vector<VP> contours,int minvalue,int maxvalue)
	{
		vector<VP> result_contours;
		for (int i=0;i<contours.size();i++){ 
			double countour_area = contourArea(contours[i]);
			if (countour_area >minvalue && countour_area<maxvalue)
				result_contours.push_back(contours[i]);
		}
		return result_contours;
	}

	vector<VP> selectShapeCircularity(Mat src,Mat& draw,vector<VP> contours,float minvalue,float maxvalue){
		vector<VP> result_contours;
		draw = Mat::zeros(src.rows,src.cols,CV_8UC3);
		for (int i=0;i<contours.size();i++){
			float fcompare = calculateCircularity(contours[i]);
			if (fcompare >=minvalue && fcompare <=maxvalue)
				result_contours.push_back(contours[i]);
		}
		for (int i=0;i<result_contours.size();i++){
			Scalar  color  = Scalar(rng.uniform(0,255),rng.uniform(0,255),rng.uniform(0,255));
			drawContours(draw,result_contours,i,color,-1);
		}
		return result_contours;
	}
	vector<VP> selectShapeCircularity(vector<VP> contours,float minvalue,float maxvalue){
		vector<VP> result_contours;
		for (int i=0;i<contours.size();i++){
			float fcompare = calculateCircularity(contours[i]);
			if (fcompare >=minvalue && fcompare <=maxvalue)
				result_contours.push_back(contours[i]);
		}
		return result_contours;
	}
	//����������Բ������
	float calculateCircularity(VP contour){
		Point2f center;
		float radius = 0;
		minEnclosingCircle((Mat)contour,center,radius);
		//����С���Բ�뾶��Ϊ��ѧ���������������ϸ��㵽Բ�ľ���ı�׼��
		float fsum = 0;
		float fcompare = 0;
		for (int i=0;i<contour.size();i++){   
			Point2f ptmp = contour[i];
			float fdistenct = sqrt((float)((ptmp.x - center.x)*(ptmp.x - center.x)+(ptmp.y - center.y)*(ptmp.y-center.y)));
			float fdiff = abs(fdistenct - radius);
			fsum = fsum + fdiff;
		}
		fcompare = fsum/(float)contour.size();
		return fcompare;
	}

	//��������֮��ľ���
	float getDistance(Point2f f1,Point2f f2)
	{
		return sqrt((float)(f1.x - f2.x)*(f1.x - f2.x) + (f1.y -f2.y)*(f1.y- f2.y));
	}
	//���ص㵽ֱ�ߣ��߶Σ��ľ���
	float GetPointLineDistance(Point2f pointInput,Point2f pa,Point2f pb,Point2f& pointOut)
	{
		
		Point2f p1;
		Point2f p2;
		if (pa.x<pb.x)
		{
			p1 = pa;
			p2 = pb;
		}
		else
		{
			p1 = pb;
			p2 = pa;
		}
		//��֧����
		if (p1.x == p2.x)
		{
			pointOut.x = p1.x ;
			pointOut.y = pointInput.y;
			return abs(pointInput.x - p1.x);
		}

		if (p1.y == p2.y)
		{
			pointOut.y = p1.y ;
			pointOut.x = pointInput.x;
			return abs(pointInput.y - p1.y);
		}

		float fthea = (p2.y - p1.y)/(p2.x-p1.x);
		int fMinDistance = 100000;
		int fMinNum = -1;
		for (int i=0;i<(int)(p2.x-p1.x);i++)
		{
			float fx = p1.x +i;
			float fy = i*fthea + p1.y;
			float ftmp =  GO::getDistance(Point2f(fx,fy),pointInput);
			if (ftmp<fMinDistance)
			{
				fMinDistance = ftmp;
				fMinNum = i;
			}
		}
		//���Ի�ͼ
		pointOut.x = p1.x +fMinNum;
		pointOut.y = fMinNum*fthea + p1.y;
		return fMinDistance;
	}

	//���ص㵽ֱ�ߣ��߶Σ��ľ���,�����˸�����Ĵ��룬Ҳ��Ҳ����Ч
	float GetPointLineDistance(Mat src,Point2f pointInput,Point2f pa,Point2f pb,Point2f& pointOut)
	{
		LineIterator it(src,pa,pb);
		int fMinDistance = 100000;
		int fMinNum = -1;
		for(int i = 0; i < it.count; i++, ++it)
		{
			float ftmp =  GO::getDistance(it.pos(),pointInput);
			if (ftmp<fMinDistance)
			{
				fMinDistance = ftmp;
				pointOut=it.pos;
			}
		}
		return fMinDistance;
	}
	//��ù�������Ҫ������ͼ�Ͻ��б�գ����ҷ��ؽǶȽ��
	//ע�⣬�����������opencv�����Ѿ������׼����
	double getOrientation(vector<Point> &pts, Mat &img)
	{
		//����pca���ݡ����������ǽ��������x��y��Ϊ����άѹ��data_pts��ȥ��
		Mat data_pts = Mat(pts.size(), 2, CV_64FC1);//ʹ��mat���������ݣ�Ҳ��Ϊ�˺���pca������Ҫ
		for (int i = 0; i < data_pts.rows; ++i)
		{
			data_pts.at<double>(i, 0) = pts[i].x;
			data_pts.at<double>(i, 1) = pts[i].y;
		}
		//ִ��PCA����
		PCA pca_analysis(data_pts, Mat(), CV_PCA_DATA_AS_ROW);
		//�������Ҫ�������ڱ����У���Ӧ�ľ��������е㣬Ҳ��ͼ���е�
		Point pos = Point(pca_analysis.mean.at<double>(0, 0),pca_analysis.mean.at<double>(0, 1));
		//�洢��������������ֵ
		vector<Point2d> eigen_vecs(2);
		vector<double> eigen_val(2);
		for (int i = 0; i < 2; ++i)
		{
			eigen_vecs[i] = Point2d(pca_analysis.eigenvectors.at<double>(i, 0),pca_analysis.eigenvectors.at<double>(i, 1));
			eigen_val[i] = pca_analysis.eigenvalues.at<double>(i,0);//ע�⣬����ط�ԭ����д����
		}
		//������/ͼ���е����СԲ
		circle(img, pos, 3, CV_RGB(255, 0, 255), 2);
		//�����ֱ�ߣ�����Ҫ�����ϻ���ֱ��
		line(img, pos, pos + 0.02 * Point(eigen_vecs[0].x * eigen_val[0], eigen_vecs[0].y * eigen_val[0]) , CV_RGB(255, 255, 0),3);
		line(img, pos, pos + 0.02 * Point(eigen_vecs[1].x * eigen_val[1], eigen_vecs[1].y * eigen_val[1]) , CV_RGB(0, 255, 255),3);
		//���ؽǶȽ��
		return atan2(eigen_vecs[0].y, eigen_vecs[0].x);
	}

	//�������߽�������Ϊ2������
	//pts ����
	//pa pb �����߶ζ˵�
	//p1 p2 ��Ϊ���ߺ���Զ2��
	//lenght1,length2 ��Ӧ����
	//img ���ڻ�ͼ
	//���� �Ƿ�ָ�ɹ�
	bool SplitContoursByMiddleLine(vector<Point> &pts,Mat &img,Point pa,Point pb,Point& p1,float& length1,Point& p2,float& length2)
	{
		//Ѱ������������(ʵ�������߶Σ��Ľ���
		int isum = 0;
		Point2f pointOut;
		//bool bIsCross =false;
		int iStart = -1;
		int iEnd = -1;
		vector<int> vecBorderPoints;
		float fDistance = 0;
		//����������Ϊ��������
		for (int i = 0;i< pts.size();i++)
		{
			float f = GetPointLineDistance(img,pts[i], pa,pb,pointOut);//������������е���Զ�����
			
		}

		//������ �����ͱ�Ե�Ľ��� �������򣬵õ�������Զ�ĵ�� 
		float fDistance = 0;
		for (int i = 0 ;i<vecBorderPoints.size();i++)
		{
			for (int j = i;j<vecBorderPoints.size();j++) //�Ѿ����������Ż���
			{
				if (getDistance(pts[vecBorderPoints[i]],pts[vecBorderPoints[j]]) > fDistance)
				{
					fDistance = getDistance(pts[vecBorderPoints[i]],pts[vecBorderPoints[j]]);
					iStart = vecBorderPoints[i];
					iEnd = vecBorderPoints[j];
				}
			}
		}
		if (-1 == iEnd ) //���������ˣ������ȽϷ���ķ�����
			return false;
		if (iStart > iEnd)
			swap(iStart,iEnd);
		if ((iEnd - iStart)<pts.size()/4)
			return false;//������ƻ���
		if (DEBUG)
			{
				printf("\n\n");
				circle(img,pts[iStart],5,Scalar(0,255,0),5);
				circle(img,pts[iEnd],5,Scalar(0,255,0),5);
			}
			

		vector<Point> vector1;
		vector<Point> vector2;
		for (int i = 0;i<pts.size();i++)
		{
			if (i>=iStart && i<=iEnd)
			{
				vector1.push_back(pts[i]);
				if(DEBUG)
					circle(img,pts[i],3,Scalar(0,0,255));
			}	
			else
			{
				vector2.push_back(pts[i]);
				if(DEBUG)
					circle(img,pts[i],3,Scalar(0,255,255));
			}

		}
		//�ֱ������������������ҵ��������
		Point pstart = pts[iStart];
		Point pend   = pts[iEnd];
		float fmax = -1;int imax = -1;
		for (int i =0;i<vector1.size();i++)
		{
			float f = GetPointLineDistance(vector1[i], pa,pb,pointOut);
			if (f>fmax) //ð��
			{
				fmax = f;
				imax = i;
			}
		}
		if (DEBUG)
			circle(img,vector1[imax],3,cv::Scalar(255,0,0),2);
		p1 = vector1[imax];
		length1 = fmax;

		fmax = -1; imax = -1;
		for (int i =0;i<vector2.size();i++)
		{
			float f = GetPointLineDistance(vector2[i], pa,pb,pointOut);
			if (f>fmax) //ð��
			{
				fmax = f;
				imax = i;
			}
		}
		if (DEBUG)
			circle(img,vector2[imax],3,cv::Scalar(255,0,0),2);
		p2 = vector2[imax];
		length2 = fmax;
		return true;
	}

	//�����ʵ�ĳ���,����ֵΪfalse�Ļ�����ʶ�𲻳ɹ�
	bool getRealWidthHeight(vector<Point> &pts,vector<Point> &resultPts, Mat &img,float& flong,float& fshort)
	{
		//����pca���ݡ����������ǽ��������x��y��Ϊ����άѹ��data_pts��ȥ��
		Mat data_pts = Mat(pts.size(), 2, CV_64FC1);//ʹ��mat���������ݣ�Ҳ��Ϊ�˺���pca������Ҫ
		for (int i = 0; i < data_pts.rows; ++i)
		{
			data_pts.at<double>(i, 0) = pts[i].x;
			data_pts.at<double>(i, 1) = pts[i].y;
		}
		//ִ��PCA����
		PCA pca_analysis(data_pts, Mat(), CV_PCA_DATA_AS_ROW);
		//�������Ҫ�������ڱ����У���Ӧ�ľ��������е㣬Ҳ��ͼ���е�
		Point pos = Point(pca_analysis.mean.at<double>(0, 0),pca_analysis.mean.at<double>(0, 1));
		//�����������������ֵ
		vector<Point2d> eigen_vecs(2);
		vector<double> eigen_val(2);
		for (int i = 0; i < 2; ++i)
		{
			eigen_vecs[i] = Point2d(pca_analysis.eigenvectors.at<double>(i, 0),pca_analysis.eigenvectors.at<double>(i, 1));
			eigen_val[i] = pca_analysis.eigenvalues.at<double>(i,0);
		}
		if (eigen_vecs[0].x == 0 || abs(eigen_vecs[0].y / eigen_vecs[0].x) >100)//һ�����������Ϊ��ֱ��������ʱ�����ֱ�Ӳ��ü򵥷���
			return false;
		if (eigen_vecs[1].x == 0 || abs(eigen_vecs[1].y / eigen_vecs[1].x) >100)
			return false;
		//������/ͼ���е����СԲ
		if (DEBUG)
			circle(img, pos, 3, CV_RGB(255, 0, 255), 2);

		//��ó�����������Ľ��ӵ㣬���ȵõ�һ�����Կ��е�ֱ��
		//����
		Point pa = pos-0.04 * Point(eigen_vecs[0].x * eigen_val[0], eigen_vecs[0].y * eigen_val[0]); 
		Point pb = pos + 0.04 * Point(eigen_vecs[0].x * eigen_val[0], eigen_vecs[0].y * eigen_val[0]) ;
		//����
		Point pc = pos- 0.2 * Point(eigen_vecs[1].x * eigen_val[1], eigen_vecs[1].y * eigen_val[1]);
		Point pd = pos + 0.2 * Point(eigen_vecs[1].x * eigen_val[1], eigen_vecs[1].y * eigen_val[1]);
		//ͨ���Ա߽�����޶�������㷨Ч�ʣ���õĽ���ܹ���֤��ʱ��pa,pb������Զ�ĵ�
		LineIterator it(img, pa, pb);
		LineIterator it2(img, pb, pa);
		LineIterator it3(img, pc, pd);
		LineIterator it4(img, pd, pc);
		for(int i = 0; i < it.count; i++, ++it)
		{
			if( 0 == pointPolygonTest(pts,it.pos(),true))//��һ���������ϵĵ�
				pa = it.pos();
		}
		
		for(int i = 0; i < it2.count; i++, ++it2)
		{
			if( 0 == pointPolygonTest(pts,it2.pos(),true))//��һ���������ϵĵ�
				pb = it2.pos();
		}
		
		for(int i = 0; i < it3.count; i++, ++it3)
		{
			if( 0 == pointPolygonTest(pts,it3.pos(),true))//��һ���������ϵĵ�
				pc = it3.pos();
		}
	
		for(int i = 0; i < it4.count; i++, ++it4)
		{
			if( 0 == pointPolygonTest(pts,it4.pos(),true))//��һ���������ϵĵ�
				pd = it4.pos();
		}
		//�����ֱ�ߣ��ڳ������ϻ���ֱ��
		if (DEBUG)
		{
			line(img, pa,pb , CV_RGB(255, 255, 0),10);
			line(img, pc,pd, CV_RGB(0, 255, 255),10);
		}

		//�� �������ճ�������л���.����_p[]�õ�����4���߽���Զ�㣻��_length���ǳ���
		Point _p[4]; 
		float _length[4] = {-1,-1,-1,-1};
		if (!SplitContoursByMiddleLine(pts,img,pa,pb,_p[0],_length[0],_p[1],_length[1]))
			return false;
		if (!SplitContoursByMiddleLine(pts,img,pc,pd,_p[2],_length[2],_p[3],_length[3]))
			return false;

		//��ʼ��ý���
		if (eigen_vecs[0].x == 0 || eigen_vecs[1].x == 0)//����Ϊ0
			return false;
		float k_long = eigen_vecs[0].y /eigen_vecs[0].x;
		float k_short = eigen_vecs[1].y /eigen_vecs[1].x;
		if (k_long == k_short)//���������Ӧ�ó���
			return false;

		//���س���
		if (_length[0]<0 || _length[1]<0 || _length[2]<0 || _length[3]<0)
			return false;
		fshort = _length[0]+_length[1];
		flong  = _length[2]+_length[3];
		//ͨ�������������������� 
		Point p[4]; 
		p[0].x = (k_long * _p[0].x   - k_short * _p[2].x  +  _p[2].y - _p[0].y)  / (k_long - k_short);
		p[0].y = (p[0].x - _p[0].x)*k_long + _p[0].y;
		p[1].x = (k_long * _p[0].x   - k_short * _p[3].x  +  _p[3].y - _p[0].y)  / (k_long - k_short);
		p[1].y = (p[1].x - _p[0].x)*k_long + _p[0].y;
		p[2].x = (k_long * _p[1].x   - k_short * _p[2].x  +  _p[2].y - _p[1].y)  / (k_long - k_short);
		p[2].y = (p[2].x - _p[1].x)*k_long + _p[1].y;
		p[3].x = (k_long * _p[1].x   - k_short * _p[3].x  +  _p[3].y - _p[1].y)  / (k_long - k_short);
		p[3].y = (p[3].x - _p[1].x)*k_long + _p[1].y;

		//������
		if (p[1].x < p[0].x)
			swap(p[1],p[0]);
		if (p[3].x < p[2].x)
			swap(p[3],p[2]);
		//��ͼ
		for (int i = 0;i<4;i++)
			resultPts.push_back(p[i]);
		
		//line(img,p[0],p[1],CV_RGB(0, 255, 255), 5);
		//line(img,p[0],p[2],CV_RGB(0, 255, 255), 5);
		//line(img,p[3],p[1],CV_RGB(0, 255, 255), 5);
		//line(img,p[3],p[2],CV_RGB(0, 255, 255), 5);

		return true;

	}

	//ͶӰ��x��Y����,�ϲ���Ϊvup,�²���Ϊvdown,gapΪ�����
	void projection2(Mat src,vector<int>& vup,vector<int>& vdown,int direction,int gap){
		Mat tmp = src.clone();
		vector<int> vdate;
		if (DIRECTION_X == direction){
			for (int i=0;i<tmp.cols;i++){
				Mat data = tmp.col(i);
				int itmp = countNonZero(data);
				vdate.push_back(itmp);
			}
		}else{
			for (int i=0;i<tmp.rows;i++){
				Mat data = tmp.row(i);
				int itmp = countNonZero(data);
				vdate.push_back(itmp);
			}
		}
		//����,ȥ������С��gap����Ķ�
		if (vdate.size()<=gap)
			return;
		for (int i=0;i<vdate.size()-gap;i++){
			if (vdate[i]>0 && vdate[i+gap]>0){
				for (int j=i;j<i+gap;j++){
					vdate[j] = 1;
				}
				i = i+gap-1;
			}
		}
		//��¼������
		for (int i=1;i<vdate.size();i++){
			if (vdate[i-1] == 0 && vdate[i]>0)
				vup.push_back(i);
			if (vdate[i-1]>0 && vdate[i] == 0)
				vdown.push_back(i);
		}
	}
	//�����ữ
	bool SmoothEdgeSingleChannel( Mat mInput,Mat &mOutput, double amount, double radius, uchar Threshold) 
	{
		if(mInput.empty())
		{
			return 0;
		}
		if(radius<1)
			radius=1;

		Mat mGSmooth,mDiff,mAbsDiff;
		mOutput = Mat(mInput.size(),mInput.type());

		GaussianBlur(mInput,mGSmooth,Size(0,0),radius); 
		//imshow("mGSmooth",mGSmooth);

		subtract(mGSmooth,mInput,mDiff);
		//imshow("mDiff",mDiff);

		mDiff*=amount;
		threshold(abs(2* mDiff),mAbsDiff,Threshold,255,THRESH_BINARY_INV);

		mDiff.setTo(Scalar(0),mAbsDiff);
		//imshow("mDiff Multiplied",mDiff);

		add(mInput,mDiff,mOutput);

		return true;
	}
#pragma endregion ͼ����

#pragma region �ļ�����
	//�ݹ��ȡĿ¼��ȫ���ļ�
	void getFiles(string path, vector<string>& files,string flag){
		//�ļ����
		long   hFile   =   0;
		//�ļ���Ϣ
		struct _finddata_t fileinfo;
		string p;
		if((hFile = _findfirst(p.assign(path).append("\\*").c_str(),&fileinfo)) !=  -1){
			do{
				//�����Ŀ¼,����֮,�������,�����б�
				if((fileinfo.attrib &  _A_SUBDIR)){
					if(strcmp(fileinfo.name,".") != 0  &&  strcmp(fileinfo.name,"..") != 0 && flag=="r")
						getFiles( p.assign(path).append("\\").append(fileinfo.name), files,flag );
				}
				else{
					files.push_back(p.assign(path).append("\\").append(fileinfo.name) );
				}
			}while(_findnext(hFile, &fileinfo)  == 0);
			_findclose(hFile);
		}
	}
	//�ݹ��ȡĿ¼��ȫ��ͼƬ
	void getFiles(string path, vector<Mat>& files,string flag){
		vector<string> fileNames;
		getFiles(path,fileNames,flag);
		for (int i=0;i<fileNames.size();i++){
			Mat tmp = imread(fileNames[i]);
			if (tmp.rows>0)//�����ͼƬ
				files.push_back(tmp);
		}
	}
	//�ݹ��ȡĿ¼��ȫ��ͼƬ������
	void getFiles(string path, vector<pair<Mat,string>>& files,string flag){
		vector<string> fileNames;
		getFiles(path,fileNames,flag);
		for (int i=0;i<fileNames.size();i++){
			Mat tmp = imread(fileNames[i]);
			if (tmp.rows>0){
				pair<Mat,string> apir;
				apir.first = tmp;
				apir.second = fileNames[i];
				files.push_back(apir);
			}
		}
	}
	////ɾ��Ŀ¼�µ�ȫ���ļ�
	void deleteFiles(string path,string flag){
		//�ļ����
		long   hFile   =   0;
		//�ļ���Ϣ
		struct _finddata_t fileinfo;
		string p;
		if((hFile = _findfirst(p.assign(path).append("\\*").c_str(),&fileinfo)) !=  -1){
			do{
				//�����Ŀ¼,����֮,�������,�����б�
				if((fileinfo.attrib &  _A_SUBDIR)){
					if(strcmp(fileinfo.name,".") != 0  &&  strcmp(fileinfo.name,"..") != 0 && flag=="r")
						deleteFiles(p.assign(path).append("\\").append(fileinfo.name).c_str(),flag );
				}
				else{
					deleteFiles(p.assign(path).append("\\").append(fileinfo.name).c_str());
				}
			}while(_findnext(hFile, &fileinfo)  == 0);
			_findclose(hFile);
		}
	}
	//��������дĿ¼�µ�csv�ļ�,��д���ļ�λ��-���ࡱ��
	int writeCsv(const string& filename,const vector<pair<string,string>>srcVect,char separator ){
		ofstream file(filename.c_str(),ofstream::app);
		if (!file)
			return 0;
		for (int i=0;i<srcVect.size();i++){
			file<<srcVect[i].first<<separator<<srcVect[i].second<<endl;
		}
		return srcVect.size();
	}
	//��ȡĿ¼�µ�csv�ļ�,��á��ļ�λ��-���ࡱ��
	vector<pair<string,string>> readCsv(const string& filename, char separator) {
		pair<string,string> apair;
		string line, path, classlabel;
		vector<pair<string,string>> retVect;
		ifstream file(filename.c_str(), ifstream::in);
		if (!file) 
			return retVect;
		while (getline(file, line)) {
			stringstream liness(line);
			getline(liness, path, separator);
			getline(liness, classlabel);
			if(!path.empty() && !classlabel.empty()) {
				apair.first = path;
				apair.second = classlabel;
				retVect.push_back(apair);
			}

		}
		return retVect;
	}
	////���ini�ļ��е�ֵ
	 CString  GetInitString( CString Name1 ,CString Name2){
		char c[100] ;
		memset( c ,0 ,100) ;
		CString csCfgFilePath;
		GetModuleFileName(NULL, csCfgFilePath.GetBufferSetLength(MAX_PATH+1), MAX_PATH); 
		csCfgFilePath.ReleaseBuffer(); 
		int nPos = csCfgFilePath.ReverseFind ('\\');
		csCfgFilePath = csCfgFilePath.Left (nPos);
		csCfgFilePath += "\\Config" ;
		BOOL br = GetPrivateProfileString(Name1,Name2 ,"0",c, 100 , csCfgFilePath) ;
		CString rstr ;
		rstr.Format("%s" , c) ;
		return rstr ;
	}
	 //д��ini�ļ��е�ֵ
	 void WriteInitString( CString Name1 ,CString Name2 ,CString strvalue){
		CString csCfgFilePath;
		GetModuleFileName(NULL, csCfgFilePath.GetBufferSetLength(MAX_PATH+1), MAX_PATH); 
		csCfgFilePath.ReleaseBuffer(); 
		int nPos = csCfgFilePath.ReverseFind ('\\');
		csCfgFilePath = csCfgFilePath.Left (nPos);
		csCfgFilePath += "\\Config" ;
		BOOL br = WritePrivateProfileString(Name1 ,Name2 ,strvalue ,csCfgFilePath) ;
		if ( !br)
			TRACE("savewrong") ;
	}

	////��õ�ǰĿ¼·��
	//static CString GetLocalPath(){
	//	CString csCfgFilePath;
	//	GetModuleFileName(NULL, csCfgFilePath.GetBufferSetLength(MAX_PATH+1), MAX_PATH); 
	//	csCfgFilePath.ReleaseBuffer(); 
	//	int nPos = csCfgFilePath.ReverseFind ('\\');
	//	csCfgFilePath = csCfgFilePath.Left (nPos);
	//	return csCfgFilePath;
	//}

	//���.exe·��
	static CString GetExePath()
	{
		CString strPath;
		GetModuleFileName(NULL,strPath.GetBufferSetLength(MAX_PATH+1),MAX_PATH);
		strPath.ReleaseBuffer();
		return strPath;
	}

	//�����Զ�����
	static BOOL SetAutoRun(CString strPath,bool flag)
	{
		CString str;
		HKEY hRegKey;
		BOOL bResult;
		str=_T("Software\\Microsoft\\Windows\\CurrentVersion\\Run");
		if(RegOpenKey(HKEY_LOCAL_MACHINE, str, &hRegKey) != ERROR_SUCCESS) 
			bResult=FALSE;
		else
		{
			_splitpath(strPath.GetBuffer(0),NULL,NULL,str.GetBufferSetLength(MAX_PATH+1),NULL);
			strPath.ReleaseBuffer();
			str.ReleaseBuffer();//str�Ǽ�������
			if (flag){
				if(::RegSetValueEx( hRegKey,str,0,REG_SZ,(CONST BYTE *)strPath.GetBuffer(0),strPath.GetLength() ) != ERROR_SUCCESS)
					bResult=FALSE;
				else
					bResult=TRUE;
			}else{
				if(	::RegDeleteValue(hRegKey,str) != ERROR_SUCCESS)
					bResult=FALSE;
				else
					bResult=TRUE;
			}
			strPath.ReleaseBuffer();
		}
		return bResult;
	}		

#pragma endregion �ļ�����

#pragma region �ַ�������
	//string�滻
	void string_replace(string & strBig, const string & strsrc, const string &strdst)
	{
		string::size_type pos=0;
		string::size_type srclen=strsrc.size();
		string::size_type dstlen=strdst.size();
		while( (pos=strBig.find(strsrc, pos)) != string::npos)
		{
			strBig.replace(pos, srclen, strdst);
			pos += dstlen;
		}
	}

	//C++��spilt����
	void SplitString(const string& s, vector<string>& v, const string& c){
		std::string::size_type pos1, pos2;
		pos2 = s.find(c);
		pos1 = 0;
		while(std::string::npos != pos2){
			v.push_back(s.substr(pos1, pos2-pos1));
			pos1 = pos2 + c.size();
			pos2 = s.find(c, pos1);
		}
		if(pos1 != s.length())
			v.push_back(s.substr(pos1));
	}
	//! ͨ���ļ������ƻ�ȡ�ļ�������������׺
	void getFileName(const string& filepath, string& name,string& lastname){
		vector<string> spilt_path;
		SplitString(filepath, spilt_path, "\\");
		int spiltsize = spilt_path.size();
		string filename = "";
		if (spiltsize != 0){
			filename = spilt_path[spiltsize-1];
			vector<string> spilt_name;
			SplitString(filename, spilt_name, ".");
			int name_size = spilt_name.size();
			if (name_size != 0)
				name = spilt_name[0];
			lastname = spilt_name[name_size-1];
		}
	}
	void getFileName(const string& filepath, string& name){
		vector<string> spilt_path;
		SplitString(filepath, spilt_path, "\\");
		int spiltsize = spilt_path.size();
		string filename = "";
		if (spiltsize != 0){
			filename = spilt_path[spiltsize-1];
			vector<string> spilt_name;
			SplitString(filename, spilt_name, ".");
			int name_size = spilt_name.size();
			if (name_size != 0)
				name = spilt_name[0];
		}
	}

#pragma endregion �ַ�������

#pragma region excel����
	//////////////////////////////////////////////////////////////////////////////
	//���ƣ�GetExcelDriver
	//���ܣ���ȡODBC��Excel����
	//���ߣ��쾰��(jingzhou_xu@163.net)
	//��֯��δ��������(Future Studio)
	//���ڣ�2002.9.1
	/////////////////////////////////////////////////////////////////////////////
	CString GetExcelDriver()
	{
		char szBuf[2001];
		WORD cbBufMax = 2000;
		WORD cbBufOut;
		char *pszBuf = szBuf;
		CString sDriver;

		// ��ȡ�Ѱ�װ����������(������odbcinst.h��)
		if (!SQLGetInstalledDrivers(szBuf, cbBufMax, &cbBufOut))
			return "";

		// �����Ѱ�װ�������Ƿ���Excel...
		do
		{
			if (strstr(pszBuf, "Excel") != 0)
			{
				//���� !
				sDriver = CString(pszBuf);
				break;
			}
			pszBuf = strchr(pszBuf, '\0') + 1;
		}
		while (pszBuf[1] != '\0');

		return sDriver;
	}

	///////////////////////////////////////////////////////////////////////////////
	//	BOOL MakeSurePathExists( CString &Path,bool FilenameIncluded)
	//	������
	//		Path				·��
	//		FilenameIncluded	·���Ƿ�����ļ���
	//	����ֵ:
	//		�ļ��Ƿ����
	//	˵��:
	//		�ж�Path�ļ�(FilenameIncluded=true)�Ƿ����,���ڷ���TURE�������ڷ���FALSE
	//		�Զ�����Ŀ¼
	//
	///////////////////////////////////////////////////////////////////////////////
	BOOL MakeSurePathExists( CString &Path,bool FilenameIncluded)
	{
		int Pos=0;
		while((Pos=Path.Find('\\',Pos+1))!=-1)
			CreateDirectory(Path.Left(Pos),NULL);
		if(!FilenameIncluded)
			CreateDirectory(Path,NULL);
		//	return ((!FilenameIncluded)?!_access(Path,0):
		//	!_access(Path.Left(Path.ReverseFind('\\')),0));
		return !_access(Path,0);
	}

	//���Ĭ�ϵ��ļ���
	//2018��6��26�� ��������Ϊ��������·��������
	BOOL GetDefaultXlsFileName(CString& sExcelFile)
	{
		/////Ĭ���ļ�����yyyymmddhhmmss.xls
		CString timeStr;
		CTime day;
		day=CTime::GetCurrentTime();
		int filenameday,filenamemonth,filenameyear,filehour,filemin,filesec;
		filenameday=day.GetDay();//dd
		filenamemonth=day.GetMonth();//mm�·�
		filenameyear=day.GetYear();//yyyy
		filehour=day.GetHour();//hh
		filemin=day.GetMinute();//mm����
		filesec=day.GetSecond();//ss
		timeStr.Format("%04d%02d%02d%02d%02d%02d",filenameyear,filenamemonth,filenameday,filehour,filemin,filesec);
		if (sExcelFile == "")
		{
			sExcelFile =  timeStr + ".xls"; //��ȡ���ʱ����ļ�����
		}else{
			sExcelFile = sExcelFile+".xls";
		}
		
		//��ѡ��·������
		CString pathName; 
		CString defaultDir = _T("C:\\outtest");
		CString fileName=sExcelFile;
		CString szFilters= _T("xls(*.xls)");
		CFileDialog dlg(FALSE,defaultDir,fileName,OFN_HIDEREADONLY|OFN_READONLY,szFilters,NULL);
		if(dlg.DoModal()==IDOK){
			//��ñ���λ��
			pathName = dlg.GetPathName();
		}else{
			return FALSE;
		}

		sExcelFile = pathName;
		return TRUE;
	}

	///////////////////////////////////////////////////////////////////////////////
	//	void GetExcelDriver(CListCtrl* pList, CString strTitle)
	//	������
	//		pList		��Ҫ������List�ؼ�ָ��
	//		strTitle	���������ݱ����
	//	˵��:
	//		����CListCtrl�ؼ���ȫ�����ݵ�Excel�ļ���Excel�ļ������û�ͨ�������Ϊ��
	//		�Ի�������ָ����������ΪstrTitle�Ĺ�������List�ؼ��ڵ��������ݣ�����
	//		��������������ı�����ʽ���浽Excel�������С��������й�ϵ��
	//	
	//	edit by [r]@dotlive.cnblogs.com
	//  2016��8��12�� �޸�Ϊ���Ա��������ģʽ
	///////////////////////////////////////////////////////////////////////////////
	CString ExportListToExcel(CString  sExcelFile,CListCtrl* pList, CString strTitle)
	{
		CString warningStr;
		if (pList->GetItemCount ()>0) {	
			CDatabase database;
			
			
			CString sSql;
			CString tableName = strTitle;

			// �����Ƿ�װ��Excel���� "Microsoft Excel Driver (*.xls)" 
			CString sDriver;
			sDriver = GetExcelDriver();
			if (sDriver.IsEmpty())
			{
				// û�з���Excel����
				AfxMessageBox("û�а�װExcel!\n���Ȱ�װExcel�������ʹ�õ�������!");
				return NULL;
			}

			///Ĭ���ļ���
		/*	CString sExcelFile; 
			if (!GetDefaultXlsFileName(sExcelFile))
				return NULL;*/

			// �������д�ȡ���ַ���
			sSql.Format("DRIVER={%s};DSN='';FIRSTROWHASNAMES=1;READONLY=FALSE;CREATE_DB=\"%s\";DBQ=%s",sDriver, sExcelFile, sExcelFile);

			// �������ݿ� (��Excel����ļ�)
			if( database.OpenEx(sSql,CDatabase::noOdbcDialog) )
			{
				// ������ṹ
				int i;
				LVCOLUMN columnData;
				CString columnName;
				int columnNum = 0;
				CString strH;
				CString strV;

				sSql = "";
				strH = "";
				columnData.mask = LVCF_TEXT;
				columnData.cchTextMax =100;
				columnData.pszText = columnName.GetBuffer (100);
				for(i=0;pList->GetColumn(i,&columnData);i++)
				{
					if (i!=0)
					{
						sSql = sSql + ", " ;
						strH = strH + ", " ;
					}
					sSql = sSql + " " + columnData.pszText +" TEXT";
					strH = strH + " " + columnData.pszText +" ";
				}
				columnName.ReleaseBuffer ();
				columnNum = i;

				sSql = "CREATE TABLE " + tableName + " ( " + sSql +  " ) ";
				database.ExecuteSQL(sSql);


				// ����������
				int nItemIndex;
				for (nItemIndex=0;nItemIndex<pList->GetItemCount ();nItemIndex++){
					strV = "";
					for(i=0;i<columnNum;i++)
					{
						if (i!=0)
						{
							strV = strV + ", " ;
						}
						strV = strV + " '" + pList->GetItemText(nItemIndex,i) +"' ";
					}

					sSql = "INSERT INTO "+ tableName 
						+" ("+ strH + ")"
						+" VALUES("+ strV + ")";
					database.ExecuteSQL(sSql);
				}

			}      

			// �ر����ݿ�
			database.Close();
			return sExcelFile;
		}
	}
	//2��datesheet��ģʽ
	CString ExportListToExcel(CListCtrl* pList, CString strTitle,CListCtrl* pList2,CString strTitle2)
	{
		CString warningStr;
		if (pList->GetItemCount ()>0) {	
			CDatabase database;
			CString sDriver;
			CString sExcelFile; 
			CString sSql;
			CString tableName = strTitle;
			CString tableName2 = strTitle2;

			// �����Ƿ�װ��Excel���� "Microsoft Excel Driver (*.xls)" 
			sDriver = GetExcelDriver();
			if (sDriver.IsEmpty())
			{
				// û�з���Excel����
				AfxMessageBox("û�а�װExcel!\n���Ȱ�װExcel�������ʹ�õ�������!");
				return NULL;
			}

			///Ĭ���ļ���
			if (!GetDefaultXlsFileName(sExcelFile))
				return NULL;

			// �������д�ȡ���ַ���
			sSql.Format("DRIVER={%s};DSN='';FIRSTROWHASNAMES=1;READONLY=FALSE;CREATE_DB=\"%s\";DBQ=%s",sDriver, sExcelFile, sExcelFile);

			// �������ݿ� (��Excel����ļ�)
			if( database.OpenEx(sSql,CDatabase::noOdbcDialog) )
			{
				// ������ṹ1
				int i;
				LVCOLUMN columnData;
				LVCOLUMN columnData2;
				CString columnName;
				CString columnName2;
				int columnNum = 0;
				CString strH;
				CString strV;

				sSql = "";
				strH = "";
				columnData.mask = LVCF_TEXT;
				columnData.cchTextMax =100;
				columnData.pszText = columnName.GetBuffer (100);
				columnData2.mask = LVCF_TEXT;
				columnData2.cchTextMax =100;
				columnData2.pszText = columnName2.GetBuffer (100);
				// ����������1
				for(i=0;pList->GetColumn(i,&columnData);i++)
				{
					if (i!=0)
					{
						sSql = sSql + ", " ;
						strH = strH + ", " ;
					}
					sSql = sSql + " " + columnData.pszText +" TEXT";
					strH = strH + " " + columnData.pszText +" ";
				}
				columnName.ReleaseBuffer ();
				columnNum = i;

				sSql = "CREATE TABLE " + tableName + " ( " + sSql +  " ) ";
				database.ExecuteSQL(sSql);

				
				int nItemIndex;
				for (nItemIndex=0;nItemIndex<pList->GetItemCount();nItemIndex++){
					strV = "";
					for(i=0;i<columnNum;i++)
					{
						if (i!=0)
						{
							strV = strV + ", " ;
						}
						strV = strV + " '" + pList->GetItemText(nItemIndex,i) +"' ";
					}

					sSql = "INSERT INTO "+ tableName 
						+" ("+ strH + ")"
						+" VALUES("+ strV + ")";
					database.ExecuteSQL(sSql);
				}
				//����������2
				sSql = "";
				strH="";
				int columnNum2 = 0;
			
				for(int i=0;pList2->GetColumn(i,&columnData2);i++)
				{
					if (i!=0)
					{
						sSql = sSql + ", " ;
						strH = strH + ", " ;
					}
					sSql = sSql + " " + columnData2.pszText +" TEXT";
					strH = strH + " " + columnData2.pszText +" ";
				}
				columnName2.ReleaseBuffer ();
				columnNum2 = i;

				sSql = "CREATE TABLE " + tableName2 + " ( " + sSql +  " ) ";
				database.ExecuteSQL(sSql);
			 
				for (nItemIndex=0;nItemIndex<pList2->GetItemCount ();nItemIndex++){
					strV = "";
					for(i=0;i<columnNum2;i++)
					{
						if (i!=0)
						{
							strV = strV + ", " ;
						}
						strV = strV + " '" + pList2->GetItemText(nItemIndex,i) +"' ";
					}

					sSql = "INSERT INTO "+ tableName2 
						+" ("+ strH + ")"
						+" VALUES("+ strV + ")";
					database.ExecuteSQL(sSql);
				}
			}      
			// �ر����ݿ�
			database.Close();

			warningStr.Format("�����ļ�������%s!",sExcelFile);
			AfxMessageBox(warningStr);
			return sExcelFile;
		}
	}
#pragma endregion excel����

}