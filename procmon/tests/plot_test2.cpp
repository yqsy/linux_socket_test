#include <algorithm>
#include <assert.h>
#include <cmath>
#include <iostream>
#include <limits>
#include <math.h>
#include <random>
#include <string>
#include <vector>

#include <boost/format.hpp>

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
  // 过去10分钟, 600s, 周期为2, 一共300个点
  auto doubles = gen_doubles(300);

  print_doubles(doubles);

  // 打印至百分位
  std::cout << "max: " << format_double2(get_max(doubles)) << std::endl;

  // round up 靠近0.0或者0.5
  std::cout << "round up max: " << format_double(round_up_half(get_max(doubles))) << std::endl;

  auto max = std::max(1.0, round_up_half(get_max(doubles)));

  // 纵轴集合点
  auto y_collectios = split_section(max);

  std::cout << "y_collectios: ";
  print_doubles(y_collectios);

  int width = 600;
  int height = 100;

  return 0;
}
