#include "color_calibration.h"
#include "windows.h"
#include <opencv.hpp>
#include <opencv2/mcc.hpp>

float ColorCalibration::CalculateCCT(float wb_gain[3])
{
    cv::Mat white_point = cv::Mat::zeros(cv::Size(1, 1), CV_32FC3);
    white_point.at<cv::Vec3f>(0, 0)[0] = float(1 / wb_gain[0]);
    white_point.at<cv::Vec3f>(0, 0)[1] = float(1 / wb_gain[1]);
    white_point.at<cv::Vec3f>(0, 0)[2] = float(1 / wb_gain[2]);
    cv::Mat white_point_XYZ;
    cv::cvtColor(white_point, white_point_XYZ, cv::COLOR_RGB2XYZ);
    float x = white_point_XYZ.at<cv::Vec3f>(0, 0)[0] / (white_point_XYZ.at<cv::Vec3f>(0, 0)[0]
        + white_point_XYZ.at<cv::Vec3f>(0, 0)[1] + white_point_XYZ.at<cv::Vec3f>(0, 0)[2]);
    float y = white_point_XYZ.at<cv::Vec3f>(0, 0)[1] / (white_point_XYZ.at<cv::Vec3f>(0, 0)[0]
        + white_point_XYZ.at<cv::Vec3f>(0, 0)[1] + white_point_XYZ.at<cv::Vec3f>(0, 0)[2]);
    float n = (x - 0.3320) / (y - 0.1858);
    float cct = -449 * pow(n, 3) + 3525 * pow(n, 2) - 6823.3 * n + 5520.33;
    return cct;
}

void ColorCalibration::CalculateWBGain(cv::Mat& charts_linearrgb, float wb_gain[3])
{
    double R_19 = charts_linearrgb.at<cv::Vec3d>(0, 18)[0];
    double G_19 = charts_linearrgb.at<cv::Vec3d>(0, 18)[1];
    double B_19 = charts_linearrgb.at<cv::Vec3d>(0, 18)[2];
    double R_20 = charts_linearrgb.at<cv::Vec3d>(0, 19)[0];
    double G_20 = charts_linearrgb.at<cv::Vec3d>(0, 19)[1];
    double B_20 = charts_linearrgb.at<cv::Vec3d>(0, 19)[2];
    double max_19 = std::max(std::max(R_19, G_19), B_19);
    double max_20 = std::max(std::max(R_20, G_20), B_20);
    wb_gain[0] = 0.5 * (max_19 / R_19) + 0.5 * (max_20 / R_20);
    wb_gain[1] = 0.5 * (max_19 / G_19) + 0.5 * (max_20 / G_20);
    wb_gain[2] = 0.5 * (max_19 / B_19) + 0.5 * (max_20 / B_20);
}

cv::Mat getColorChecker(double* checker, int row)
{
    cv::Mat res(row, 1, CV_64FC3);
    for (int i = 0; i < row; ++i)
    {
        res.at<cv::Vec3d>(i, 0) = cv::Vec3d(checker[3 * i], checker[3 * i + 1], checker[3 * i + 2]);
    }
    return res;
}


float ColorCalibration::CalculateYGain(cv::Mat& WB_charts_linearrgb)
{
    double R_19 = WB_charts_linearrgb.at<cv::Vec3d>(0, 18)[0];
    double G_19 = WB_charts_linearrgb.at<cv::Vec3d>(0, 18)[1];
    double B_19 = WB_charts_linearrgb.at<cv::Vec3d>(0, 18)[2];
    double R_20 = WB_charts_linearrgb.at<cv::Vec3d>(0, 19)[0];
    double G_20 = WB_charts_linearrgb.at<cv::Vec3d>(0, 19)[1];
    double B_20 = WB_charts_linearrgb.at<cv::Vec3d>(0, 19)[2];

    double mean_19 = (R_19 + G_19 + B_19) / 3;
    double mean_20 = (R_20 + G_20 + B_20) / 3;

    float Y_gain = 0.5 * (this->charts_19_mean / mean_19) + 0.5 * (this->charts_20_mean / mean_20);
    return Y_gain;
}

