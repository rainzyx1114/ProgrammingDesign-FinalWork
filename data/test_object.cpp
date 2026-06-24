class Calculator {
    int value;
    int add(int x) {
        value = value + x;
        return value;
    }
};

int main() {
    Calculator* calc;
    calc = new Calculator();
    calc->value = 100;
    int result;
    result = calc->add(50);
    return result;
}
