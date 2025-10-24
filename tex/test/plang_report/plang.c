#include <stdio.h>

int sumArray(int *arr, int size) {
    int sum = 0;
    for (int i = 0; i < size; i++) {
        sum += arr[i];
    }
    return (int)sum;
}

double averageArray(int *arr, int size) {
    int sum = sumArray(arr, size);
    return (double)sum / size;
}