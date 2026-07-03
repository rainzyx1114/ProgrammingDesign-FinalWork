class Base {
public:
    void fun1() {}
};

class Derived : public Base {
};

int main() {
    Derived* d = new Derived();
    d->fun1();
    return 0;
}
