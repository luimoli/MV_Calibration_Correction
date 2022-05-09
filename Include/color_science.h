#pragma once
#include "windows.h"
#include <fstream>
#include <opencv.hpp>
#include <opencv2/mcc.hpp>

void CalculateDeltaE(cv::Mat& lab1, cv::Mat& lab2, std::vector<double>& delta_E, std::vector<double>& delta_C);
void EvaluateImage(cv::Mat& iamge, int input_format, std::vector<double>& delta_E, std::vector<double>& delta_C);
double deltaE_CIEDE2000(const cv::Vec3d& lab1, const cv::Vec3d& lab2, const double& kL = 1.0, const double& kC = 1.0, const double& kH = 1.0);
double deltaC_CIE2000(const cv::Vec3d& lab1, const cv::Vec3d& lab2, const double& kL = 1.0, const double& kC = 1.0, const double& kH = 1.0);
void getColorChecker(cv::Mat& ideal_lab);

//cv::Mat ideal_lab = cv::Mat::zeros(24, 1, CV_64FC3);



//class ColorScience {
//public:
//	~ColorScience() = default;
//	void CalculateDeltaE(cv::Mat& lab1, cv::Mat& lab2);
//    void EvaluateImage(cv::Mat& iamge, int input_format);
//	double deltaE_CIEDE2000(const cv::Vec3d& lab1, const cv::Vec3d& lab2, const double& kL = 1.0, const double& kC = 1.0, const double& kH = 1.0);
//    double deltaC_CIE2000(const cv::Vec3d& lab1, const cv::Vec3d& lab2, const double& kL = 1.0, const double& kC = 1.0, const double& kH = 1.0);
//
//private:
//    void getColorChecker();
//public:
//    double ColorChecker2005_LAB_D50_2[24][3] = {
//    { 37.986, 13.555, 14.059 },
//    { 65.711, 18.13, 17.81 },
//    { 49.927, -4.88, -21.925 },
//    { 43.139, -13.095, 21.905 },
//    { 55.112, 8.844, -25.399 },
//    { 70.719, -33.397, -0.199 },
//    { 62.661, 36.067, 57.096 },
//    { 40.02, 10.41, -45.964 },
//    { 51.124, 48.239, 16.248 },
//    { 30.325, 22.976, -21.587 },
//    { 72.532, -23.709, 57.255 },
//    { 71.941, 19.363, 67.857 },
//    { 28.778, 14.179, -50.297 },
//    { 55.261, -38.342, 31.37 },
//    { 42.101, 53.378, 28.19 },
//    { 81.733, 4.039, 79.819 },
//    { 51.935, 49.986, -14.574 },
//    { 51.038, -28.631, -28.638 },
//    { 96.539, -0.425, 1.186 },
//    { 81.257, -0.638, -0.335 },
//    { 66.766, -0.734, -0.504 },
//    { 50.867, -0.153, -0.27 },
//    { 35.656, -0.421, -1.231 },
//    { 20.461, -0.079, -0.973 }};
//
//    cv::Mat ideal_lab = cv::Mat::zeros(24, 1, CV_64FC3);
//    std::vector<double> delta_E;
//    std::vector<double> delta_C;
//
//};