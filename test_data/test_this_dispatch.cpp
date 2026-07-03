class Base {
public:
    void caller() {
        callee();
    }
    virtual void callee() {}
};

class Derived : public Base {
public:
    virtual void callee() {}
};

int main() {
    Base* b = new Derived();
    b->caller();
    return 0;
}
