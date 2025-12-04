// Self-written MiniC program to test array access

extern int print_int(int X);

int test_array_acces() {
    int arr[5];
    int x;

    x = arr[0];
    print_int(arr[2]);

    return x;
}