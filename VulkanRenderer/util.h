#pragma once

#include <iostream>

#define PRINTVEC4(v) std::cout << "(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")" << "\n";
#define PRINTVEC3(v) std::cout << "(" << v.x << ", " << v.y << ", " << v.z << ")" << "\n";
#define PRINTVEC2(v) std::cout << "(" << v.x << ", " << v.y << ")" << "\n";
#define LOG(x) std::cout << x << std::endl;
#define PRINTMAT4x4(m) PRINTVEC4(m[0]) PRINTVEC4(m[1]) PRINTVEC4(m[2]) PRINTVEC4(m[3]);