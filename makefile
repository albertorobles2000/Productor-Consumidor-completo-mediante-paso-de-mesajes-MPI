# ************ Compilación de módulos ************ 

prodcons_all: prodcons_all.cpp
	mpicxx -std=c++11 -o prodcons_all prodcons_all.cpp
