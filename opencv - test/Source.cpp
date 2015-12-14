#include <cstdlib>
#include <opencv2\opencv.hpp>
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <vector>
#include <math.h>

using namespace cv;

const unsigned NUM_CARDS = 56;
const float CARD_SIZE_X = 250;
const float CARD_SIZE_Y = 350;

bool compareContourAreas ( std::vector<cv::Point> contour1, std::vector<cv::Point> contour2 ) {
    double i = fabs( contourArea(cv::Mat(contour1)) );
    double j = fabs( contourArea(cv::Mat(contour2)) );
    return !( i < j );
}

void rectify(Point2f points[4]) {
	float sum[4];
	float diff[4];
	for(unsigned i=0;i<4;i++){
		sum[i] = points[i].x+points[i].y;//std::cout<<"sum["<<i<<"] = "<<sum[i]<<std::endl;
		diff[i] = points[i].x-points[i].y;//std::cout<<"diff["<<i<<"] = "<<diff[i]<<std::endl;
	}
	float smallest_sum, smallest_diff, biggest_sum, biggest_diff;
	smallest_sum=smallest_diff=10000;
	biggest_sum=biggest_diff=-10000;
	Point2f smallest_sum_p, smallest_diff_p, biggest_sum_p, biggest_diff_p;
	for(unsigned i=0;i<4;i++){
		if (sum[i]<smallest_sum){smallest_sum=sum[i];smallest_sum_p=points[i];}
		if (sum[i]>biggest_sum){biggest_sum=sum[i];biggest_sum_p=points[i];}
		if (diff[i]<smallest_diff){smallest_diff=diff[i];smallest_diff_p=points[i];}
		if (diff[i]>biggest_diff){biggest_diff=diff[i];biggest_diff_p=points[i];}
	}
	//for(unsigned i=0;i<4;i++)std::cout<<"points["<<i<<"] = "<<points[i]<<" sum:"<<sum[i]<<" diff:"<<diff[i]<<std::endl;
	points[0] = smallest_sum_p;
	points[3] = smallest_diff_p;
	points[2] = biggest_sum_p;
	points[1] = biggest_diff_p;
	//for(unsigned i=0;i<4;i++)std::cout<<"points["<<i<<"] = "<<points[i]<<std::endl;
}

void rectify2(Point2f points[4]) {
	Point2f pA;
	float best_dis=-1,this_dis;
	unsigned index=0;
	//distancias p->p2
	for(unsigned i=0;i<3;i++){
		this_dis = norm(points[i]-points[i+1]);
		if(this_dis>best_dis){
			best_dis = this_dis;
			index=i;
			pA = points[i];
		}
	}
	for(unsigned i=0;i<4;i++)std::cout<<"points["<<i<<"] = "<<points[i]<<std::endl;
	if(index==0) {
		points[0] = points[1];
		points[1] = points[2];
		points[2] = points[3];
		points[3] = pA;
	}
	else if(index==2) {
		points[0] = points[1];
		points[1] = points[2];
		points[2] = points[3];
		points[3] = pA;
	}
	for(unsigned i=0;i<4;i++)std::cout<<"points["<<i<<"] = "<<points[i]<<std::endl;
}

void rectify3(Point2f points[4]) {
	//fix horizontal card
	if(norm(points[0]-points[1]) > norm(points[1]-points[2])) {
		Point2f firstP = points[0];
		points[0] = points[1];
		points[1] = points[2];
		points[2] = points[3];
		points[3] = firstP;
	}
	//mirror Y axis
	Point2f temp = points[0];
	points[0] = points[1];
	points[1] = temp;
	temp = points[2];
	points[2] = points[3];
	points[3] = temp;
}

void toPoints( std::vector<Point> approx, Point2f rect_points[4] ) {
	for(unsigned i=0; i<4; i++)
		rect_points[i] = Point2f(approx[i]);
	rectify3(rect_points);
}

void main() {

	Mat img, gray, blur, flag, thresh;
	std::vector<std::vector<Point>> contours, scontours;
	std::vector<Vec4i> hierarchy;

	// load image
	img = imread("cards_deck_fixed.png");

	// grayscale
	cvtColor(img,gray,COLOR_BGR2GRAY);

	// blur ?
	GaussianBlur(gray,blur,Size(1,1),0,0,1000);

	//threshold
	threshold(blur,thresh,120,255,THRESH_BINARY);
	threshold(blur,flag,120,255,THRESH_BINARY);


	//contours
	findContours(thresh,contours,hierarchy,RETR_TREE,CHAIN_APPROX_SIMPLE,Point(0,0));
	std::sort(contours.begin(), contours.end(), compareContourAreas);
	
	
	//find rectangles
	std::vector<Point> card, approx, r;
	double peri;
	RotatedRect rect;
	Mat transform, warp;
	Point2f h[4] = {Point2f(0,0),Point2f(CARD_SIZE_X-1,0),Point2f(CARD_SIZE_X-1,CARD_SIZE_Y-1),Point2f(0,CARD_SIZE_Y-1)};
	Point2f rect_points[4];
	Mat warps[NUM_CARDS];

	for(unsigned i=0; i<NUM_CARDS; i++) {
	  card = contours[i];
      peri = arcLength(card, true);
	  approxPolyDP(card, approx, 0.02*peri, true);//std::cout<<"aprox:\n"<<approx<<std::endl;
	  //rect = minAreaRect(approx);
	  //rect.points( rect_points);
	  toPoints( approx, rect_points );//std::cout<<"rect_points:\n";for(unsigned i=0; i<4; i++) std::cout<<rect_points[i]<<std::endl;
	  transform = getPerspectiveTransform(rect_points,h);
	  transform = transform*-1;
	  warpPerspective( img, warp, transform, Size(CARD_SIZE_X,CARD_SIZE_Y));
	  warps[i]=warp.clone();
	}


	//window
	namedWindow( "OpenCV", WINDOW_AUTOSIZE );

	//show image
	
	/*// Draw canny
	imshow( "OpenCV", canny );
	*/

	/*// Draw contours
	Mat drawing = Mat::zeros( canny.size(), CV_8UC3 );
	RNG rng(12345);
	for( int i = 0; i< contours.size(); i++ ){
		Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
		drawContours( drawing, contours, i, color, 2, 8, hierarchy, 0, Point() );
	}
	imshow( "OpenCV", drawing);
	*/

	/// Draw contours + rotated rects + ellipses
	RNG rng(12345);
	Mat drawing = Mat::zeros( thresh.size(), CV_8UC3 );
	for( size_t i = 0; i< contours.size(); i++ ){
		Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
		// contour
		drawContours( drawing, contours, (int)i, color, 1, 8, std::vector<Vec4i>(), 0, Point() );
	}
	for( size_t i = 0; i<NUM_CARDS; i++ ){
		imshow("warp"+i,warps[i]);
	}
	imshow( "OpenCV", drawing );

	//allow opencv window to stay open
	waitKey(0);
}