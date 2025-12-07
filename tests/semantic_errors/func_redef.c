// Semantic error test: Function redefinition

int foo() {
    return 1;
}

int foo() {
    return 2;
}

int test() {
    return foo();
}