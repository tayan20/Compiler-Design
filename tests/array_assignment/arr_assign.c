// Self-wrriten test MiniC program to test array assignment

extern int print_int(int X);

int test_array_assignment() {
    int arr[5];
    int x;

    arr[0] = 10;
    arr[1] = 20;
    arr[2] = arr[0] + arr[1];

    x = arr[2];
    print_int(x);

    return x;
}