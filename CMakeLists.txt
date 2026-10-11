cmake_minimum_required(VERSION 3.10)
project(stuns C CXX)

add_executable(stuns
  Main.cpp FBaseDecoder.cpp
  Deflate/FDeflateDecoder.cpp Deflate/puff.c
  PKWare/FPKWareDecoder.cpp PKWare/blast.c
  LZO/FLZODecoder.cpp LZO/minilzo.c
  UCL/FUCL2BDecoder.cpp UCL/FUCL2DDecoder.cpp UCL/n2b_ds.c UCL/n2d_ds.c
)
target_include_directories(stuns PRIVATE UCL)

if(MSVC)
  target_compile_definitions(stuns PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

enable_testing()
add_test(NAME stuns COMMAND sh ${CMAKE_SOURCE_DIR}/tests/run.sh $<TARGET_FILE:stuns>)
