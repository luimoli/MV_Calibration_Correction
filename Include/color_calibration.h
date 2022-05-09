#pragma once
#include "windows.h"
#include <fstream>
#include <opencv.hpp>
#include <opencv2/mcc.hpp>

//  test!!!!
class ColorCalibration {
public:
	~ColorCalibration() = default;
	void CalculateWBGain(cv::Mat& charts_rgb, float wb_gain[3]);
	float CalculateYGain(cv::Mat& WB_charts_linearrgb);
	cv::Mat CalculateCCM(cv::Mat& charts_rgb, int ccm_type, double cc_value[24][3]);
	void WriteConfig(std::string data_path, cv::Mat& ccm, double cct, int ccm_format);
	float CalculateCCT(float wb_gain[3]);
private:

public:
	//cv::Mat charts_rgb;
	//cv::Mat detected_image;
	//cv::Mat WB_charts_linearrgb;
	//cv::Mat Y_WB_charts_linearrgb;
	//double R_gain;
	//double G_gain;
	//double B_gain;
	//float Y_gain;
	//double cct;
	//cv::Mat ccm;
	//cv::Mat white_point = cv::Mat::zeros(cv::Size(1, 1), CV_32FC3);
	double charts_19_mean = 0.9077429;
	double charts_20_mean = 0.58816046;
	//int CCM_format = -1;
	//cv::ccm::ColorCorrectionModel model;
};