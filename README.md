![GitHub](https://img.shields.io/github/license/proxict/lib-optional) ![TravisCI](https://travis-ci.org/proxict/lib-optional.svg?branch=master)

lib-optional
------------

This library provides an `Optional` type - wrapper for representing an optional object which may or may not contain an initialized value.
Also, this implementation allows storing references:

```c++
int k = 3;
Optional<int&> intRef(k);
k = 1;
assert(*intRef == 1);
```

The whole implementation is in `libOptional` namespace.
 
Integration with CMake
----------------------------
```cmake
add_subdirectory(third-party/lib-optional)
target_link_libraries(your-target
    PRIVATE lib-optional
)
```

Tests can be allowed by setting `BUILD_TESTS` variable to `TRUE`:
```c++
mkdir -p build && cd build
cmake -DBUILD_TESTS=1 ..
```

How to use?
-----------
Hopefully, this short example might give you a rough idea about how this type could be used:
```c++
#include <lib-optional/optional.hpp>
using namespace libOptional;

Optional<std::string> getAttribute(const std::string& attributeName) {
    auto attribute = mAttributes.find(attributeName);
    if (attribute != mAttributes.end()) {
        return *attribute;
    }
    return NullOptional;
}

int main() {
    auto id = getAttribute("id");
    if (id) {
        std::cout << "id=" << *id << '\n';
    }
    return 0;
}
```

What's the difference from `std::optional`?
-------------------------------------------
`std::optional` is only available since C++17 and this library offers nearly the same functionality but in C++11 standard.
I was trying to follow the standard, so there shouldn't be many differences, but I'm sure there are some deviations from the standard.
