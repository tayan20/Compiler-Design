// Self-written MiniC program to test global array declarations

extern int print_int(int X);

int globalArr[10];
float globalMatrix[4][4];
int global3D[2][3][4];

int arr_global_decl() {
    int x;

    x = 10;
    print_int(x);
    return x;
}