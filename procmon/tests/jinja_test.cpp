#include <iostream>
#include <string>

#include <Jinja2CppLight/Jinja2CppLight.h>

using namespace std;
using namespace Jinja2CppLight;

int main(int argc, char *argv[])
{
  const std::string source = ("Hello {{name}}!"
                              ""
                              "{% if test %}"
                              "    How are you?"
                              "{% endif %}");
  Template mytemplate(source);
  mytemplate.setValue("name", "yqsy");
  // mytemplate.setValue("test", 1);

  const string result = mytemplate.render();
  std::cout << "[" << result << "]" << endl;

  return 0;
}