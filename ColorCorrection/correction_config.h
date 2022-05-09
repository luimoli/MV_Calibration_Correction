#pragma once
#include <map>
#include <vector>
#include "windows.h"
#include "stdafx.h"
#include <iostream>


typedef struct BasicConfig {
public:
    std::string ccm_result_directory;
    //std::string ccm_result_directory = "C:/Users/30880/Desktop/four_ccm1.bin";
  
    int input_image_format = 0;      // 0:linear rgb   1:srgb    2.linear rgb with wb     3.linear rgb with wb and Y gain
    int output_image_format = 0;     // 0:linear rgb   1:srgb    2.linear rgb with wb     3.linear rgb with wb and Y gain
    int ccm_position = 0;            // 0:linear rgb   1::srgb
    int ccm_format = 0;              // 0:3x3   1:3x4
    int wb_format = 0;               // 0:colorchecker  1:white paper  2-10:prediction(no support yet)
    int color_checker_type = 3;      // 0:xrite  1:imagetest  2.3nh   3.ColorChecker2005_LAB_D50_2
    int ae_time=55;
    float color_checker_resize_ratio = 1;
    int calib_image_num = 10;
    /*float white_paper_gain[3] = { 1.01, 1, 1.13 };*/
    //float white_paper_gain[3] = { 1.01, 1, 1.05 };
    float white_paper_gain[3] = { 1, 1, 1 };
    float blc=0;

    bool m_debug = TRUE;

    double ColorChecker2005_LAB_D50_2[24][3] = {
    { 37.986, 13.555, 14.059 },
    { 65.711, 18.13, 17.81 },
    { 49.927, -4.88, -21.925 },
    { 43.139, -13.095, 21.905 },
    { 55.112, 8.844, -25.399 },
    { 70.719, -33.397, -0.199 },
    { 62.661, 36.067, 57.096 },
    { 40.02, 10.41, -45.964 },
    { 51.124, 48.239, 16.248 },
    { 30.325, 22.976, -21.587 },
    { 72.532, -23.709, 57.255 },
    { 71.941, 19.363, 67.857 },
    { 28.778, 14.179, -50.297 },
    { 55.261, -38.342, 31.37 },
    { 42.101, 53.378, 28.19 },
    { 81.733, 4.039, 79.819 },
    { 51.935, 49.986, -14.574 },
    { 51.038, -28.631, -28.638 },
    { 96.539, -0.425, 1.186 },
    { 81.257, -0.638, -0.335 },
    { 66.766, -0.734, -0.504 },
    { 50.867, -0.153, -0.27 },
    { 35.656, -0.421, -1.231 },
    { 20.461, -0.079, -0.973 }};
    std::map<int, double[24][3]> color_checker_data;
} Config;

