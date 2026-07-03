class Animal {
public:
    virtual void speak() {}
};

class Dog : public Animal {
public:
    virtual void speak() {}
    void bark() {}
};

int main() {
    Dog* d = new Dog();
    d->speak();
    Animal* a = d;
    a->speak();
    return 0;
}
