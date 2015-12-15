#include <cstdlib>
#include <opencv2\opencv.hpp>
//#include <opencv2\imgcodecs.hpp>
//#include <opencv2\highgui\highgui.hpp>
//#include <vector>
//#include <math.h>

using namespace cv;

const unsigned NUM_CARDS = 4;
const unsigned NUM_DECK_CARDS = 52;
const int CARD_SIZE_X = 125;
const int CARD_SIZE_Y = 175;

bool compareContourAreas ( std::vector<cv::Point> contour1, std::vector<cv::Point> contour2 ) {
    double j = fabs( contourArea(cv::Mat(contour1)) );
    double i = fabs( contourArea(cv::Mat(contour2)) );
    return ( i < j );
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
	double best_dis=-1,this_dis;
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

void get_cards( Mat img, Mat warps[], unsigned num_cards) {

	Mat gray, blur, thresh;
	std::vector<std::vector<Point>> contours, scontours;
	std::vector<Vec4i> hierarchy;

	// grayscale
	cvtColor(img, gray, CV_BGR2GRAY);

	// blur ?
	GaussianBlur(gray,blur,Size(1,1),0,0,1000);

	//threshold
	threshold(blur,thresh,120,255,THRESH_BINARY);

	//contours
	findContours(thresh,contours,hierarchy,RETR_TREE,CHAIN_APPROX_SIMPLE,Point(0,0));

	//order by size (cards will be first)
	std::sort(contours.begin(), contours.end(), compareContourAreas);
	
	
	//find rectangles
	std::vector<Point> card, approx, r;
	double peri;
	RotatedRect rect;
	Mat transform, warp;
	Point2f h[4] = {Point2f(0,0),Point2f(CARD_SIZE_X-1,0),Point2f(CARD_SIZE_X-1,CARD_SIZE_Y-1),Point2f(0,CARD_SIZE_Y-1)};
	Point2f rect_points[4];

	for(unsigned i=0; i<num_cards; i++) {
	  card = contours[i];
      peri = arcLength(card, true);
	  approxPolyDP(card, approx, 0.05*peri, true);//std::cout<<"aprox:\n"<<approx<<std::endl;
	  //rect = minAreaRect(approx);
	  //rect.points( rect_points);
	  toPoints( approx, rect_points );//std::cout<<"rect_points:\n";for(unsigned i=0; i<4; i++) std::cout<<rect_points[i]<<std::endl;
	  transform = getPerspectiveTransform(rect_points,h);
	  warpPerspective( img, warp, transform, Size(CARD_SIZE_X,CARD_SIZE_Y));
	  warps[i]=warp.clone();
	}
}

struct card {
	std::string suit;
	std::string value;
	Mat view;
};

void load_deck( Mat img, card cards[]) {

	std::string suits[4] = {"H","D","C","S"};
	std::string value[13] = {"A","2","3","4","5","6","7","8","9","10","J","Q","K"};
	Mat gray, blur, thresh;
	Mat warps[NUM_DECK_CARDS];
	get_cards( img, warps, NUM_DECK_CARDS);
	//                          0    1    2    3    4    5    6    7    8    9    10   11   12   13  14   15    16   17    18   19   20   21   22   23   24  25   26   27    28   29    30   31   32   33   34   35   36   37   38   39    40   41   42   43  44   45   46   47    48  49    50   51    52
	std::string values[52] = { "A", "2", "A", "3", "A", "2", "4", "A", "2", "5", "3", "9", "6", "2", "4", "10", "7", "3", "J", "Q", "5", "K", "9", "3", "8", "4", "6", "10", "7", "J", "8", "4", "5", "K", "9", "Q", "6", "10", "7", "5", "8", "9", "J", "K", "6", "Q", "10", "7", "J", "Q", "K", "8"};
	std::string naipes[52] = { "H", "H", "D", "H", "C", "D", "H", "S", "C", "H", "D", "H", "H", "S", "D",  "H", "H", "C", "H", "H", "D", "H", "D", "S", "H", "C", "D",  "D", "D", "D", "D", "S", "C", "D", "C", "D", "C",  "C", "C", "S", "C", "S", "C", "C", "S", "C",  "S", "S", "S", "S", "S", "S"};

	for( unsigned i=0; i<NUM_DECK_CARDS; i++) {
		cards[i].suit = values[i];//std::to_string(i);
		cards[i].value = naipes[i];//std::to_string(i);
		cards[i].view = warps[i];
	}
}

void main() {

	Mat img, deck_img;

	// load images
	img = imread("hearts_play_rotated.jpg");
	deck_img = imread("cards_deck_fixed.png");

	//get cards in input image
	Mat cards[NUM_CARDS] = {};
	get_cards(img, cards, NUM_CARDS);

	//load deck
	card deck[NUM_DECK_CARDS] = {};
	load_deck(deck_img, deck);


	//find wich cards u have
	/*Mat diff[NUM_CARDS][NUM_DECK_CARDS];
	for(unsigned i=0; i<NUM_CARDS; i++) {
		for(unsigned j=0; j<NUM_DECK_CARDS; j++) {
			Mat incard,indeckcard,diffcard;
			//prepare
			cvtColor(cards[i], incard, CV_BGR2GRAY);
			GaussianBlur(incard,incard,Size(7,7),0,0);

			cvtColor(deck[j].view, indeckcard, CV_BGR2GRAY);
			GaussianBlur(indeckcard,indeckcard,Size(1,1),0,0);

			//calc diff
			absdiff( incard, indeckcard, diffcard);

			//threshold
			threshold(diffcard,diffcard,30,255,THRESH_BINARY);

			//show
			imshow("card", incard);
			imshow("deckcard", indeckcard);
			imshow("diff", diffcard);
			waitKey(0);
		}
	}
	/**/
	/*img1 = cv2.GaussianBlur(img1,(5,5),5)
  img2 = cv2.GaussianBlur(img2,(5,5),5)    
  diff = cv2.absdiff(img1,img2)  
  diff = cv2.GaussianBlur(diff,(5,5),5)*/

	//get winner according to rules


	//print over the cards the winner



	/*//window
	namedWindow( "OpenCV", WINDOW_AUTOSIZE );
	/**/

	//show image
	imshow("inputImage", img);
	//show deck
	imshow("deck", deck_img);
	/**/
	
	/*// Draw canny
	imshow( "OpenCV", canny );
	/**/

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
	/*RNG rng(12345);
	Mat drawing = Mat::zeros( thresh.size(), CV_8UC3 );
	for( size_t i = 0; i< contours.size(); i++ ){
		Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
		// contour
		drawContours( drawing, contours, (int)i, color, 1, 8, std::vector<Vec4i>(), 0, Point() );
	}
	imshow( "OpenCV", drawing );*/

	//Draw cards
	for( unsigned i = 0; i<NUM_CARDS; i++ ){
		std::string windowName = "card "+std::to_string(i);
		std::cout<<"printing card number "<<i<<": "<<windowName<<std::endl;
		imshow(windowName,cards[i]);
	}
	/**/

	//Draw deck
	for( unsigned i = 0; i<NUM_DECK_CARDS; i++ ){
		std::string windowName = deck[i].suit+" "+deck[i].value;
		std::cout<<"printing deck card number "<<i<<": "<<windowName<<std::endl;
		imshow(windowName,deck[i].view);
	}
	/**/

	//allow opencv window to stay open
	waitKey(0);
}