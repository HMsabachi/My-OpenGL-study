#pragma once

// ‘§±‡“Î∫Í
#ifdef DEBUG
#define GL_CALL(func) func; checkError(); // call the function and check for errors after it
#else
#define GL_CALL(func) func // call the function without checking for errors
#endif

void checkError();

template<typename T>
void printArray(T* arr, int size) {
    for (int i = 0; i < size; i++) {
        std::cout << arr[i] << " ";
    }
    std::cout << std::endl;
}