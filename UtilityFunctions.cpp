/* Utility functions usually for debugging
 */



#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/features2d/features2d.hpp>



#include "UtilityFunctions.hpp"
#include <stdio.h>
#include <sys/stat.h>




void debugShow(cv::Mat& frame)
{

    cv::Mat cvFlippedImg;
    //cv::bitwise_not(frame, cvFlippedImg);
    cvFlippedImg = frame;

    const char* source_window = "DebugSource";
    cv::namedWindow( source_window, CV_WINDOW_NORMAL );

    cv::imshow(source_window, cvFlippedImg);
    cv::waitKey(0);

}

/*Workarounds for fermi grid old GCC*/
//bool bubbleBRectSort(cv::Rect a, cv::Rect b){return a.area()>b.area();}

/*Code to show histogram*/
void showHistogramImage(cv::Mat& frame)
{

    /* Establish the number of bins */
    int histSize = 256;

    /* Set the ranges ( for colour intensities) ) */
    float range[] = { 0, 256 } ;
    const float* histRange = { range };

    /*Extra Parameters*/
    bool uniform = true;
    bool accumulate = false;

    /*Calculation of the histogram*/
    cv::Mat frame_hist;
    calcHist( &frame, 1, 0, cv::Mat(), frame_hist, 1, &histSize, &histRange, uniform, accumulate );

    cv::Size s = frame_hist.size();
    int rows = s.height;
    int cols = s.width;

    for (int i=0; i<=256; i++){
        //std::cout<<"Rows: "<<rows<<" Cols: "<<cols<<"\n";
        std::cout<<i<<" "<<frame_hist.at<float>(i)<<"\n";
    }

}

float ImageDynamicRangeSum(cv::Mat& frame, int LowerBand, int HigherBand){

    /* Establish the number of bins */
    int histSize = 256;

    /* Set the ranges ( for colour intensities) ) */
    float range[] = { 0, 256 } ;
    const float* histRange = { range };

    /*Extra Parameters*/
    bool uniform = true;
    bool accumulate = false;

    /*Calculation of the histogram*/
    cv::Mat frame_hist;
    calcHist( &frame, 1, 0, cv::Mat(), frame_hist, 1, &histSize, &histRange, uniform, accumulate );

    cv::Size s = frame_hist.size();
    int rows = s.height;
    int cols = s.width;

    float DynamicRangeSum = 0;

    for (int i=LowerBand; i<=HigherBand; i++){
        //std::cout<<"Rows: "<<rows<<" Cols: "<<cols<<"\n";
        DynamicRangeSum += frame_hist.at<float>(i);
    }

    //printf("Drange: %f\n", DynamicRangeSum);

    return DynamicRangeSum;


}

/*Spinning cursors*/

void advance_cursor() {
  static int pos=0;
  char cursor[4]={'/','-','\\','|'};
  printf("%c\b", cursor[pos]);
  fflush(stdout);
  pos = (pos+1) % 4;
}




/**
 * Get the size of a file.
 * @param filename The name of the file to check size for
 * @return The filesize, or 0 if the file does not exist.
 */
size_t getFilesize(const std::string& filename) {
    struct stat st;
    if(stat(filename.c_str(), &st) != 0) {
        return 0;
    }
    return st.st_size;
}

/**
 * Check if file exists.
 * @param filename The name of the file to check size for
 * @return True if it does exist, False if it doesnt
 */

bool doesFileExist (const std::string& name) {
  struct stat buffer;
  return (stat (name.c_str(), &buffer) == 0);
}


