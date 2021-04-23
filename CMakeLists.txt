cmake_minimum_required(VERSION 3.19)
project(pokemon_randomizer)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(CMAKE_CXX_STANDARD 17)

add_executable(pokemon_randomizer src/main.cpp src/pokemon.cpp src/items.cpp src/item.cpp src/rom.cpp)