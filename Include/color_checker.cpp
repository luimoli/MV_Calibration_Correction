#include "color_checker.h"
#include "windows.h"
#include <opencv.hpp>
#include <opencv2/mcc.hpp>
#include <math.h>
#include <algorithm>
//#include <iostream>

cv::Point2f transform_points_forward(cv::Matx33f& _T, cv::Point2f X)
{
    cv::Point2f Xt;
    //cv::Matx33f _T = T.getMat();
    cv::Matx31f p, xt;
    cv::Point2f pt;
    p(0, 0) = X.x;
    p(1, 0) = X.y;
    p(2, 0) = 1;
    xt = _T * p;
    pt.x = xt(0, 0) / xt(2, 0);
    pt.y = xt(1, 0) / xt(2, 0);
    Xt = pt;
    return Xt;
}

template <typename T>
void polyanticlockwise(std::vector<T>& points)
{
    // Sort the points in anti-clockwise order
    // Trace a line between the first and second point.
    // If the third point is at the right side, then the points are anti-clockwise
    cv::Point v1 = points[1] - points[0];
    cv::Point v2 = points[2] - points[0];

    double o = (v1.x * v2.y) - (v1.y * v2.x);

    if (o < 0.0) //if the third point is in the left side, then sort in anti-clockwise order
        std::swap(points[1], points[3]);
}


void ColorCheckerFunc::GetCheckerCentroid(cv::Ptr<cv::mcc::CChecker>& checker)
{
    const cv::Point2f cellchart[24] = {
    {1.50f, 1.50f},
    {4.25f, 1.50f},
    {7.00f, 1.50f},
    {9.75f, 1.50f},
    {12.50f, 1.50f},
    {15.25f, 1.50f},
    {1.50f, 4.25f},
    {4.25f, 4.25f},
    {7.00f, 4.25f},
    {9.75f, 4.25f},
    {12.50f, 4.25f},
    {15.25f, 4.25f},
    {1.50f, 7.00f},
    {4.25f, 7.00f},
    {7.00f, 7.00f},
    {9.75f, 7.00f},
    {12.50f, 7.00f},
    {15.25f, 7.00f},
    {1.50f, 9.75f},
    {4.25f, 9.75f},
    {7.00f, 9.75f},
    {9.75f, 9.75f},
    {12.50f, 9.75f},
    {15.25f, 9.75f},
    };
    auto center = checker->getCenter();
    auto box = checker->getBox();
    cv::Size2i size = cv::Size2i(4, 6);
    cv::Size2f boxsize = cv::Size2f(11.25, 16.75);
    std::vector<cv::Point2f> fbox;
    fbox.resize(4);
    fbox[0] = cv::Point2f(0.00, 0.00);
    fbox[1] = cv::Point2f(16.75, 0.00);
    fbox[2] = cv::Point2f(16.75, 11.25);
    fbox[3] = cv::Point2f(0.00, 11.25);

    cv::Matx33f ccT = cv::getPerspectiveTransform(fbox, box);

    for (auto i = 0; i < 24; i++)
    {
         cv::Point2f Xt = transform_points_forward(ccT, cellchart[i]);
         this->centroid.push_back(Xt);
    }
}

void ColorCheckerFunc::DetectChecker(const cv::Mat& image, int input_format, float resize_ratio)
{
    auto detector = cv::mcc::CCheckerDetector::create();
    this->detected_image = image.clone();   
    if (resize_ratio != 1)
    {
        cv::resize(this->detected_image, this->detected_image, cv::Size(), resize_ratio, resize_ratio);
    }
    if (input_format == 0)
    {
        cv::pow(this->detected_image, (1 / 2.2), this->detected_image);
    }
    this->detected_image.convertTo(this->detected_image, CV_8UC3, 255);
    detector->process(this->detected_image, cv::mcc::MCC24, TRUE);
    auto checkers = detector->getListColorChecker();
    if (checkers.size() == 0)
    {
        this->is_colorchecker = FALSE;
    }
    else
    {
        auto checker = checkers[0];
        GetCheckerCentroid(checker);
        auto center = checker->getCenter();
        auto box = checker->getBox();
        auto drawer = cv::mcc::CCheckerDraw::create(checker);
        auto chartsRGB = checker->getChartsRGB();
        cv::Mat src = chartsRGB.col(1).clone().reshape(3, chartsRGB.rows / 3);
        this->original_charts_srgb = src / 255.;
        cv::pow(this->original_charts_srgb, 2.2, this->original_charts_linearrgb);
        drawer->draw(this->detected_image);
        cv::line(this->detected_image, box[0], box[1], cv::Scalar(255, 255, 0), 2);
        cv::line(this->detected_image, box[2], box[3], cv::Scalar(255, 0, 255), 2);
        cv::line(this->detected_image, box[0], box[3], cv::Scalar(255, 255, 255), 2);
        cv::line(this->detected_image, box[1], box[2], cv::Scalar(255, 255, 255), 2);
        cv::circle(this->detected_image, center, 3, cv::Scalar(255, 255, 255), 2);


        for (auto i = 18; i < 24; i++)
        {
            std::cout << "linear rgb block" << i << ":" << original_charts_linearrgb.at<cv::Vec3d>(i, 0) * 255 << "  ";
            std::cout << "G/B=" << original_charts_linearrgb.at<cv::Vec3d>(i, 0)[1] / original_charts_linearrgb.at<cv::Vec3d>(i, 0)[2] << std::endl;
        }
        this->is_colorchecker = TRUE;
    }
}


