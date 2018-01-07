// 画的和陈硕的procmon差不多

#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>

#include <math.h>

#include <muduo/base/Types.h>

#include <gd.h>
#include <gdfonts.h>

typedef struct gdImageStruct *gdImagePtr;

using namespace muduo;

int main()
{
  int width = 640;
  int height = 100;
  auto total_seconds = 600;
  auto period = 2;

  gdImagePtr image = gdImageCreate(width, height);

  const int background = gdImageColorAllocate(image, 255, 255, 240);
  gdImageFilledRectangle(image, 0, 0, width, height, background);
  gdImageSetThickness(image, 2);

  std::vector<double> cpu_doubles;

  int max_size = 300;
  for (int i = 0; i < max_size; ++i)
  {
    cpu_doubles.push_back(1.0 + sin(pow(i / 30.0, 2)));
  }

  double max = *std::max_element(cpu_doubles.begin(), cpu_doubles.end());
  if (max >= 10.0)
  {
    max = ceil(max);
  }
  else
  {
    max = std::max(0.1, ceil(max * 10.0) / 10.0); // 精确到小数点十分位(ROUNDUP)
  }

  // 画max value
  char buf[64];
  if (max >= 10.0)
  {
    snprintf(buf, sizeof(buf), "%.0f", max);
  }
  else
  {
    snprintf(buf, sizeof(buf), "%.1f", max);
  }

  auto font = gdFontGetSmall();
  auto font_width = font->w;
  auto font_height = font->h;
  auto left_margin = 5;
  auto right_margin = 3 * font_width + 5;
  auto marginy = 5;

  // 画max value
  auto black = gdImageColorAllocate(image, 0, 0, 0);
  gdImageString(image, font, width - right_margin, marginy,
                reinterpret_cast<unsigned char *>(buf), black);

  // 画最新的时间 0s
  snprintf(buf, sizeof(buf), "0");
  gdImageString(image, font, width - right_margin,
                height - marginy - font_height,
                reinterpret_cast<unsigned char *>(buf), black);

  // 画最起始的时间
  snprintf(buf, sizeof(buf), "-%ds", total_seconds);
  gdImageString(image, font, left_margin, height - marginy - font_height,
                reinterpret_cast<unsigned char *>(buf), black);

  // 画线(把每个点连起来)
  for (size_t i = 0; i < cpu_doubles.size() - 1; ++i)
  {
    double x1 = double(width) * (double(i) / double(max_size));
    double y1 = double(height) * (1.0 - double(cpu_doubles[i]) / max);

    double x2 = double(width) * (double(i + 1) / double(max_size));
    double y2 = double(height) * (1.0 - double(cpu_doubles[i + 1]) / max);

    gdImageLine(image, x1, y1, x2, y2, black);
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
