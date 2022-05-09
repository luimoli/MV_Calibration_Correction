#pragma once
#include "windows.h"
#include <opencv.hpp>
#include <opencv2/mcc.hpp>


class ColorCheckerFunc {
 public:
  ~ColorCheckerFunc() = default;
  void DetectChecker(const cv::Mat& image, int input_format, float resize_ratio);  // input_format:0: linear rgb  1:srgb
  void GetCheckerCentroid(cv::Ptr<cv::mcc::CChecker>& checker);
  //void Prepare(cv::Rect rect);
  //void CalculateWBGain();
  //void CalculateYGain();

private:

public:
  cv::Mat original_charts_linearrgb;
  cv::Mat original_charts_srgb;
  cv::Mat WB_charts_rgb;
  cv::Mat Y_WB_charts_rgb;
  cv::Mat detected_image;
  float R_gain;
  float G_gain;
  float B_gain;
  float Y_gain;
  BOOL is_colorchecker;
  cv::Mat white_point = cv::Mat::zeros(cv::Size(1, 1), CV_32FC3);
  std::vector<cv::Point2f> centroid;
};
