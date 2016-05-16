#include "FindCorners.h"
#include <omp.h>


void find_corners(const cv::Mat &_gray_img,
                  std::vector<cv::Point2f> &_corners) {
  cv::Size image_size = _gray_img.size();

  cv::Mat blurred(image_size, CV_32F);
  cv::Mat rx(image_size, CV_32F);
  cv::Mat ry(image_size, CV_32F);
  cv::Mat rxx(image_size, CV_32F);
  cv::Mat rxy(image_size, CV_32F);
  cv::Mat ryx(image_size, CV_32F);
  cv::Mat ryy(image_size, CV_32F);
  cv::Mat rxxryy(image_size, CV_32F);
  cv::Mat rxyryx(image_size, CV_32F);

  cv::Mat S(image_size, CV_32F);

  _corners.clear();

  cv::GaussianBlur(_gray_img, blurred, cv::Size(5, 5), 0);
#pragma omp parallel sections
  {
#pragma omp section
    {
      cv::Scharr(blurred, rx, CV_32F, 1, 0);
    }
#pragma omp section
    {
      cv::Scharr(blurred, ry, CV_32F, 0, 1);
    }
  }

#pragma omp parallel sections
  {
#pragma omp parallel section
    {
      cv::Scharr(rx, rxx, CV_32F, 1, 0);
    }
#pragma omp parallel section
    {
      cv::Scharr(rx, rxy, CV_32F, 0, 1);
    }
#pragma omp parallel section
    {
      cv::Scharr(ry, ryy, CV_32F, 0, 1);
    }
#pragma omp parallel section
    {
      cv::Scharr(ry, ryx, CV_32F, 1, 0);
    }
  }
  
#pragma omp parallel sections
    {
#pragma omp section
      {
        rxxryy = rxx.mul(ryy);
      }
#pragma omp section
      {
        rxyryx = rxy.mul(ryx);
      }
    }
    
    S = rxxryy - rxyryx;

    double s_min = 0.;
    double s_max = 0.;
    cv::minMaxIdx(S, &s_min, &s_max);
    s_min *= 0.5;
    
    std::vector<cv::Point> points;
    
    float *s = nullptr;
    for (int r = 0; r < _gray_img.rows; ++r) {
      s = S.ptr<float>(r);
      for (int c = 0; c < _gray_img.cols; ++c) {
        if (s[c] < -1000. && s[c] < s_min) {
          points.push_back(cv::Point(c, r));
        }
      }
    }
    std::vector<cv::Point2f> final_points;
    std::vector<float> vals;
    std::vector<bool> accessed(points.size(), false);
    for (std::size_t i = 0; i < points.size(); ++i) {
      if (accessed[i])
        continue;
    
      accessed[i] = true;
      
      float x = points[i].x;
      float y = points[i].y;
      float val = S.at<float>(points[i]);
      int counter = 1;
      for (std::size_t j = i; j < points.size(); ++j) {
        if (accessed[j])
          continue;
        else if (std::abs(points[j].x - points[i].x) < 5 &&
                 std::abs(points[j].y - points[i].y) < 5) {
          accessed[j] = true;
          x += points[j].x;
          y += points[j].y;
          val += S.at<float>(points[j]);
          counter++;
        }
      }
      x /= counter;
      y /= counter;
      val /= counter;
      final_points.push_back(cv::Point2f(x, y));
      vals.push_back(val);
    }

    cv::cornerSubPix(
        _gray_img, final_points, cv::Size(5, 5), cv::Size(-1, -1),
        cv::TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));
    
    std::vector<int> idx(vals.size());
    sortValueIndex(vals, idx);
    for (std::size_t i = 0; i < 40 && i < vals.size(); ++i) {
      int &index = idx[i];
      cv::Point2f &ele = final_points[index];
      _corners.push_back(ele);
    }
}
