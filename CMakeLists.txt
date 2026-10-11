cmake_minimum_required(VERSION 3.15)
project(stuns C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(stuns
  Main.cpp FAnalysis.cpp
  Deflate/FDeflateDecoder.cpp Deflate/puff.c
  PKWare/FPKWareDecoder.cpp PKWare/blast.c
  LZO/FLZODecoder.cpp LZO/minilzo.c
  UCL/FUCLDecoder.cpp UCL/n2b_ds.c UCL/n2d_ds.c UCL/n2e_ds.c
  LZ4/FLZ4Decoder.cpp
  LZMA/FLZMADecoder.cpp
  aPLib/FAPLibDecoder.cpp
)
target_include_directories(stuns PRIVATE UCL)

if(MSVC)
  target_compile_definitions(stuns PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

enable_testing()
add_test(NAME stuns COMMAND sh ${CMAKE_SOURCE_DIR}/tests/run.sh $<TARGET_FILE:stuns>)
