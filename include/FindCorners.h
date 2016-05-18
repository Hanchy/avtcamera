#pragma once

#include <vector>
#include <algorithm>

#include <opencv2/opencv.hpp>

inline
void sortValueIndex(const std::vector<float> &_vals, std::vector<int> &_idx) {
  std::size_t n(0);
  std::generate(std::begin(_idx), std::end(_idx), [&]{return n++;});
  std::sort(std::begin(_idx), std::end(_idx),
            [&](int i1, int i2){return _vals[i1] < _vals[i2];});
}


void find_corners(const cv::Mat &_gray_img,
                  std::vector<cv::Point2f> &_corners);
  
  

