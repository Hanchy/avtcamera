#include <opencv2/opencv.hpp>
#include <iostream>
#include <limits>
#include <csignal>
#include <cfloat>
#include <cstdio>
#include <vector>
#include <array>
#include <algorithm>
#include <chrono>
#include <thread>

#include <omp.h>

static bool stop = false;
void sigIntHandler(int signal) {
  stop = true;
}


inline
float getValueSubpix(const cv::Mat &_img, cv::Point2f &_pt) {
  assert(!_img.empty());
  assert(_img.channels() == 1);

  int x = (int)_pt.x;
  int y = (int)_pt.y;

  int x0 = cv::borderInterpolate(x,   _img.cols, cv::BORDER_REFLECT_101);
  int x1 = cv::borderInterpolate(x+1, _img.cols, cv::BORDER_REFLECT_101);
  int y0 = cv::borderInterpolate(y,   _img.rows, cv::BORDER_REFLECT_101);
  int y1 = cv::borderInterpolate(y+1, _img.rows, cv::BORDER_REFLECT_101);

  float a = _pt.x - (float)x;
  float c = _pt.y - (float)y;

  float val = (1.f - c) *(_img.at<float>(y0, x0) * (1.f - a) +
                           _img.at<float>(y0, x1) * a) +
               c * (_img.at<float>(y1, x0) * (1.f - a) +
                    _img.at<float>(y1, x1) * a);

  return val;
}


inline
void sortValueIndex(const std::vector<float> &_vals, std::vector<int> &_idx) {
  std::size_t n(0);
  std::generate(std::begin(_idx), std::end(_idx), [&]{return n++;});
  std::sort(std::begin(_idx), std::end(_idx),
            [&](int i1, int i2){return _vals[i1] < _vals[i2];});
}


int main(int argc, char **argv) {
  cv::VideoCapture cap(0);
  if (!cap.isOpened())
    return -1;

  std::signal(SIGINT, sigIntHandler);
  
  cv::Mat img;
  cv::Mat gray(640, 480, CV_32F);
  cv::Mat blurred(640, 480, CV_32F);
  cv::Mat rx(640, 480, CV_32F);
  cv::Mat ry(640, 480, CV_32F);
  cv::Mat rxx(640, 480, CV_32F);
  cv::Mat rxy(640, 480, CV_32F);
  cv::Mat ryx(640, 480, CV_32F);
  cv::Mat ryy(640, 480, CV_32F);
  cv::Mat rxxryy(640, 480, CV_32F);
  cv::Mat rxyryx(640, 480, CV_32F);
  //cv::Mat A(640, 480, CV_64F);
  //cv::Mat B(640, 480, CV_64F);
  //cv::Mat lambda1(640, 480, CV_64F);
  //cv::Mat lambda2(640, 480, CV_64F);
  cv::Mat S(640, 480, CV_32F);
  double s_min = 0.;
  double s_max = 0.;
  cv::Scalar RED = cv::Scalar(0, 0, 255);
  //cv::Scalar GREEN = cv::Scalar(0, 255, 0);


  std::vector<cv::Point> points;
  std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
  std::size_t num_frames = 0;
  //img = cv::imread("/home/fans/Pictures/kinect_chessboard.png");

  for (;!stop;)
  {
    points.clear();
    cap >> img;
    num_frames++;
    cv::cvtColor(img, gray, CV_BGR2GRAY);
    // cv::equalizeHist(gray, gray);
    cv::GaussianBlur(gray, blurred, cv::Size(5, 5), 0);
#pragma omp parallel sections
    {
#pragma omp section
      {
        cv::Scharr(blurred, rx, CV_32F, 1, 0);
        cv::Scharr(rx, rxx, CV_32F, 1, 0);
        cv::Scharr(rx, rxy, CV_32F, 0, 1);
      }
#pragma omp section
      {
        cv::Scharr(blurred, ry, CV_32F, 0, 1);
        cv::Scharr(ry, ryx, CV_32F, 1, 0);
        cv::Scharr(ry, ryy, CV_32F, 0, 1);
      }
    }
    //A = rxx + ryy;
    //cv::sqrt((rxx - ryy).mul(rxx-ryy) + rxy.mul(4*ryx), B);
    //lambda1 = A + B;
    //lambda2 = A - B;
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
    
    cv::minMaxIdx(S, &s_min, &s_max);
    s_min *= 0.5;
    float *s = nullptr;
    for (int r = 0; r < img.rows; ++r) {
      s = S.ptr<float>(r);
      for (int c = 0; c < img.cols; ++c) {
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
        gray, final_points, cv::Size(5, 5), cv::Size(-1, -1),
        cv::TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));
    
    
    std::vector<int> idx(vals.size());
    sortValueIndex(vals, idx);
    for (std::size_t i = 0; i < 40 && i < vals.size(); ++i) {
      int &index = idx[i];
      cv::Point2f &ele = final_points[index];
      // float sx = getValueSubpix(rx, ele);
      // float sy = getValueSubpix(ry, ele);
      // float sxx = getValueSubpix(rxx, ele);
      // float sxy = getValueSubpix(rxy, ele);
      // float syx = getValueSubpix(ryx, ele);
      // float syy = getValueSubpix(ryy, ele);
      // float numerator = sxx * syy - sxy * syx;
      // ele.x += (sy * sxy - sx * syy) / numerator;
      // ele.y += (sx * syx - sy * sxx) / numerator;
      cv::circle(img, ele, 2, RED, -1);
    }
    // for (auto ele = final_points.begin(); ele != final_points.end(); ele++) {
    //   double sx = getValueSubpix(rx, *ele);
    //   double sy = getValueSubpix(ry, *ele);
    //   double sxx = getValueSubpix(rxx, *ele);
    //   double sxy = getValueSubpix(rxy, *ele);
    //   double syx = getValueSubpix(ryx, *ele);
    //   double syy = getValueSubpix(ryy, *ele);
    //   double numerator = sxx * syy - sxy * syx;
    //   ele->x += (sy * sxy - sx * syy) / numerator;
    //   ele->y += (sx * syx - sy * sxx) / numerator;
    //   cv::circle(img, *ele, 5, RED);
    // }
        
    
    cv::imshow("img", img);
    cv::waitKey(1);
  }
  std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
  double dur = std::chrono::duration<double>(t2 - t1).count();
  std::cout << num_frames/dur << std::endl;
  
  cv::destroyAllWindows();
  cap.release();
  return 0;
}
