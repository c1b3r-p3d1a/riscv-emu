volatile int value;

int sum_until(int n) {
    int total = 0;
    int i = 1;
    while (i <= n) {
        total = total + i;
        i = i + 1;
    }
    return total;
}

void _start(void) {
    value = sum_until(10);
    while (1) {}
}