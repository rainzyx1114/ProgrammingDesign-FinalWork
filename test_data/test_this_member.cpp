class Calc {
    int value;
public:
    int add(int x) {
        value = value + x;
        return value;
    }
};

int main() {
    Calc* c = new Calc();
    c->value = 100;
    int result = c->add(50);
    return result;
}
