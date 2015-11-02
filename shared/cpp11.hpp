#pragma once
#ifndef GRAEHL_CPP11
#if __cplusplus >= 201103L || SDL_CPP11 || _MSC_VER >= 1900
#define GRAEHL_CPP11 1
#if _MSC_VER >= 1900
#define GRAEHL_CPP14_TYPETRAITS 1
#else
#define GRAEHL_CPP14_TYPETRAITS 0
#endif
#if __cplusplus >= 201400L
#define GRAEHL_CPP14 1
#define GRAEHL_CPP14_TYPETRAITS 1
#endif
#else
#define GRAEHL_CPP11 0
#define GRAEHL_CPP14 0
#define GRAEHL_CPP14_TYPETRAITS 0
#endif
#endif