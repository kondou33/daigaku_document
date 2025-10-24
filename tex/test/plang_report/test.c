#include <stdio.h>
#include "plang.h"

int main() {
    int arr[] = {1, 2, 3, 4, 5};
    int size = sizeof(arr) / sizeof(arr[0]);

    int sum = sumArray(arr, size);
    double average = averageArray(arr, size);

    printf("Sum: %d\n", sum);
    printf("Average: %.2f\n", average);

    return 0;
}