//
// Copyright (c) 2022 Christopher Gassib
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// build command:
// g++ -std=c++20 gen_sintable.cpp -o gen_sintable
//

#include <cassert>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include "clg-math/clg_math.hpp"

constexpr int sinePer90Deg = 400;
double sine_table[sinePer90Deg + 1]; // [0, pi/2] or (400 samples per 90 degrees

void GenerateSineTable()
{
    constexpr double rad_inc = clg::trig<double>::half_pi / sinePer90Deg;
    for (int i = 0; i <= sinePer90Deg; i++)
    {
        sine_table[i] = std::sin(rad_inc * i);
    }
}

inline constexpr float sin_index(int index)
{
    if (index <= sinePer90Deg) // [0, 400]
    {
        assert(index >= 0 && index <= sinePer90Deg);
        return sine_table[index];
    }
    else // [401, 800]
    {
        index = sinePer90Deg - (index - sinePer90Deg); // 400 - (401 - 400) = 399; 400 - (800 - 400) = 0
        assert(index >= 0 && index <= sinePer90Deg);
        return sine_table[index];
    }
}

inline constexpr float sin_lookup(float rad)
{
    constexpr float rad_inc = clg::trig<float>::half_pi / sinePer90Deg;
    constexpr float half_inc = rad_inc * 0.5f;
    constexpr float inv_rad_inc = 1.0f / rad_inc;
    int index = static_cast<int>((rad + half_inc) * inv_rad_inc);
    index %= sinePer90Deg * 4;
    if (index <= sinePer90Deg * 2)
    {
        return sin_index(index);
    }
    else
    {
        return -sin_index(index - sinePer90Deg * 2);
    }
}

inline constexpr float cos_lookup(float rad)
{
    constexpr float rad_inc = clg::trig<float>::half_pi / sinePer90Deg;
    constexpr float half_inc = rad_inc * 0.5f;
    constexpr float inv_rad_inc = 1.0f / rad_inc;
    int index = static_cast<int>((rad + half_inc) * inv_rad_inc) + sinePer90Deg;
    index %= sinePer90Deg * 4;
    if (index <= sinePer90Deg * 2)
    {
        return sin_index(index);
    }
    else
    {
        return -sin_index(index - sinePer90Deg * 2);
    }
}

int main()
{
    using namespace std;

    GenerateSineTable();

    cout << "constexpr int sinePer90Deg = " << sinePer90Deg << ";\n"
        << "constexpr float sine_table[sinePer90Deg + 1] =\n{\n";
    for (int i = 0; i <= sinePer90Deg;)
    {
        cout << "    ";
        for (int j = 0; j < 6 && i <= sinePer90Deg; j++, i++)
        {
            cout << fixed << setprecision(8) << setw(10) << sine_table[i] << "f, ";
        }
        cout << "\n";
    }
    cout << "};\n";

    // for (int i = 0; i < sinePer90Deg; i++)
    // {
    //     cout << "[" << i << "] " << sine_table[i] << "\n";// " -- " << clg::to_degrees(sine_table[i]) << "\n";
    // }

    // const double inc = clg::trig<double>::two_pi / (sinePer90Deg * 4);
    // for (int i = 0; i < sinePer90Deg * 4; i++)
    // {
    //     const auto rads = inc * i;
    //     const auto sin_result = sin_lookup(rads);
    //     const auto cos_result = cos_lookup(rads);
    //     cout << "sin[" << i << "] " << sin_result << " -- " << clg::to_degrees(rads) << " *** ";
    //     cout << "cos[" << i << "] " << cos_result << " -- " << clg::to_degrees(rads) << "\n";
    // }

    return 0;
}
