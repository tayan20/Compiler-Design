// MiniC program to test array declarations

extern int print_int(int X);

int arr_declaration() {
    int arr[5];
    int matrix[3][3];
    float cube[2][2][2];
    int x;

    x = 10;
    print_int(x);
    return x;
}