#include <algorithm>
#include <assert.h>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <math.h>
#include <random>
#include <string>
#include <vector>

#include <muduo/base/Types.h>

#include <boost/format.hpp>

#include <gd.h>
#include <gdfonts.h>

using namespace muduo;

typedef struct gdImageStruct *gdImagePtr;

std::vector<double> gen_doubles(int num)
{
  assert(num > 0);

  std::mt19937 gen(std::random_device{}());
  std::uniform_real_distribution<> dis(0, 100);
  std::vector<double> r0;
  for (int i = 0; i < num; ++i)
  {
    auto d = dis(gen);
    r0.push_back(d);
  }
  return r0;
}

bool are_same(double a, double b) { return std::fabs(a - b) < std::numeric_limits<double>::epsilon(); }

// 精确到十分位
std::string format_double(double d) { return boost::str(boost::format("%.1f") % d); }

// 精确到百分位
std::string format_double2(double d) { return boost::str(boost::format("%.2f") % d); }

// round up 十分位
double round_up_tenths(double d) { return ceil(d * 10.0) / 10.0; }

// round up 靠近0.0或者0.5
double round_up_half(double d)
{
  assert(d > 0.0);

  // 先round up 到十分位
  d = ceil(d * 10.0) / 10.0;

  double fractpart, intpart;
  fractpart = modf(d, &intpart);

  if (are_same(fractpart, 0.0) || are_same(fractpart, 0.5))
  {
    return d;
  }
  else if (fractpart < 0.5)
  {
    return intpart + 0.5;
  }
  else
  {
    return intpart + 1.0;
  }
}

void print_doubles(const std::vector<double> &doubles)
{
  for (auto d : doubles)
  {
    assert(are_same(d, 0.0) || d > 0.0);
    std::cout << format_double(d) << " ";
  }

  std::cout << std::endl;
}

double get_max(const std::vector<double> &doubles)
{
  auto max = *std::max_element(doubles.begin(), doubles.end());
  return max;
}

// 水平分割线,分割线最少有8根
// max_d 不能太小,因为不够切分了
std::vector<double> split_section(double max_d)
{
  auto section_point = max_d / 7;

  std::vector<double> doubles{0.0};

  double one_val = section_point;

  for (int i = 0; i < 7; i++)
  {
    doubles.push_back(one_val);
    one_val += section_point;
  }

  return doubles;
}

int main(int argc, char *argv[])
{
  const double margin = 15;

  const double width = 700 + margin * 2;
  const double height = 165;

  // 一共要塞满300个横轴点
  const int max_points = 300;

  // 过去10分钟, 600s, 周期为2, 一共300个点
  auto doubles = gen_doubles(300);

  print_doubles(doubles);

  // 打印至百分位
  std::cout << "max: " << format_double2(get_max(doubles)) << std::endl;

  // round up 靠近0.0或者0.5
  std::cout << "round up max: " << format_double(round_up_half(get_max(doubles))) << std::endl;

  auto max = std::max(1.0, round_up_half(get_max(doubles)));

  // 纵轴集合点(数值,非坐标点)(时间维度是递增)(数值维度)
  // auto y_collections = split_section(max);

  auto y_collections = doubles;

  std::cout << "y_collections: ";
  print_doubles(y_collections);

  // 纵轴集合点转换成纵轴坐标集合点
  for (size_t i = 0; i < y_collections.size(); i++)
  {
    y_collections[i] = (1 - y_collections[i] / max) * height;
  }

  // 给纵轴集合点加上时间维度
  // 递增画上时间点
  // 考虑到点可能不会充足,所以从左至右画时要加上补填的长度
  std::vector<double> x_collections;
  double draw_points_len = ((double)y_collections.size() / (double)max_points) * (double)width;
  double fill_points_len = width - draw_points_len;

  for (size_t i = 0; i < y_collections.size(); ++i)
  {
    double x = margin / 2 + fill_points_len + draw_points_len * ((double)i / (double)y_collections.size());
    x_collections.push_back(x);
  }
  assert(y_collections.size() == x_collections.size());

  gdImagePtr image = gdImageCreate(width, height);

  // 背景灰色线
  // const int background_line_color = gdImageColorAllocate(image, 230, 233, 235);

  // 背景白色线
  const int background_color = gdImageColorAllocate(image, 255, 255, 255);

  gdImageFilledRectangle(image, 0, 0, width, height, background_color);
  gdImageSetThickness(image, 2);

  // 统计蓝色线
  const int statistics_color = gdImageColorAllocate(image, 51, 132, 225);
  // 连线
  for (size_t i = 0; i < x_collections.size() - 1; ++i)
  {
    double x1 = x_collections[i];
    double y1 = y_collections[i];

    double x2 = x_collections[i + 1];
    double y2 = y_collections[i + 1];
    gdImageLine(image, x1, y1, x2, y2, statistics_color);
  }

  int size = 0;
  void *png = gdImagePngPtr(image, &size);
  string result(static_cast<char *>(png), size);

  std::ofstream file;
  file.open("xx.png");
  file << result;
  file.close();
  gdFree(png);
  gdImageDestroy(image);
  return 0;
}
