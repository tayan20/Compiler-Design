extern int print_int(int X);

int add(int a, int b) {
    return a + b;
}

int main() {
    int x; 
    x = add(1, 2, 3); // ERROR: too many arguments
    return x;
}