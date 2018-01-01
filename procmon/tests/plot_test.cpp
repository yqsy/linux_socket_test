#include <fstream>

#include <muduo/base/Types.h>

#include <gd.h>
#include <gdfonts.h>

typedef struct gdImageStruct *gdImagePtr;

using namespace muduo;

int main()
{
  int width = 640;
  int height = 100;

  gdImagePtr image = gdImageCreate(width, height);

  const int background = gdImageColorAllocate(image, 255, 255, 240);
  gdImageFilledRectangle(image, 0, 0, width, height, background);

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
