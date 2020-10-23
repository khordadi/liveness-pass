struct structure {
    int i;
};

int foo() {
    int a = 1, b = 2, c = 3;

    int result = 123;

    if (a > 0) {
        int d = a * b;
        int e = b / c;
        if (d == e) {
            struct structure s2;
            s2.i = 21;
        } else {
            struct structure s1;
            s1.i = 12;
        }
    } else {
        result = 321;
        int arr[66];
    }
    return result;
}