#include <cassert>
#include <iostream>
#include <stddef.h>

  struct some_t {
    std::string name;
    const char* a     = "ahbsjs";
    int         b [3] = {1, 2, 3};
    float       c     = 3.14;
  } some {.name="some"};

std::ostream& operator<<(std::ostream& os, const some_t& s) {
  os << s.name <<": {"<< "a: " << s.a << " b: " << s.b << " c: " << s.c <<'}'<< '\n';  
  return os;
};

int main() {

  std::cout   << "\n\
  struct some_t {\n\
  \tstd::string name;\n\
  \tconst char*\t\ta\t= \"ahbsjs\";\n\
  \tint\t\t\tb [3]\t= {1, 2, 3};\n\
  \tfloat\t\t\tc\t= 3.14;\n\
  } some {.name=\"some\"};" << "\n\n";

  std::cout << &some.name << "\t<- Address of some.name" << '\n';
  std::cout << &some.a << "\t<- Address of some.a" << '\n';
  std::cout << &some.b << "\t<- Address of some.b" << '\n';
  std::cout << &some.c << "\t<- Address of some.c" << '\n';
  std::cout << &some << "\t<- Address of some" << '\n';

  some_t *addr = reinterpret_cast<some_t*>((char*)&some.c - offsetof(struct some_t, c));
  std::cout << addr << "\t<- derived addr <- reinterpret_cast<some_t*>((char*)&some.c - offsetof(struct some_t, c));\n\n";
  std::cout << some;
  std::cout << (addr->c=982.44, "Just modified some_t some with the derived address\n");
  std::cout << some <<"\n";
  
  assert((char*)&some==(char*)&some.name);
}