cv::Mat ColorCalibration::CalculateCCM(cv::Mat& charts_rgb, int ccm_type, double cc_value[24][3])
{
    //input_format: 0:linear rgb 1:srgb
    cv::Mat input_charts_rgb;
    input_charts_rgb = charts_rgb.clone();
    cv::Mat cc_value_mat = getColorChecker(*cc_value, 24);
    //auto model = cv::ccm::ColorCorrectionModel(input_charts_rgb, cv::ccm::COLORCHECKER_Macbeth);
    auto model = cv::ccm::ColorCorrectionModel(input_charts_rgb, cc_value_mat, cv::ccm::COLOR_SPACE_Lab_D65_2);

    model.setColorSpace(cv::ccm::COLOR_SPACE_sRGB);
    if (ccm_type == 0)
    {
        model.setCCM_TYPE(cv::ccm::CCM_3x3);
        cv::Mat weights_list_ = (cv::Mat_<double>(24, 1) <<
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1);
        //cv::Mat weights_list_ = (cv::Mat_<double>(24, 1) <<
        //    0, 0, 0, 0, 0, 0,
        //    1, 0, 0, 0, 0, 1,
        //    0, 0, 1, 0, 0, 0,
        //    1, 0, 0, 0, 0, 0);

        //cv::Mat weights_list_ = (cv::Mat_<double>(24, 1) <<
        //    2.3358736,  2.2939215,  1.699611,   1.8576946,  1.7156681,  1.8140517,
        //    2.5880995,  1.2222651,  2.6719851,  1.6625049,  1.7388835,  2.3147871,
        //    1.,         1.4906853,  3.,        2.0886416,  2.047327,   1.510445,
        //    2.1234856,  2.1239724,  2.122768,   2.1285148,  2.1265082,  2.1312704);

          //cv::Mat weights_list_ = (cv::Mat_<double>(24, 1) <<
          //    7.0114317,   6.822647,    4.148249  ,  4.8596253 ,  4.2205067 ,  4.663233,
          //    8.146448,    2.000193 ,   8.523933  ,  3.9812722 ,  4.324976 ,   6.916542,
          //    1.,          3.208084,   20.      ,    5.8988876 ,  5.712971  ,  3.2970028,
          //    6.055685,    6.0578756,   6.052456  ,  6.0783167,   6.0692873  , 6.090717);

        
        model.setWeightsList(weights_list_);
    }
    else if (ccm_type == 1)
    {
        model.setCCM_TYPE(cv::ccm::CCM_4x3);
        //cv::Mat weights_list_ = (cv::Mat_<double>(24, 1) <<
        //    1, 1, 1, 1, 1, 1,
        //    1, 1, 1, 1, 1, 1,
        //    1, 1, 1, 1, 1, 1,
        //    10, 10, 1, 1, 1, 1);
        //model.setWeightsList(weights_list_);
        auto a = model.getWeights();
    }

    model.setDistance(cv::ccm::DISTANCE_CIE2000);
    model.setLinear(cv::ccm::LINEARIZATION_GAMMA);
    model.setLinearGamma(1);
    model.setLinearDegree(3);
    //model.setWeightCoeff(3);
    model.run();
    cv::Mat ccm = model.getCCM();
    auto loss = model.getLoss();
    std::cout << "loss:" << loss << std::endl;
    std::cout << ccm << std::endl;
    return ccm;
}

void ColorCalibration::WriteConfig(std::string data_path, cv::Mat& ccm, double cct, int ccm_format)
{
    std::cout << data_path << std::endl;
    std::ofstream fout(data_path, std::ios::binary | std::ios::app);
    fout.write((char*)&ccm_format, sizeof(int));
    fout.write((char*)&cct, sizeof(double));
    fout.write((char*)&ccm.at<double>(0, 0), sizeof(double));
    fout.write((char*)&ccm.at<double>(0, 1), sizeof(double));
    fout.write((char*)&ccm.at<double>(0, 2), sizeof(double));
    fout.write((char*)&ccm.at<double>(1, 0), sizeof(double));
    fout.write((char*)&ccm.at<double>(1, 1), sizeof(double));
    fout.write((char*)&ccm.at<double>(1, 2), sizeof(double));
    fout.write((char*)&ccm.at<double>(2, 0), sizeof(double));
    fout.write((char*)&ccm.at<double>(2, 1), sizeof(double));
    fout.write((char*)&ccm.at<double>(2, 2), sizeof(double));
    if (ccm_format == 1)
    {
        fout.write((char*)&ccm.at<double>(3, 0), sizeof(double));
        fout.write((char*)&ccm.at<double>(3, 1), sizeof(double));
        fout.write((char*)&ccm.at<double>(3, 2), sizeof(double));
    }
}