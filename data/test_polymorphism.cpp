class Animal {
    int age;
    virtual void speak() {}
};

class Dog : public Animal {
    void speak() {}
    void bark() {}
};

int main() {
    Dog d;
    d.age = 3;
    Animal* a;
    a = &d;
    return 0;
}
