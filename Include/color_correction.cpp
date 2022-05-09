#include "color_correction.h"
#include "windows.h"
#include "color_checker.h"
#include <opencv.hpp>
#include <opencv2/mcc.hpp>
#include <fstream>

float ColorCorrection::CalculateCCT(cv::Mat& white_point)
{
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


void ColorCorrection::CorrectImage(cv::Mat& image, int input_format, cv::Mat& calibrated_image,
    int do_WB, int do_Ygain, int do_CCM, int ccm_position, float wb_gain[3], float Y_gain, cv::Mat& ccm, int do_exposure_gain)
{

    std::cout << "r_gain:" << wb_gain[0] << std::endl;
    std::cout << "g_gain:" << wb_gain[1] << std::endl;
    std::cout << "b_gain:" << wb_gain[2] << std::endl;
    float exposure_gain;
    if (do_exposure_gain)
    {
        auto mean = cv::mean(image); //BGR
        exposure_gain = (0.0722 * mean[0] + 0.7152 * mean[1] + 0.2126 * mean[2]) /
            (0.0722 * wb_gain[2] * mean[0] + 0.7152 * wb_gain[1] * mean[1] + 0.2126 * wb_gain[0] * mean[2]);
    }
    //float exposure_gain = (1 * mean[0] +1 * mean[1] + 1 * mean[2]) /
    //    (1 * wb_gain[2] * mean[0] + 1 * wb_gain[1] * mean[1] + 1 * wb_gain[0] * mean[2]);

    int rows = image.rows;
    int cols = image.cols;
    calibrated_image = image.clone();
    for (auto row = 0; row < rows; row++) {
        for (auto col = 0; col < cols; col++) {
            double r = image.at<cv::Vec3f>(row, col)[2];
            double g = image.at<cv::Vec3f>(row, col)[1];
            double b = image.at<cv::Vec3f>(row, col)[0];
            if (input_format == 1)
            {
                r = pow(r, 2.2);
                g = pow(g, 2.2);
                b = pow(b, 2.2);
            }
            if (do_WB)
            {
                r = r * wb_gain[0];
                g = g * wb_gain[1];
                b = b * wb_gain[2];



                r = std::min(1.0, r);
                g = std::min(1.0, g);
                b = std::min(1.0, b);
            }
            if (do_Ygain)
            {
                r = r * Y_gain;
                g = g * Y_gain;
                b = b * Y_gain;
                r = std::min(1.0, r);
                g = std::min(1.0, g);
                b = std::min(1.0, b);
            }
            if (do_CCM)
            {
                if (ccm_position == 1)
                {
                    r = pow(r, (1 / 2.2));
                    g = pow(g, (1 / 2.2));
                    b = pow(b, (1 / 2.2));
                }
                double temp_r, temp_g, temp_b;
                if (ccm.rows == 4)
                {
                    temp_r = float(ccm.at<double>(0, 0) * r + ccm.at<double>(1, 0) * g + ccm.at<double>(2, 0) * b + ccm.at<double>(3, 0));
                    temp_g = float(ccm.at<double>(0, 1) * r + ccm.at<double>(1, 1) * g + ccm.at<double>(2, 1) * b + ccm.at<double>(3, 1));
                    temp_b = float(ccm.at<double>(0, 2) * r + ccm.at<double>(1, 2) * g + ccm.at<double>(2, 2) * b + ccm.at<double>(3, 2));
                }
                else
                {
                    temp_r = float(ccm.at<double>(0, 0) * r + ccm.at<double>(1, 0) * g + ccm.at<double>(2, 0) * b);
                    temp_g = float(ccm.at<double>(0, 1) * r + ccm.at<double>(1, 1) * g + ccm.at<double>(2, 1) * b);
                    temp_b = float(ccm.at<double>(0, 2) * r + ccm.at<double>(1, 2) * g + ccm.at<double>(2, 2) * b);
                }

                r = temp_r;
                g = temp_g;
                b = temp_b;
                r = std::min(1.0, r);
                g = std::min(1.0, g);
                b = std::min(1.0, b);
            }
            if (input_format == 1)
            {
                r = pow(r, (1 / 2.2));
                g = pow(g, (1 / 2.2));
                b = pow(b, (1 / 2.2));
            }
            calibrated_image.at<cv::Vec3f>(row, col)[2] = float(r);
            calibrated_image.at<cv::Vec3f>(row, col)[1] = float(g);
            calibrated_image.at<cv::Vec3f>(row, col)[0] = float(b);
        }
    }
    if (do_exposure_gain)
    {
        calibrated_image = calibrated_image * exposure_gain;
    }
}

cv::Mat ColorCorrection::CalculateWhitePoint(float wb_gain[3])
{
    cv::Mat white_point = cv::Mat::zeros(cv::Size(1, 1), CV_32FC3);
    white_point.at<cv::Vec3f>(0, 0)[0] = float(1 / wb_gain[0]);
    white_point.at<cv::Vec3f>(0, 0)[1] = float(1 / wb_gain[1]);
    white_point.at<cv::Vec3f>(0, 0)[2] = float(1 / wb_gain[2]);
    return white_point;
}

cv::Mat ColorCorrection::PredictCCM(cv::Mat& white_point)
{
    cv::Mat ccm;
    float cct = CalculateCCT(white_point);
    if (cct < this->cct_list[0])
    {
        ccm = this->ccm_list[0];
    }
    else if (cct > this->cct_list[cct_list.size() - 1])
    {
        ccm = this->ccm_list[cct_list.size() - 1];
    }
    else
    {
        for (auto i = 1; i < this->cct_list.size(); i++)
        {
            if (cct < this->cct_list[i])
            {
                float cct1 = this->cct_list[i-1];
                float cct2 = this->cct_list[i];
                cv::Mat ccm1 = this->ccm_list[i-1];
                cv::Mat ccm2 = this->ccm_list[i];
                float alpha = (1/cct - 1/cct2) / (1/cct1 - 1/cct2);
                ccm = alpha * ccm1 + (1 - alpha) * ccm2;
                std::cout << cct1 << " " << cct << " " << cct2 << std::endl;
                break;
            }
        }
    }
    std::cout << "===========cct:" << cct << std::endl;
    std::cout << "===========ccm:" << ccm << std::endl;
    return ccm;
}

void ColorCorrection::PredictWBgain(cv::Mat& white_paper, float wb_gain[3], int wb_format)
{
    if (wb_format == 0)
    {
        int x = int(white_paper.cols / 2 - white_paper.cols / 8);
        int y = int(white_paper.rows / 2 - white_paper.rows / 8);
        int width = int(white_paper.cols / 4);
        int height = int(white_paper.rows / 4);

        cv::Point2f p1, p2;
        p1.x = x;
        p1.y = y;
        p2.x = x + width;
        p2.y = y + height;
        cv::rectangle(white_paper, p1, p2, cv::Scalar(255, 255, 255), 2);

        cv::Mat white_paper_ROI_1 = white_paper(cv::Rect(x, y, width, height));
        auto mean = cv::mean(white_paper_ROI_1);
        double max_value = std::max(std::max(mean[0], mean[1]), mean[2]);
        std::cout << "b:" << mean[0] / 255. << std::endl;
        std::cout << "g:" << mean[1] / 255. << std::endl;
        std::cout << "r:" << mean[2] / 255. << std::endl;
        wb_gain[0] = max_value / mean[2];
        wb_gain[1] = max_value / mean[1];
        wb_gain[2] = max_value / mean[0];
    }
    else if (wb_format == 1)
    {
        white_paper.convertTo(white_paper, CV_32FC3, float(1 / 255.));
        auto color_checker_func = ColorCheckerFunc();
        color_checker_func.DetectChecker(white_paper, 0, 1);
        int white_index = 18;

        double max_value = std::max(std::max(color_checker_func.original_charts_linearrgb.at<double>(white_index, 0), color_checker_func.original_charts_linearrgb.at<double>(white_index, 1)), color_checker_func.original_charts_linearrgb.at<double>(white_index, 2));
        std::cout << "b:" << color_checker_func.original_charts_linearrgb.at<double>(white_index, 2) << std::endl;
        std::cout << "g:" << color_checker_func.original_charts_linearrgb.at<double>(white_index, 1) << std::endl;
        std::cout << "r:" << color_checker_func.original_charts_linearrgb.at<double>(white_index, 0) << std::endl;
        
        wb_gain[0] = max_value / color_checker_func.original_charts_linearrgb.at<double>(white_index, 0);
        wb_gain[1] = max_value / color_checker_func.original_charts_linearrgb.at<double>(white_index, 1);
        wb_gain[2] = max_value / color_checker_func.original_charts_linearrgb.at<double>(white_index, 2);
    }
}

void ColorCorrection::GetCalibCCM(std::string config_path)
{
    std::ifstream inFile(config_path, std::ios::in | std::ios::binary);
    if (!inFile) {
        std::cout << "error" << std::endl;
    }
    double temp_cct;
    int ccm_format;
    std::vector<double> data;
    while (inFile.read((char*)&ccm_format, sizeof(int)))
    {
        inFile.read((char*)&temp_cct, sizeof(double));
        cv::Mat temp_ccm;
        if (ccm_format == 0)
        {
            temp_ccm = cv::Mat::zeros(cv::Size(3, 3), CV_64FC1);
        }
        else
        {
            temp_ccm = cv::Mat::zeros(cv::Size(3, 4), CV_64FC1);
        }
        this->cct_list.push_back(temp_cct);

        inFile.read((char*)&temp_ccm.at<double>(0, 0), sizeof(double));
        inFile.read((char*)&temp_ccm.at<double>(0, 1), sizeof(double));
        inFile.read((char*)&temp_ccm.at<double>(0, 2), sizeof(double));
        inFile.read((char*)&temp_ccm.at<double>(1, 0), sizeof(double));
        inFile.read((char*)&temp_ccm.at<double>(1, 1), sizeof(double));
        inFile.read((char*)&temp_ccm.at<double>(1, 2), sizeof(double));
        inFile.read((char*)&temp_ccm.at<double>(2, 0), sizeof(double));
        inFile.read((char*)&temp_ccm.at<double>(2, 1), sizeof(double));
        inFile.read((char*)&temp_ccm.at<double>(2, 2), sizeof(double));
        if (ccm_format == 1)
        {
            inFile.read((char*)&temp_ccm.at<double>(3, 0), sizeof(double));
            inFile.read((char*)&temp_ccm.at<double>(3, 1), sizeof(double));
            inFile.read((char*)&temp_ccm.at<double>(3, 2), sizeof(double));
        }
        this->ccm_list.push_back(temp_ccm);
    }

    double temp1;
    cv::Mat temp2;
    for (int i = 0; i != cct_list.size(); ++i)
    {
        for (int j = 0; j != cct_list.size(); ++j)
        {
            if (this->cct_list[i] < this->cct_list[j])
            {
                temp1 = this->cct_list[i];
                this->cct_list[i] = this->cct_list[j];
                this->cct_list[j] = temp1;

                temp2 = this->ccm_list[i].clone();
                this->ccm_list[i] = this->ccm_list[j].clone();
                this->ccm_list[j] = temp2.clone();
            }
        }
    }
}