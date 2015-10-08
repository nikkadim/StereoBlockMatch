#include <iostream>
#include <sstream>
#include <vector>
#include <opencv2/opencv.hpp>
#include <cvImagePipeline.h>
#include "getopt.h"
#include "util.h"

using namespace cvImagePipeline::Filter;

class DrawChessboardCorners : public ImageProcessor {
    cv::Size patternSize;
    int flags;
public:
    DECLARE_CVFILTER;
    DrawChessboardCorners()
        : patternSize(7, 7),
        flags(
                CV_CALIB_CB_ADAPTIVE_THRESH |
                CV_CALIB_CB_NORMALIZE_IMAGE |
                //CV_CALIB_CB_FILTER_QUADS |
                CV_CALIB_CB_FAST_CHECK )
    {}
    void execute() {
        const cv::Mat& in = getInputMat();
        if(in.empty()) {
            return;
        }
        cv::Mat& out = refOutputMat();
        out = in;

        std::vector<cv::Point2f> corners;
        bool patternFound = cv::findChessboardCorners(
                in, patternSize, corners, flags);

        if(patternFound) {
            cv::cornerSubPix(
                    in, corners,
                    cv::Size(11, 11), cv::Size(-1, -1),
                    cv::TermCriteria(
                        CV_TERMCRIT_EPS + CV_TERMCRIT_ITER,
                        30, 0.1));
            /***********************************************
            ***********************************************/
            cv::drawChessboardCorners(
                    out, patternSize,
                    cv::Mat(corners),
                    patternFound);
        }
    }
};
IMPLEMENT_CVFILTER(DrawChessboardCorners);
int main(int argc, char* argv[]) {
    char const* optstring = "L:R:";
    int opt = 0;
    char const* optR = 0;
    char const* optL = 0;
    while((opt = getopt(argc, argv, optstring)) != -1) {
        switch(opt) {
            case 'R':
                optR = optarg;
                break;
            case 'L':
                optL = optarg;
                break;
        }
    }
    int deviceIndexR = toInt(optR, 1);
    int deviceIndexL = toInt(optL, 2);

    cvImagePipeline::Filter::ImgProcSet proc;
    
    // capture right camera, and grayscale
    proc.add("VideoCapture", "capR", false)
        .property("deviceIndex", deviceIndexR);
    proc.add("ColorConverter", "gryR")
        .property("cvtCode", CV_BGR2GRAY);
    proc.add("DrawChessboardCorners", "cornerR");

    // capture left camera, and grayscale
    proc.add("VideoCapture", "capL", false)
        .property("deviceIndex", deviceIndexL);
    proc.add("ColorConverter", "gryL")
        .property("cvtCode", CV_BGR2GRAY);
    proc.add("DrawChessboardCorners", "cornerL");

    // tile both corner-drawn images to a window.
    proc.add("FitInGrid", "fig", false)
        .property("cols", 2).property("rows", 1)
        .property("width", 1280).property("height", 480);

    proc["cornerR"] >> proc["fig"].input("0");
    proc["cornerL"] >> proc["fig"].input("1");

    proc.add("ImageWindow", "wnd")
        .property("windowName", "stereoShot");

    std::cerr << "[SPC] to shoot, [Shift]+[Q] to quit." << std::endl;
    int photo_number = 0;
    std::vector<int> imwrite_params;
    imwrite_params.push_back(CV_IMWRITE_JPEG_QUALITY);
    imwrite_params.push_back(100);

    while(true) {
        proc.execute();
        int c = cvWaitKey(1);
        if(c == 'Q') {
            break;
        } else if(c == ' ') {
            // save L-R images to file.
            const cv::Mat& gryR = proc["gryR"].getOutputMat();
            const cv::Mat& gryL = proc["gryL"].getOutputMat();
            std::stringstream fnR;
            std::stringstream fnL;
            fnR << "cal_" << photo_number << "_R.jpg";
            fnL << "cal_" << photo_number << "_L.jpg";
            cv::imwrite(fnR.str(), gryR, imwrite_params);
            cv::imwrite(fnL.str(), gryL, imwrite_params);
            photo_number++;
            std::cerr << "save right image to " << fnR.str()
                << " and left one to " << fnL.str() << "."
                << std::endl;
        }
    }
    return 0;
}

