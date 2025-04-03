#include <stdio.h>

int main() {
    int arr[100] = {2, 4, 6, 8, 10}; // Initial array
    int size = 5; // Current size of the array

    int newPosition = 4; // Index where you want to insert the new element
    int newElement = 7; // Element to be inserted

    // Shift elements to make space for the new element
    for (int i = size - 1; i >= newPosition; i--) {
        arr[i + 1] = arr[i];
    }

    // Insert the new element at the specified position
    arr[newPosition] = newElement;

    size++; // Update the size of the array

    // Print the updated array
    for (int i = 0; i < size; i++) {
        printf("%d ", arr[i]);
    }

    printf("\n");

    return 0;
}
