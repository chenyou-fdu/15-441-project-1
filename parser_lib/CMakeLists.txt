#cmake_minimum_required(VERSION 3.15)
#project(parser_lib)
find_package(BISON)
#BISON_TARGET(MyParser parser.y parser.c)
#add_executable(Foo main.cpp ${BISON_MyParser_OUTPUTS})
find_package(FLEX)
bison_target(MyParser parser.y ${CMAKE_CURRENT_BINARY_DIR}/y.tab.c)
flex_target(MyScanner lexer.l ${CMAKE_CURRENT_BINARY_DIR}/l.tab.c)
ADD_FLEX_BISON_DEPENDENCY(MyScanner MyParser)
include_directories(.)
#add_executable(example
#        example.c
#        parse.c
#        ${BISON_MyParser_OUTPUTS}
#        ${FLEX_MyScanner_OUTPUTS}
#)
#target_link_libraries(parser_lib parse.c ${BISON_MyParser_OUTPUTS} ${FLEX_MyScanner_OUTPUTS})
add_library(parser_lib parse.c ${BISON_MyParser_OUTPUTS} ${FLEX_MyScanner_OUTPUTS})
#target_link_libraries(parser_lib ${FLEX_LIBRARIES} ${BISON_LIBRARIES})

