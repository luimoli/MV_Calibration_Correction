#include "color_science.h"
#include "windows.h"
#include <opencv.hpp>
#include <opencv2/mcc.hpp>
#include "color_checker.h"


double toRad(const double& degree) { return degree / 180 * CV_PI; };

void getColorChecker(cv::Mat& ideal_lab)
{
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
        { 20.461, -0.079, -0.973 } };
    for (int i = 0; i < 24; ++i)
    {
        ideal_lab.at<cv::Vec3d>(i, 0) = cv::Vec3d(ColorChecker2005_LAB_D50_2[i][0],
            ColorChecker2005_LAB_D50_2[i][1], ColorChecker2005_LAB_D50_2[i][2]);
    }
}

double deltaE_CIEDE2000(const cv::Vec3d& lab1, const cv::Vec3d& lab2, const double& kL, const double& kC, const double& kH)
{
    double delta_L_apo = lab2[0] - lab1[0];
    double l_bar_apo = (lab1[0] + lab2[0]) / 2.0;
    double C1 = sqrt(pow(lab1[1], 2) + pow(lab1[2], 2));
    double C2 = sqrt(pow(lab2[1], 2) + pow(lab2[2], 2));
    double C_bar = (C1 + C2) / 2.0;
    double G = sqrt(pow(C_bar, 7) / (pow(C_bar, 7) + pow(25, 7)));
    double a1_apo = lab1[1] + lab1[1] / 2.0 * (1.0 - G);
    double a2_apo = lab2[1] + lab2[1] / 2.0 * (1.0 - G);
    double C1_apo = sqrt(pow(a1_apo, 2) + pow(lab1[2], 2));
    double C2_apo = sqrt(pow(a2_apo, 2) + pow(lab2[2], 2));
    double C_bar_apo = (C1_apo + C2_apo) / 2.0;
    double delta_C_apo = C2_apo - C1_apo;

    double h1_apo;
    if (C1_apo == 0)
    {
        h1_apo = 0.0;
    }
    else
    {
        h1_apo = atan2(lab1[2], a1_apo);
        if (h1_apo < 0.0)
            h1_apo += 2. * CV_PI;
    }

    double h2_apo;
    if (C2_apo == 0)
    {
        h2_apo = 0.0;
    }
    else
    {
        h2_apo = atan2(lab2[2], a2_apo);
        if (h2_apo < 0.0)
            h2_apo += 2. * CV_PI;
    }

    double delta_h_apo;
    if (abs(h2_apo - h1_apo) <= CV_PI)
    {
        delta_h_apo = h2_apo - h1_apo;
    }
    else if (h2_apo <= h1_apo)
    {
        delta_h_apo = h2_apo - h1_apo + 2. * CV_PI;
    }
    else
    {
        delta_h_apo = h2_apo - h1_apo - 2. * CV_PI;
    }

    double H_bar_apo;
    if (C1_apo == 0 || C2_apo == 0)
    {
        H_bar_apo = h1_apo + h2_apo;
    }
    else if (abs(h1_apo - h2_apo) <= CV_PI)
    {
        H_bar_apo = (h1_apo + h2_apo) / 2.0;
    }
    else if (h1_apo + h2_apo < 2. * CV_PI)
    {
        H_bar_apo = (h1_apo + h2_apo + 2. * CV_PI) / 2.0;
    }
    else
    {
        H_bar_apo = (h1_apo + h2_apo - 2. * CV_PI) / 2.0;
    }

    double delta_H_apo = 2.0 * sqrt(C1_apo * C2_apo) * sin(delta_h_apo / 2.0);
    double T = 1.0 - 0.17 * cos(H_bar_apo - toRad(30.)) + 0.24 * cos(2.0 * H_bar_apo) + 0.32 * cos(3.0 * H_bar_apo + toRad(6.0)) - 0.2 * cos(4.0 * H_bar_apo - toRad(63.0));
    double sC = 1.0 + 0.045 * C_bar_apo;
    double sH = 1.0 + 0.015 * C_bar_apo * T;
    double sL = 1.0 + ((0.015 * pow(l_bar_apo - 50.0, 2.0)) / sqrt(20.0 + pow(l_bar_apo - 50.0, 2.0)));
    double R_C = 2.0 * sqrt(pow(C_bar_apo, 7.0) / (pow(C_bar_apo, 7.0) + pow(25, 7)));
    double RT = -sin(toRad(60.0) * exp(-pow((H_bar_apo - toRad(275.0)) / toRad(25.0), 2.0))) * R_C;
    double res = (pow(delta_L_apo / (kL * sL), 2.0) + pow(delta_C_apo / (kC * sC), 2.0) + pow(delta_H_apo / (kH * sH), 2.0) + RT * (delta_C_apo / (kC * sC)) * (delta_H_apo / (kH * sH)));
    return res > 0 ? sqrt(res) : 0;
}


