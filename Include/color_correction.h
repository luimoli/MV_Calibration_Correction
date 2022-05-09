#pragma once
#include "windows.h"
#include <opencv.hpp>
#include <opencv2/mcc.hpp>


class ColorCorrection {
public:
	~ColorCorrection() = default;
	//void SetCCM(cv::Mat& ccm, int ccm_format);
	//void SetWBgain(double R_gain, double G_gain, double B_gain);
	//void SetYgain(double Y_gain);
	cv::Mat CalculateWhitePoint(float wb_gain[3]);
	void PredictWBgain(cv::Mat& white_paper, float wb_gain[3], int wb_format);
	cv::Mat PredictCCM(cv::Mat& white_point);
	void CorrectImage(cv::Mat& image, int input_format, cv::Mat& calibrated_image, int do_WB, int do_Ygain, int do_CCM, int ccm_position, float wb_gain[3], float Y_gain, cv::Mat& ccm, int do_exposure_gain);
	float CalculateCCT(cv::Mat& white_point);
	void GetCalibCCM(std::string config_path);
private:

public:
	//double R_gain = 1;
	//double G_gain = 1;
	//double B_gain = 1;
	//double Y_gain = 1;
	//double cct;
	//cv::Mat ccm = cv::Mat::zeros(cv::Size(3, 3), CV_64FC1);
	//cv::Mat ccm;

	//cv::Mat white_point = cv::Mat::zeros(cv::Size(1, 1), CV_32FC3);
	//int ccm_format = -1;
	std::vector<double> cct_list;
	std::vector<cv::Mat> ccm_list;
	//cv::ccm::ColorCorrectionModel model;
};