double deltaC_CIE2000(const cv::Vec3d& lab1, const cv::Vec3d& lab2, const double& kL, const double& kC, const double& kH)
{
    double C1 = sqrt(pow(lab1[1], 2) + pow(lab1[2], 2));
    double C2 = sqrt(pow(lab2[1], 2) + pow(lab2[2], 2));
    double C_bar = (C1 + C2) / 2.0;
    double G = sqrt(pow(C_bar, 7) / (pow(C_bar, 7) + pow(25, 7)));
    double a1_apo = lab1[1] + lab1[1] / 2.0 * (1.0 - G);
    double a2_apo = lab2[1] + lab2[1] / 2.0 * (1.0 - G);
    double C1_apo = sqrt(pow(a1_apo, 2) + pow(lab1[2], 2));
    double C2_apo = sqrt(pow(a2_apo, 2) + pow(lab2[2], 2));
    double C_bar_apo = (C1_apo + C2_apo) / 2.0;
    double delta_C_apo = C2_apo - C1_apo;

    double h1_apo;
    if (C1_apo == 0)
    {
        h1_apo = 0.0;
    }
    else
    {
        h1_apo = atan2(lab1[2], a1_apo);
        if (h1_apo < 0.0)
            h1_apo += 2. * CV_PI;
    }

    double h2_apo;
    if (C2_apo == 0)
    {
        h2_apo = 0.0;
    }
    else
    {
        h2_apo = atan2(lab2[2], a2_apo);
        if (h2_apo < 0.0)
            h2_apo += 2. * CV_PI;
    }

    double delta_h_apo;
    if (abs(h2_apo - h1_apo) <= CV_PI)
    {
        delta_h_apo = h2_apo - h1_apo;
    }
    else if (h2_apo <= h1_apo)
    {
        delta_h_apo = h2_apo - h1_apo + 2. * CV_PI;
    }
    else
    {
        delta_h_apo = h2_apo - h1_apo - 2. * CV_PI;
    }

    double H_bar_apo;
    if (C1_apo == 0 || C2_apo == 0)
    {
        H_bar_apo = h1_apo + h2_apo;
    }
    else if (abs(h1_apo - h2_apo) <= CV_PI)
    {
        H_bar_apo = (h1_apo + h2_apo) / 2.0;
    }
    else if (h1_apo + h2_apo < 2. * CV_PI)
    {
        H_bar_apo = (h1_apo + h2_apo + 2. * CV_PI) / 2.0;
    }
    else
    {
        H_bar_apo = (h1_apo + h2_apo - 2. * CV_PI) / 2.0;
    }

    double delta_H_apo = 2.0 * sqrt(C1_apo * C2_apo) * sin(delta_h_apo / 2.0);
    double T = 1.0 - 0.17 * cos(H_bar_apo - toRad(30.)) + 0.24 * cos(2.0 * H_bar_apo) + 0.32 * cos(3.0 * H_bar_apo + toRad(6.0)) - 0.2 * cos(4.0 * H_bar_apo - toRad(63.0));
    double sC = 1.0 + 0.045 * C_bar_apo;
    double sH = 1.0 + 0.015 * C_bar_apo * T;
    double R_C = 2.0 * sqrt(pow(C_bar_apo, 7.0) / (pow(C_bar_apo, 7.0) + pow(25, 7)));
    double RT = -sin(toRad(60.0) * exp(-pow((H_bar_apo - toRad(275.0)) / toRad(25.0), 2.0))) * R_C;
    double res = (pow(delta_C_apo / (kC * sC), 2.0) + pow(delta_H_apo / (kH * sH), 2.0) + RT * (delta_C_apo / (kC * sC)) * (delta_H_apo / (kH * sH)));
    return res > 0 ? sqrt(res) : 0;
}





void CalculateDeltaE(cv::Mat& lab1, cv::Mat& lab2, std::vector<double>& delta_E, std::vector<double>& delta_C)
{
    for (auto i = 0; i < 24; i++)
    {
        double temp = deltaE_CIEDE2000(lab1.at<cv::Vec3d>(0, i), lab2.at<cv::Vec3d>(0, i));
        delta_E.push_back(temp);
        double temp2 = deltaC_CIE2000(lab1.at<cv::Vec3d>(0, i), lab2.at<cv::Vec3d>(0, i));
        delta_C.push_back(temp2);
    }
}

void EvaluateImage(cv::Mat& image, int input_format, std::vector<double>& delta_E, std::vector<double>& delta_C)
{
    cv::Mat image2 = image.clone();
    if (input_format == 0)
    {
        cv::pow(image2, (1 / 2.2), image2);
    }
    auto color_checker = ColorCheckerFunc();
    color_checker.DetectChecker(image2, 1, 1);
    if (color_checker.is_colorchecker)
    {
        auto charts_srgb = color_checker.original_charts_srgb;
        for (auto i = 18; i < 24; i++)
        {
            std::cout << "block" << i << ":" << color_checker.original_charts_srgb.at<cv::Vec3d>(i, 0) << std::endl;
        }

        cv::Mat charts_lab;
        cv::Mat ideal_lab = cv::Mat::zeros(24, 1, CV_64FC3);
        getColorChecker(ideal_lab);
        charts_srgb.convertTo(charts_srgb, CV_32FC3);
        cv::cvtColor(charts_srgb, charts_lab, cv::COLOR_RGB2Lab);
        charts_lab.convertTo(charts_lab, CV_64FC3);

        CalculateDeltaE(charts_lab, ideal_lab, delta_E, delta_C);

        for (auto i = 0; i < 24; i++)
        {
            cv::putText(image, std::to_string(int(10 * delta_C[i])), color_checker.centroid[i], cv::FONT_HERSHEY_COMPLEX, 2, cv::Scalar(0, 1, 1));
        }
    }
}